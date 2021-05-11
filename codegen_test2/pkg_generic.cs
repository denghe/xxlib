using TemplateLibrary;

namespace Generic {
    [TypeId(1)]
    class Success {};

    [TypeId(2)]
    class Error {
        int errNumber;
        string errMessage;
    };
}
