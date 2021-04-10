using TemplateLibrary;

[TypeId(16)]
class foo {
    int id;
    string name;
};

[Struct]
class foo2 {
    int id;
    byte[] d;
};

[TypeId(22)]
class bar : foo {
};

[TypeId(30)]
class FishBase {
    int cfgId;
    int id;
    float x, y;
    float a, r;
    long coin;
};
[TypeId(31)]
class FishWithChilds : FishBase {
    List<Shared<FishBase>> childs;
};
