﻿using TemplateLibrary;

// type id 号段: 1 ~ 9

[Include, TypeId(1)]
class A {
	int id;
	Nullable<string> nick;
	Weak<A> parent;
	List<Shared<A>> children;
};

[Include, TypeId(2)]
class B : A {
	byte[] data;
	C c;
	Nullable<C> c2;
	List<Nullable<C>> c3;
};

[Desc("asdfasdf")]
struct C {
	float x, y;
	List<Weak<A>> targets;
};

// todo: enum