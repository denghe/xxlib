using TemplateLibrary;

namespace SS {

    struct XY {
        int x, y;
    }

    [TypeId(1), Include, Include_]
    class Scene {
        int frameNumber;
        Shared<Shooter> shooter;
    };

    [TypeId(2), Include, Include_]
    class Shooter {
        float bodyAngle;
        int moveDistancePerFrame = 10;
        XY pos;
        List<Shared<Bullet>> bullets;

        ControlState cs;
    };

    struct ControlState {
        XY aimPos;
        bool moveLeft;
        bool moveRight;
        bool moveUp;
        bool moveDown;
        bool button1;
        bool button2;
    };

    [TypeId(3), Include, Include_]
    class Bullet {
        int life;
        XY pos, inc;
    };

}

namespace SS_C2S {
    [TypeId(10)]
    class Cmd {
        SS.ControlState cs;
    };
}

namespace SS_S2C {
    [TypeId(20)]
    class Sync {
        Shared<SS.Scene> scene;
    };

    [TypeId(21)]
    class Event {
        int frameNumber;
        SS.ControlState cs;
    };
}
