using TemplateLibrary;

struct C {
	float x, y;
	List<Weak<A>> targets;
};

[Include, TypeId(2)]
class B : A {
	byte[] data;
	C c;
	Nullable<C> c2;
	List<List<Nullable<C>>> c3;
};

[Include, TypeId(1)]
class A {
	int id;
	Nullable<string> nick;
	Weak<A> parent;
	List<Shared<A>> children;
};
