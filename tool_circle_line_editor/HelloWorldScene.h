#pragma once
#include "cocos2d.h"
#include <filesystem>
#include "xx_helpers.h"

struct Circle;
struct DraggingRes;

#include "ajson.hpp"

// json 存档格式. 在 .lines 中，r 半径值没意义. 当前为简化，都存储为统一格式
struct Item {
    XX_SIMPLE_STRUCT_DEFAULT_CODES(Item);
    float x, y, r;
};
AJSON(Item, x, y, r);
struct Data {
    XX_SIMPLE_STRUCT_DEFAULT_CODES(Data);
    std::vector<Item> items;
};
AJSON(Data, items);

enum class PicTypes {
    SpriteFrame, Spine, C3b
};

// 图节点信息
struct PicInfo {
    XX_SIMPLE_STRUCT_DEFAULT_CODES(PicInfo);

    // 所在容器数组
    std::vector<PicInfo*>* container = nullptr;

    // 指向上一个，下一个 ( 第一个的上一个指向最后一个, 最后一个的下一个指向第一个 )
    PicInfo* prev = nullptr, *next = nullptr;

    // 位于数组的下标值
    int idx = -1;

    bool IsFirst() const { return idx == 0; }
    bool IsLast() const { return idx + 1 == container->size(); }

    // plist: sprite frame name
    // spine / c3b: fileName + "_" + actionName + "_" + idx
    std::string name;

    // 0: sprite frame     1: spine     2: c3b
    PicTypes type = PicTypes::SpriteFrame;

    // 文件名（带扩展名）
    std::string fileName;
    std::string fileName2;
    // 动作名
    std::string actionName;
    // 起始时间点
    float timePoint = 0.f;
};

#define SAVE_FILE_EXT_CIRCLES ".circles"
#define SAVE_FILE_EXT_LINES ".lines"

struct HelloWorld : cocos2d::Scene {
    using BaseType = cocos2d::Scene;
    using BaseType::BaseType;
    bool init() override;
    void onEnter() override;
    void onExit() override;
    void onDrawImGui();

    // 程序启动时加载是否成功
    bool ok = false;

    // ok == false 报错文字
    std::string errorMessage;

    // 程序启动时填充 pics & picGroups。从 Resources 目录找 *.plist, *.atlas, *.c3b
    // 
    // first: sprite frame name       second: plist file name
    std::vector<PicInfo> allPics;

    // key: 过滤掉数字部分的精灵帧名
    std::map<std::string, std::vector<PicInfo*>> picGroups;

    // 当前数据存档扩展名（ 在 .lines 之间切换 )
    std::string currExt = SAVE_FILE_EXT_CIRCLES;

    // 当前选中的图
    PicInfo* currPic;

    // 当前选中的图的圆数据
    Data currData;


    // 缓存 AppDelegate::designWidth/Height
    float W = 0, H = 0, W_2 = 0, H_2 = 0;

    // 设计尺寸相关
    float DW = 1280, DH = 720, DW_2 = DW / 2, DH_2 = DH / 2;

    // touch 状态记录
    cocos2d::Vec2 lastPos;
    bool touching = false;
    Circle* currCircle = nullptr;

    // 图绘制区域 缩放值
    float zoom = 1.0f;

    // 指向图绘制区域容器
    cocos2d::Node* nodeCircles = nullptr;

    // 当前用来做播放
    void update(float delta) override;

    // 是否正在播放
    bool playing = false;
    // 当前播放帧延迟
    float playingFrameDelay = 1.f / 60.f;
    // 时间池( 稳帧用 )
    float playingTimePool = 0;
    // 播放前备份
    PicInfo* bak_currPic = nullptr;

    // 创建一个圆节点到 nodeCircles
    Circle* CreateCircle(cocos2d::Vec2 const& pos, float const& r);

    // 从 nodeCircles 获取 Circle* 数组
    std::vector<Circle*> GetCircles();

    // 播放
    void Play(float const& delaySecs);

    // 根据 currPic 显示编辑界面
    void Draw();

    // 加载数据到 pics. 返回是否 ok
    bool LoadPics();

    // 加载数据到 currData. 成功 0
    int Load();

    // 存储 currData 为 currPic + ".circle" 文件名
    void Save();

    // 上移图焦点
    void MoveUp();

    // 下移图焦点
    void MoveDown();

    // 从上一张图复制圆
    void CopyPrevCircles();

    // 清除当前图的圆
    void Clear();

    // 创建 2d 帧图( 锚点 中心 ). 显示为指定 siz 大小
    cocos2d::Sprite* CreateSpriteFrame(cocos2d::Vec2 const& pos, cocos2d::Size const& siz, std::string const& spriteFrameName, cocos2d::Node* const& container = nullptr);
};
