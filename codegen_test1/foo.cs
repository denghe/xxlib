using TemplateLibrary;

[TypeId(16)]
class foo {
    int id;
    string name;
};

[Struct]
class foo2 {
    int id;
    string name;
};

[TypeId(22)]
class bar : foo {
};
