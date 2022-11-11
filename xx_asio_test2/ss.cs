using TemplateLibrary;

namespace SS {

    [Include]
    [Desc("坐标")]
    struct XY {
        int x, y;
    }

    [TypeId(10), Include]
    [Desc("场景")]
    class Scene {
        [Desc("帧编号")]
        int frameNumber;

        [Desc("射击者集合. key: clientId")]
        Dict<uint, Shared<Shooter>> shooters;
    };

    [TypeId(20), Include]
    [Desc("射击者")]
    class Shooter {
        [Desc("指向所在场景")]
        Weak<Scene> scene;

        [Desc("客户端编号( 连上服务器时自增分配 )")]
        uint clientId;

        [Desc("身体旋转角度( 并不影响逻辑 )")]
        int bodyAngle;

        [Desc("移动速度( 每帧移动距离 )")]
        int speed = 10;

        [Desc("中心点 锚点 位于 场景容器 坐标")]
        XY pos;

        [Desc("拥有的子弹集合")]
        List<Shared<Bullet>> bullets;

        [Desc("控制指令面板")]
        ControlState cs;
    };

    [Include]
    [Desc("射击者的控制指令面板")]
    struct ControlState {
        [Desc("鼠标坐标( 位于场景容器中的 )")]
        XY aimPos;

        [Desc("左移")]
        bool moveLeft;

        [Desc("右移")]
        bool moveRight;

        [Desc("上移")]
        bool moveUp;

        [Desc("下移")]
        bool moveDown;

        [Desc("按钮1")]
        bool button1;

        [Desc("按钮2")]
        bool button2;
    };

    [TypeId(30), Include]
    [Desc("射击者的子弹( 基类 )")]
    class Bullet {
        [Desc("指向发射者")]
        Weak<Shooter> shooter;

        [Desc("移动速度( 每帧移动距离 )")]
        int speed = 60;

        [Desc("剩余生命周期( 多少帧后消亡 )")]
        int life;

        [Desc("中心点 锚点 位于 场景容器 坐标")]
        XY pos;
    };

    [TypeId(31), Include]
    [Desc("射击者的子弹--直线飞行")]
    class Bullet_Straight : Bullet {
        [Desc("每帧的 移动增量( 直线飞行专用, 已预乘 speed )")]
        XY inc;
    };

    [TypeId(32), Include]
    [Desc("射击者的子弹--跟踪")]
    class Bullet_Track : Bullet {
        [Desc("跟踪目标")]
        Weak<Shooter> target;
    };

}


// client -> server
namespace SS_C2S {

    [TypeId(40)]
    [Desc("请求进入游戏")]
    class Enter {
        // todo: login info?
    };

    [TypeId(41)]
    [Desc("控制指令")]
    class Cmd {
        SS.ControlState cs;
    };
}


// server -> client
namespace SS_S2C {

    [TypeId(50)]
    [Desc("进入结果( 这应该是客户端发 Enter 之后，收到的 首包 )")]
    class EnterResult {
        [Desc("客户端编号, 存下来用于在 Sync.scene.shooters 中查找自身")]
        uint clientId;
    };

    [TypeId(51)]
    [Desc("完整同步")]
    class Sync {
        [Desc("整个场景数据( 后于 EnterResult 收到，但先于所有 Event )")]
        Shared<SS.Scene> scene;
    };

    [TypeId(52)]
    [Desc("事件通知")]
    class Event {
        [Desc("对应的帧编号( 客户端收到后，如果慢于它就需要快进，快于它就需要回滚 )")]
        int frameNumber;

        [Desc("所有退出的玩家id列表. 客户端收到后遍历并从 shooters 中移除相应对象( 优先级:1 )")]
        List<uint> quits;

        [Desc("所有进入的玩家的数据. 客户端收到后遍历挪进 shooters 并初始化( 优先级:2 )")]
        List<Shared<SS.Shooter>> enters;

        [Desc("所有发生控制行为的玩家的 控制指令. uint: 用来定位 shooter 的 clientId")]
        List<Tuple<uint, SS.ControlState>> css;
    };
}
