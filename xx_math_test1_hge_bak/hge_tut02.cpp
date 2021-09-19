#include "../../../xxlib/src/xx_math.h"
#include "../../../xxlib/src/xx_ptr.h"
#include "../../../xxlib/src/xx_string.h"
#include "../../../xxlib/src/xx_randoms.h"
#include "../../include/hge.h"
#include <omp.h>
#include <thread>

namespace GameLogic {
    // 下列代码模拟 一组 圆形对象, 位于 2d 空间，一开始挤在一起，之后物理随机排开, 直到静止。统计一共要花多长时间。
    // 数据结构方面不做特殊优化，走正常 oo 风格
    // 要对比的是 用 或 不用 grid 系统的效率

    // 模拟配置. 格子尺寸, grid 列数均为 2^n 方便位运算
    static const int gridWidth = 128;
    static const int gridHeight = 64;
    static const int gridDiameter = 128;

    static const int mapMinX = gridDiameter;							// >=
    static const int mapMaxX = gridDiameter * (gridWidth - 1) - 1;		// <=
    static const int mapMinY = gridDiameter;
    static const int mapMaxY = gridDiameter * (gridHeight - 1) - 1;
    static const xx::XY mapCenter = { (mapMinX + mapMaxX) / 2, (mapMinY + mapMaxY) / 2 };

    // Foo 的半径 / 直径 / 移动速度( 每帧移动单位距离 ) / 邻居查找个数
    static const int fooRadius = 50;
    static const int fooSpeed = 5;
    static const int fooFindNeighborLimit = 20;	// 9格范围内通常能容纳的个数

    struct Scene;
    struct Foo {
        // 当前半径
        int radius = fooRadius;
        // 当前速度
        int speed = fooSpeed;
        // 所在 grid 下标. 放入 im->grid 后，该值不为 -1
        int gridIndex = -1;
        // 当前坐标( Update2 单线程填充, 同时同步 grid )
        xx::XY xy;
        // 即将生效的坐标( Update 多线并行填充 )
        xx::XY xy2;

        // 构造时就放入 grid 系统
        Foo(Scene* const& scene, xx::XY const& xy);

        // 计算下一步 xy 的动向, 写入 xy2
        void Update(Scene* const& scene);

        // 令 xy2 生效, 同步 grid
        void Update2(Scene* const& scene);
    };

    struct Scene {
        Scene() = default;
        Scene(Scene const&) = delete;
        Scene& operator=(Scene const&) = delete;

        // 格子系统( int16 则总 item 个数不能超过 64k, int 就敞开用了 )
        xx::Grid2d<Foo*, int> grid;

        // 对象容器 需要在 格子系统 的下方，确保 先 析构，这样才能正确的从 格子系统 移除
        std::vector<Foo> objs;

        // 随机数一枚, 用于对象完全重叠的情况下产生一个移动方向
        xx::Random1 rnd;

        // 每帧统计还有多少个对象正在移动
        int count = 0;

        Scene(int const& fooCount) {
            // 初始化 2d 空间索引 容器
            grid.Init(gridHeight, gridWidth, fooCount);

            // 必须先预留足够多个数, 否则会导致数组变化, 指针失效
            objs.reserve(fooCount);

            // 直接构造出 n 个 Foo
            for (int i = 0; i < fooCount; i++) {
                objs.emplace_back(this, mapCenter);
            }
        }

        void Update() {
            count = 0;
            auto siz = objs.size();
            // 这里先简化，没有删除行为。否则就要记录到线程安全队列，事后批量删除
#pragma omp parallel for
            for (int i = 0; i < siz; ++i) {
                objs[i].Update(this);
            }
            for (int i = 0; i < siz; ++i) {
                objs[i].Update2(this);
            }
        }
    };

    inline Foo::Foo(Scene* const& scene, xx::XY const& xy) {
        this->xy = xy;
        gridIndex = scene->grid.Add(xy.y / gridDiameter, xy.x / gridDiameter, this);
    }

    inline void Foo::Update(Scene* const& scene) {
        // 提取坐标
        auto xy = this->xy;

        // 判断周围是否有别的 Foo 和自己重叠，取前 n 个，根据对方和自己的角度，结合距离，得到 推力矢量，求和 得到自己的前进方向和速度。
        // 最关键的是最终移动方向。速度需要限定最大值和最小值。如果算出来矢量为 0，那就随机角度正常速度移动。
        // 移动后的 xy 如果超出了 地图边缘，就硬调整. 

        // 查找 n 个邻居
        int limit = fooFindNeighborLimit;
        int crossNum = 0;
        xx::XY v;
        scene->grid.LimitFindNeighbor(limit, gridIndex, [&](auto o) {
            assert(o != (Item*)this);
            // 类型转换下方便使用
            auto& f = *(Foo*)o;
            // 如果圆心完全重叠，则不产生推力
            if (f.xy == this->xy) {
                ++crossNum;
                return;
            }
            // 准备判断是否有重叠. r1* r1 + r2 * r2 > (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y)
            auto r1 = f.radius;
            auto r2 = this->radius;
            auto r12 = r1 * r1 + r2 * r2;
            auto p1 = f.xy;
            auto p2 = this->xy;
            auto p12 = (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);
            // 有重叠
            if (r12 > p12) {
                // 计算 f 到 this 的角度( 对方产生的推力方向 ). 
                auto a = xx::GetAngle<(gridDiameter * 2 >= 1024)>(p1, p2);
                // 重叠的越多，推力越大?
                auto inc = xx::Rotate(xx::XY{ this->speed * r12 / p12, 0 }, a);
                v += inc;
                ++crossNum;
            }
            });

        if (crossNum) {
            // 如果 v == 0 那就随机角度正常速度移动
            if (v.IsZero()) {
                auto a = scene->rnd.Next() % xx::table_num_angles;
                auto inc = xx::Rotate(xx::XY{ speed, 0 }, a);
                xy += inc;
            }
            // 根据 v 移动
            else {
                auto a = xx::GetAngleXY(v.x, v.y);
                auto inc = xx::Rotate(xx::XY{ speed * std::max(crossNum, 5), 0 }, a);
                xy += inc;
            }

            // 边缘限定( 当前逻辑有重叠才移动，故边界检测放在这里 )
            if (xy.x < mapMinX) xy.x = mapMinX;
            else if (xy.x > mapMaxX) xy.x = mapMaxX;
            if (xy.y < mapMinY) xy.y = mapMinY;
            else if (xy.y > mapMaxY) xy.y = mapMaxY;
        }

        // 保存 xy ( update2 的时候应用 )
        xy2 = xy;
    }

    inline void Foo::Update2(Scene* const& scene) {
        // 变更检测与同步
        if (xy2 != xy) {
            ++scene->count;
            xy = xy2;
            assert(gridIndex != -1);
            scene->grid.Update(gridIndex, xy2.y / gridDiameter, xy2.x / gridDiameter);
        }
    }
}




#define SCREEN_WIDTH  1920
#define SCREEN_HEIGHT 1080
#define SCREEN_SCALE 0.1225f
#define BALL_SCALED_SIZE (0.1225f * 100.f)


GameLogic::Scene* scene = nullptr;
HGE* hge = nullptr;
hgeQuad quad;

bool frame_func() {
    // logic here
    scene->Update();

    // Continue execution
    return false;
}

bool render_func() {
    hge->Gfx_BeginScene();
    hge->Gfx_Clear(0);

    // for set quad v & render quad
    for (auto& f : scene->objs) {
        float x = f.xy.x * SCREEN_SCALE;
        float y = f.xy.y * SCREEN_SCALE;
        quad.v[0].x = x - BALL_SCALED_SIZE;
        quad.v[0].y = y - BALL_SCALED_SIZE;
        quad.v[1].x = x + BALL_SCALED_SIZE;
        quad.v[1].y = y - BALL_SCALED_SIZE;
        quad.v[2].x = x + BALL_SCALED_SIZE;
        quad.v[2].y = y + BALL_SCALED_SIZE;
        quad.v[3].x = x - BALL_SCALED_SIZE;
        quad.v[3].y = y + BALL_SCALED_SIZE;
        hge->Gfx_RenderQuad(&quad);
    }

    hge->Gfx_EndScene();

    // RenderFunc should always return false
    return false;
}


int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    // 拿一半的 cpu 跑物理( 对于超线程 cpu 来说这样挺好 )
    auto numThreads = std::thread::hardware_concurrency();
    omp_set_num_threads(numThreads / 2);
    omp_set_dynamic(0);

    // Get HGE interface
    hge = hgeCreate(HGE_VERSION);

    // Set up log file, frame function, render function and window title
    hge->System_SetState(HGE_LOGFILE, "hge_tut02.log");
    hge->System_SetState(HGE_FRAMEFUNC, frame_func);
    hge->System_SetState(HGE_RENDERFUNC, render_func);
    hge->System_SetState(HGE_TITLE, "HGE Tutorial 02 - test simple physics");

    // Set up video mode
    hge->System_SetState(HGE_USESOUND, false);
    hge->System_SetState(HGE_WINDOWED, true);
    hge->System_SetState(HGE_SCREENWIDTH, SCREEN_WIDTH);
    hge->System_SetState(HGE_SCREENHEIGHT, SCREEN_HEIGHT);
    hge->System_SetState(HGE_SCREENBPP, 32);
    hge->System_SetState(HGE_FPS, HGEFPS_UNLIMITED);
    //hge->System_SetState(HGE_FPS, HGEFPS_VSYNC);

    if (hge->System_Initiate()) {
        // Load texture
        quad.tex = hge->Texture_Load("b.png");
        if (!quad.tex) {
            MessageBox(nullptr, "Can't load b.png", "Error",
                MB_OK | MB_ICONERROR | MB_APPLMODAL);
            hge->System_Shutdown();
            hge->Release();
            return 0;
        }

        // Set up quad which we will use for rendering sprite
        quad.blend = hgeBlendMode(BLEND_COLORADD | BLEND_ALPHABLEND | BLEND_NOZWRITE);

        for (int i = 0; i < 4; i++) {
            // Set up z-coordinate of vertices
            quad.v[i].z = 0.5f;
            // Set up color. The format of DWORD col is 0xAARRGGBB
            //quad.v[i].col = 0xFFFFA000;
            quad.v[i].col = 0xFF000000;
        }

        // Set up quad's texture coordinates.
        // 0,0 means top left corner and 1,1 -
        // bottom right corner of the texture.
        quad.v[0].tx = 0.f;
        quad.v[0].ty = 0.f;
        quad.v[1].tx = 1.f;
        quad.v[1].ty = 0.f;
        quad.v[2].tx = 1.f;
        quad.v[2].ty = 1.f;
        quad.v[3].tx = 0.f;
        quad.v[3].ty = 1.f;

        // create game logic
        scene = new GameLogic::Scene(300000);

        // Let's rock now!
        hge->System_Start();

        // release game logic
        delete scene;

        // Free loaded texture
        hge->Texture_Free(quad.tex);
    }
    else MessageBox(nullptr, hge->System_GetErrorMessage(), "Error",
        MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);

    // Clean up and shutdown
    hge->System_Shutdown();
    hge->Release();
    return 0;
}
