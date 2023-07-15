#pragma once
#include <iostream>
#include "xx_lua_bind.h"

// 示例1: 以 C++ 类为主体, 生命周期由 C++ 控制, 虚函数 call 到 lua function, 映射到 lua 的只是裸指针

// 基类
struct FooBase {
	int n = 123;
	std::string name = "Foo";
	virtual int Update() = 0;
	virtual ~FooBase() = default;
};

// 派生类
struct Foo : FooBase {
	xx::Lua::Func onUpdate;
	int Update() override;
	Foo(lua_State* const& L);
};

namespace xx::Lua {
	// 基类 指针方式 meta
	template<>
	struct MetaFuncs<FooBase*, void> {
		using U = FooBase*;
		inline static std::string name = TypeName<U>();
		static void Fill(lua_State* const& L) {
			SetType<U>(L);
			SetFieldCClosure(L, "n", [](auto L)->int { return Push(L, To<U>(L)->n); });
			SetFieldCClosure(L, "set_n", [](auto L)->int { To(L, 2, To<U>(L)->n); return 0; });
			SetFieldCClosure(L, "name", [](auto L)->int { return Push(L, To<U>(L)->name); });
			SetFieldCClosure(L, "set_name", [](auto L)->int { To(L, 2, To<U>(L)->name); return 0; });
		}
	};
	// 派生类 指针方式 meta
	template<>
	struct MetaFuncs<Foo*, void> {
		using U = Foo*;
		inline static std::string name = TypeName<U>();
		static void Fill(lua_State* const& L) {
			MetaFuncs<FooBase*>::Fill(L);
			SetType<U>(L);
			SetFieldCClosure(L, "onUpdate", [](auto L)->int { return Push(L, To<U>(L)->onUpdate); });
			SetFieldCClosure(L, "set_onUpdate", [](auto L)->int { To(L, 2, To<U>(L)->onUpdate); return 0; });
		}
	};
	// 指针方式 push + to
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<std::is_pointer_v<std::decay_t<T>>&& std::is_base_of_v<FooBase, std::remove_pointer_t<std::decay_t<T>>>>> {
		using U = std::decay_t<T>;
		static constexpr int checkStackSize = 1;
		static int Push_(lua_State* const& L, T&& in) {
			return PushUserdata<U>(L, in);
		}
		static void To_(lua_State* const& L, int const& idx, T& out) {
			AssertType<U>(L, idx);
			out = *(U*)lua_touserdata(L, idx);
		}
	};
}

// test4.lua 文件内容：
/*
local this = ...
this:set_onUpdate(function(delta)
	print("lua print:", delta)
	this:set_n(12)
	this:set_name("asdf")
	return 0
end)
*/
Foo::Foo(lua_State* const& L) {
	xx::Lua::DoFile(L, "test4.lua", this);
}

int Foo::Update() {
	return onUpdate.Call<int>(0.016f);
}

void TestLuaBind1() {
	xx::Lua::State L;
	Foo foo(L);
	//xx::Lua::SetGlobal(L, "foo", &foo);
	int r = foo.Update();
	xx::CoutN("n = ", foo.n, " name = ", foo.name, " r = ", r);
}


// 示例2: 以 lua 为主体, 映射到 lua 的是 userdata + placement new 的类实例

struct Barrrrrrrrrrrrrr {
	bool b = false;
	void SetTrue() { b = true; }
	void SetFalse() { b = false; }
	bool IsTrue() { return b; }
};

namespace xx::Lua {
	// 值方式 meta 但是访问成员时转为指针
	template<typename T>
	struct MetaFuncs<T, std::enable_if_t<std::is_same_v<Barrrrrrrrrrrrrr, std::decay_t<T>>>> {
		using U = Barrrrrrrrrrrrrr;
		inline static std::string name = std::string(TypeName_v<U>);
		static void Fill(lua_State* const& L) {
			SetType<U>(L);
			SetFieldCClosure(L, "SetTrue", [](auto L)->int { To<U*>(L)->SetTrue(); return 0; });
			SetFieldCClosure(L, "SetFalse", [](auto L)->int { To<U*>(L)->SetFalse(); return 0; });
			SetFieldCClosure(L, "IsTrue", [](auto L)->int { return Push(L, To<U*>(L)->IsTrue()); });
		}
	};
	// 值方式 push
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<std::is_same_v<Barrrrrrrrrrrrrr, std::decay_t<T>>>> {
		static constexpr int checkStackSize = 1;
		static int Push_(lua_State* const& L, T&& in) {
			return PushUserdata<Barrrrrrrrrrrrrr>(L, std::forward<T>(in));
		}
	};
	// 指针方式 to 但是做 值方式 检查
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<std::is_pointer_v<std::decay_t<T>>&& std::is_same_v<Barrrrrrrrrrrrrr
		, std::decay_t<std::remove_pointer_t<std::decay_t<T>>>>>> {
		static void To_(lua_State* const& L, int const& idx, T& out) {
			AssertType<Barrrrrrrrrrrrrr>(L, idx);
			out = (T)lua_touserdata(L, idx);
		}
	};
}

void TestLuaBind2() {
	xx::Lua::State L;
	xx::Lua::SetGlobalCClosure(L, "NewBar", [](auto L)->int { xx::Lua::PushUserdata<Barrrrrrrrrrrrrr>(L); return 1; });
	xx::Lua::DoString(L, R"--(
nb = NewBar()
print(nb:IsTrue())
nb:SetTrue()
print(nb:IsTrue())
)--");
	auto nb = xx::Lua::GetGlobal<Barrrrrrrrrrrrrr*>(L, "nb");
	xx::CoutN(nb->b);
}









// 老代码备份
// 
//namespace xx::Lua {
	///******************************************************************************************************************/
	//// 元表辅助填充类
	//// C 传入 MetaFuncs 的 T
	//template<typename C, typename B = void>
	//struct Meta {
	//protected:
	//	lua_State* const& L;
	//public:
	//	explicit Meta(lua_State* const& L, std::string const& name) : L(L) {
	//		if constexpr (!std::is_void_v<B>) {
	//			MetaFuncs<B, void>::Fill(L);
	//		}
	//		SetField(L, (void*)name.data(), 1);
	//	}
	//	template<typename F>
	//	Meta& Func(char const* const& name, F const& f) {
	//		lua_pushstring(L, name);
	//		new(lua_newuserdata(L, sizeof(F))) F(f);                    // ..., ud
	//		lua_pushcclosure(L, [](auto L) {                            // ..., cc
	//			C* p = nullptr;
	//			PushToFuncs<C, void>::ToPtr(L, 1, p);
	//			auto&& self = ToPointer(*p);
	//			if (!self) Error(L, std::string("args[1] is nullptr?"));
	//			auto&& f = *(F*)lua_touserdata(L, lua_upvalueindex(1));
	//			FuncA_t<F> tuple;
	//			To(L, 2, tuple);
	//			int rtv = 0;
	//			std::apply([&](auto &... args) {
	//				if constexpr (std::is_void_v<FuncR_t<F>>) {
	//					((*self).*f)(std::move(args)...);
	//				}
	//				else {
	//					rtv = xx::Lua::Push(L, ((*self).*f)(std::move(args)...));
	//				}
	//				}, tuple);
	//			return rtv;
	//			}, 1);
	//		lua_rawset(L, -3);
	//		return *this;
	//	}
	//	template<typename P>
	//	Meta& Prop(char const* const& getName, P const& o) {
	//		SetField(L, (char*)getName, [o](C& self) {
	//			return (*ToPointer(self)).*o;
	//			});
	//		return *this;
	//	}
	//	template<typename P>
	//	Meta& Prop(char const* const& getName, char const* const& setName, P const& o) {
	//		if (getName) {
	//			Prop<P>(getName, o);
	//		}
	//		if (setName) {
	//			SetField(L, (char*)setName, [o](C& self, MemberPointerR_t<P> const& v) {
	//				(*ToPointer(self)).*o = v;
	//				});
	//		}
	//		return *this;
	//	}
	//	// lambda 第一个参数为 C& 类型，接受 self 传入
	//	template<typename F>
	//	Meta& Lambda(char const* const& name, F&& f) {
	//		lua_pushstring(L, name);
	//		Push(L, std::forward<F>(f));
	//		lua_rawset(L, -3);
	//		return *this;
	//	}
	//};
//}
