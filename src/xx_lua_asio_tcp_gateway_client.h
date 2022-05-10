#pragma once
#include <xx_asio_tcp_gateway_client.h>
#include "xx_lua_bind.h"

/*
xx::Asio::Tcp::Gateway::Client 映射到 lua

C++ 注册:
	xx::Lua::Asio::Tcp::Gateway::Client::Register( L )

LUA 中的 全局 创建 函数:
	NewAsioTcpGatewayClient()

成员函数见 funcs 数组
*/

//namespace xx::Asio::Tcp::Gateway::Lua::Client {
//	// 在 lua 中注册 全局 创建 函数
//	inline void Register(lua_State* const& L) {
//		SetGlobalCClosure(L, "NewAsioTcpGatewayLuaClient", [](auto L)->int {
//			if (lua_gettop(L) > 0) {
//				return PushUserdata<xx::UvClient>(L, To<int>(L, 1));
//			}
//			return PushUserdata<xx::UvClient>(L);
//		});
//	}
//}
//namespace xx::Asio::Tcp::Gateway::Lua {
//}
