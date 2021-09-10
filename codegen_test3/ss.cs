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
    [Desc("完整同步")]
    class Sync {
        Shared<SS.Scene> scene;
    };

    [TypeId(51)]
    [Desc("事件通知( 基类 )")]
    class Event {
        int frameNumber;
    };

    [TypeId(52)]
    [Desc("玩家进入事件")]
    class Event_Enter : Event {
        Shared<SS.Shooter> shooter;
    };

    [TypeId(53)]
    [Desc("玩家控制事件")]
    class Event_Cmd : Event {
        SS.ControlState cs;
    };
}
