#pragma once
#include <xx_asio_tcp_gateway_client.h>
#include "xx_lua_data.h"

/*
xx::Asio::Tcp::Gateway::Client 映射到 lua

C++ 注册:
	xx::Lua::Asio::Tcp::Gateway::Client::Register( L );

LUA 中的 全局 创建 函数: ( 默认 )
	NewAsioTcpGatewayClient()

成员函数见 下方 SetFieldCClosure 代码段

注意事项：
	Dial() 是协程函数。调用后需要不断的用 Busy() 来探测状态，如果没脱离 busy 则不可以 call 别的函数
*/

namespace xx::Lua::Asio::Tcp::Gateway::Client {
	// 在 lua 中注册 全局 创建 函数
	inline void Register(lua_State* const& L, char const* keyName = "NewAsioTcpGatewayClient") {
		SetGlobalCClosure(L, keyName, [](auto L)->int {
			return PushUserdata<xx::Asio::Tcp::Gateway::Client>(L);
		});
	}
}

namespace xx::Lua {
	// 值方式 meta 但是访问成员时转为指针
	template<typename T>
	struct MetaFuncs<T, std::enable_if_t<std::is_same_v<xx::Asio::Tcp::Gateway::Client, std::decay_t<T>>>> {
		using U = xx::Asio::Tcp::Gateway::Client;
		inline static std::string name = std::string(TypeName_v<U>);
		static void Fill(lua_State* const& L) {
			SetType<U>(L);
			SetFieldCClosure(L, "Update", [](auto L)->int {
				To<U*>(L)->Update();
				return 0; 
			});
			SetFieldCClosure(L, "Reset", [](auto L)->int {
				To<U*>(L)->Reset();
				return 0; 
			});
			SetFieldCClosure(L, "SetSecret", [](auto L)->int {
				To<U*>(L)->SetSecret(To<std::string_view>(L, 2));
				return 0;
			});
			SetFieldCClosure(L, "SetDomainPort", [](auto L)->int {
				To<U*>(L)->SetDomainPort(To<std::string_view>(L, 2), To<int>(L, 3));
				return 0;
			});
			SetFieldCClosure(L, "SetCppServerId", [](auto L)->int {
				for (int i = lua_gettop(L); i > 1; --i) {
					To<U*>(L)->SetCppServerId(To<int>(L, i));
				}
				return 0;
			});
			SetFieldCClosure(L, "Dial", [](auto L)->int {
				auto c = To<U*>(L);
				if (c->busy) Error(L, "Client is busy now !");
				c->busy = true;
				co_spawn(c->ioc, [c]()->awaitable<void> {
					co_await c->Dial();
					c->busy = false;
				}, detached);
				return 0;
			});
			SetFieldCClosure(L, "Busy", [](auto L)->int {
				return Push(L, To<U*>(L)->busy);
			});
			SetFieldCClosure(L, "Alive", [](auto L)->int {
				return Push(L, !!*To<U*>(L));
			});
			SetFieldCClosure(L, "IsOpened", [](auto L)->int {
				return Push(L, To<U*>(L)->IsOpened(To<int>(L, 2)));
			});
			SetFieldCClosure(L, "SendTo", [](auto L)->int {
				To<U*>(L)->SendTo(To<int>(L, 2), To<int>(L, 3), *To<xx::Data*>(L, 4));
				return 0;
			});
			SetFieldCClosure(L, "Send", [](auto L)->int {
				To<U*>(L)->Send(std::move(*To<xx::Data*>(L, 2)));
				return 0;
			});
			SetFieldCClosure(L, "TryPop", [](auto L)->int {
				xx::Asio::Tcp::Gateway::Package<xx::Data> pkg;
				return To<U*>(L)->TryPop(pkg) ? Push(L, pkg.serverId, pkg.serial, std::move(pkg.data)) : 0;
			});
		}
	};
	// 值方式 push
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<std::is_same_v<xx::Asio::Tcp::Gateway::Client, std::decay_t<T>>>> {
		static constexpr int checkStackSize = 1;
		static int Push_(lua_State* const& L, T&& in) {
			return PushUserdata<xx::Asio::Tcp::Gateway::Client>(L, std::forward<T>(in));
		}
	};
	// 指针方式 to 但是做 值方式 检查
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<std::is_pointer_v<std::decay_t<T>>&& std::is_same_v<xx::Asio::Tcp::Gateway::Client, std::decay_t<std::remove_pointer_t<std::decay_t<T>>>>>> {
		static void To_(lua_State* const& L, int const& idx, T& out) {
			AssertType<xx::Asio::Tcp::Gateway::Client>(L, idx);
			out = (T)lua_touserdata(L, idx);
		}
	};
}
