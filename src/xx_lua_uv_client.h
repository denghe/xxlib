#pragma once

/*
xx::UvClient 主体映射到 lua

C++ 注册:
	xx::Lua::UvClient::Register( L )

LUA 全局函数:
	NewUv()

成员函数见 funcs 数组
*/

#include "xx_lua_bind.h"
#include "xx_uv_ext2.h"

namespace xx::Lua::UvClient {
	// 在 lua 中注册 全局的 Uv 创建函数
	inline void Register(lua_State* const& L) {
		SetGlobalCClosure(L, "NewUvClient", [](auto L)->int {
		    if(lua_gettop(L) > 0) {
                return PushUserdata<xx::UvClient>(L, To<int>(L, 1));
		    }
		    return PushUserdata<xx::UvClient>(L);
		});
	}
}

namespace xx::Lua {
	// 值方式 meta 但是访问成员时转为指针
	template<typename T>
	struct MetaFuncs<T, std::enable_if_t<std::is_same_v<xx::UvClient, std::decay_t<T>>>> {
		using U = xx::UvClient;
		inline static std::string name = std::string(TypeName_v<U>);
		static void Fill(lua_State* const& L) {
			SetType<U>(L);
			SetFieldCClosure(L, "Update", [](auto L)->int { To<U*>(L)->uv.Run(UV_RUN_NOWAIT); return 0; });
			SetFieldCClosure(L, "AddAddress", [](auto L)->int { To<U*>(L)->client->AddAddress(To<char*>(L, 2), To<int>(L, 3)); return 0; });
			SetFieldCClosure(L, "ClearAddresses", [](auto L)->int { To<U*>(L)->client->ClearAddresses(); return 0; });
			SetFieldCClosure(L, "Dial", [](auto L)->int { return Push(L, To<U*>(L)->client->Dial(lua_gettop(L) > 1 ? To<int>(L, 2) : 5000)); });
			SetFieldCClosure(L, "Busy", [](auto L)->int { auto c = To<U*>(L); return Push(L, c->client->Busy() || c->resolver->Busy()); });
			SetFieldCClosure(L, "Cancel", [](auto L)->int { auto c = To<U*>(L); c->client->Cancel(); c->resolver->Cancel(); return 0; });
			SetFieldCClosure(L, "Disconnect", [](auto L)->int { To<U*>(L)->client->PeerDispose(); return 0; });
			SetFieldCClosure(L, "Alive", [](auto L)->int { return Push(L, To<U*>(L)->client->PeerAlive()); });
			SetFieldCClosure(L, "IsKcp", [](auto L)->int { auto c = To<U*>(L)->client; if (c->PeerAlive()) return Push(L, c->peer->IsKcp()); return 0; });
			SetFieldCClosure(L, "IsOpened", [](auto L)->int { return Push(L, To<U*>(L)->client->IsOpened(To<int>(L, 2))); });
			SetFieldCClosure(L, "SendTo", [](auto L)->int { return Push(L, To<U*>(L)->client->SendTo(To<int>(L, 2), To<int>(L, 3), *To<xx::Data*>(L, 4))); });
			SetFieldCClosure(L, "SendEcho", [](auto L)->int { return Push(L, To<U*>(L)->client->SendEcho(*To<xx::Data*>(L, 2))); });
			SetFieldCClosure(L, "SetCppServiceId", [](auto L)->int { To<U*>(L)->client->SetCppServiceId(To<int>(L, 2)); return 0; });
			SetFieldCClosure(L, "TryGetPackage", [](auto L)->int { xx::Package pkg; if (To<U*>(L)->client->TryGetPackage(pkg)) return Push(L, pkg.serviceId, pkg.serial, std::move(pkg.data)); return 0; });
			SetFieldCClosure(L, "Resolve", [](auto L)->int { return Push(L, To<U*>(L)->resolver->Resolve(To<char*>(L, 2), lua_gettop(L) > 2 ? To<int>(L, 3) : 3000)); });
			SetFieldCClosure(L, "GetIPList", [](auto L)->int { return Push(L, To<U*>(L)->resolver->ips); });
		}
	};
	// 值方式 push
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<std::is_same_v<xx::UvClient, std::decay_t<T>>>> {
		static int Push(lua_State* const& L, T&& in) {
			return PushUserdata<xx::UvClient>(L, std::forward<T>(in));
		}
	};
	// 指针方式 to 但是做 值方式 检查
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<std::is_pointer_v<std::decay_t<T>>&& std::is_same_v<xx::UvClient, std::decay_t<std::remove_pointer_t<std::decay_t<T>>>>>> {
		static void To(lua_State* const& L, int const& idx, T& out) {
			AssertType<xx::UvClient>(L, idx);
			out = (T)lua_touserdata(L, idx);
		}
	};
}
