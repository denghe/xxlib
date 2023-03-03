#pragma once
#include <random>
#include <xx_obj.h>

// 为 cpp 实现一个带 拨号 与 通信 的 udp/kcp 网关 协程 client, 基于帧逻辑, 无需继承, 开箱即用

namespace xx::Asio {

	// 收到的数据容器 T == Data: for script     T == xx::ObjBase_s for c++
	template<typename T>
	struct Package {
		XX_OBJ_STRUCT_H(Package);
		uint32_t serverId = 0;
		int32_t serial = 0;
		T data;
		Package(uint32_t const& serverId, int32_t const& serial, T&& data) : serverId(serverId), serial(serial), data(std::move(data)) {}
	};

	// 构造一个 uint32_le 数据长度( 不含自身 ) + uint32_le target + args 的 类 序列化包 并返回
	template<bool containTarget, bool containSerial, size_t cap, typename PKG = ObjBase, typename ... Args>
	Data MakeData(ObjManager& om, uint32_t const& target, int32_t const& serial, Args const&... args) {
		Data d;
		d.Reserve(cap);
		auto bak = d.WriteJump<false>(sizeof(uint32_t));
		if constexpr (containTarget) {
			d.WriteFixed<false>(target);
		}
		if constexpr (containSerial) {
			d.WriteVarInteger<false>(serial);
		}
		// 传统写包
		if constexpr (std::is_same_v<ObjBase, PKG>) {
			om.WriteTo(d, args...);
		}
		// 直写 cache buf 包
		else if constexpr (std::is_same_v<Span, PKG>) {
			d.WriteBufSpans(args...);
		}
		// 简单构造命令行包
		else if constexpr (std::is_same_v<std::string, PKG>) {
			d.Write(args...);
		}
		// 使用 目标类的静态函数 快速填充 buf
		else {
			PKG::WriteTo(d, args...);
		}
		d.WriteFixedAt(bak, (uint32_t)(d.len - 4));
		return d;
	}
	// 构造一个 uint32_le 数据长度( 不含自身 ) + args 的 类 序列化包 并返回
	template<size_t cap = 8192, typename PKG = ObjBase, typename ... Args>
	Data MakeTargetPackageData(ObjManager& om, uint32_t const& target, int32_t const& serial, Args const&... args) {
		return MakeData<true, true, cap, PKG>(om, target, serial, args...);
	}

	// 生成指定长度的随机码并返回
	inline Data GenSecret(int lenFrom = 5, int lenTo = 19) {
		std::mt19937 rng(std::random_device{}());
		std::uniform_int_distribution<int> dis1(lenFrom, lenTo);	// 4: old protocol
		std::uniform_int_distribution<int> dis2(0, 255);
		auto siz = dis1(rng);
		Data d;
		d.Resize(siz);
		for (size_t i = 0; i < siz; i++) {
			d[i] = dis2(rng);
		}
		return d;
	}

	template<typename PeerDeriveType, typename IOCType>
	struct TcpKcpRequestTargetCodes {

		int32_t reqAutoId = 0;
		std::unordered_map<int32_t, std::pair<asio::steady_timer, ObjBase_s>> reqs;
		ObjManager om;

		template<typename PKG = ObjBase, typename ... Args>
		void SendResponseTo(uint32_t const& target, int32_t const& serial, Args const& ... args) {
			PEERTHIS->Send(MakeTargetPackageData<8192, PKG>(om, target, serial, args...));
		}

		template<typename PKG = ObjBase, typename ... Args>
		void SendPushTo(uint32_t const& target, Args const& ... args) {
			PEERTHIS->Send(MakeTargetPackageData<8192, PKG>(om, target, 0, args...));
		}

		template<typename PKG = ObjBase, typename ... Args>
		awaitable<ObjBase_s> SendRequestTo(uint32_t const& target, std::chrono::steady_clock::duration d, Args const& ... args) {
			reqAutoId = (reqAutoId + 1) % 0x7FFFFFFF;
			auto key = reqAutoId;
			auto [iter, ok] = reqs.emplace(key, std::make_pair(asio::steady_timer(PEERTHIS->ioc, std::chrono::steady_clock::now() + d), ObjBase_s()));
			assert(ok);
			auto& [timer, data] = iter->second;
			PEERTHIS->Send(MakeTargetPackageData<8192, PKG>(om, target, -key, args...));
			auto [e] = co_await timer.async_wait(use_nothrow_awaitable);
			if (PEERTHIS->stoped || (e && (iter = reqs.find(key)) == reqs.end())) co_return nullptr;
			auto r = std::move(data);
			reqs.erase(iter);
			co_return r;
		}

		ObjBase_s ReadFrom(Data_r& dr) {
			ObjBase_s o;
			int r = om.ReadFrom(dr, o);
			if (r || (o && o.GetTypeId() == 0)) {
				om.KillRecursive(o);
				return nullptr;
			}
			return o;
		}

		int HandleData(Data_r&& dr) {
			// 读出 target
			uint32_t target;
			if (dr.ReadFixed(target)) return __LINE__;
			{
				int r = HandleTargetMessage(target, dr);
				if (r == 0) return 0;		// continue
				if (r < 0) return __LINE__;
			}

			// 读出序号
			int32_t serial;
			if (dr.Read(serial)) return __LINE__;

			// 如果是 Response 包，则在 req 字典查找。如果找到就 解包 + 传递 + 协程放行
			if (serial > 0) {
				if (auto iter = reqs.find(serial); iter != reqs.end()) {
					auto o = ReadFrom(dr);
					if (!o) return __LINE__;
					iter->second.second = std::move(o);
					iter->second.first.cancel();
				}
			} else {
				// 如果是 Push 包，且有提供 ReceivePush 处理函数，就 解包 + 传递
				if (serial == 0) {
					auto o = ReadFrom(dr);
					if (!o) return __LINE__;
					if (ReceiveTargetPush(target, std::move(o))) return __LINE__;
				}
				// 如果是 Request 包，且有提供 ReceiveRequest 处理函数，就 解包 + 传递
				else {
					auto o = ReadFrom(dr);
					if (!o) return __LINE__;
					if (ReceiveTargetRequest(target, -serial, std::move(o))) return __LINE__;
				}
				if (PEERTHIS->stoped) return __LINE__;
			}
			return 0;
		}

		// 拦截处理特殊 target 路由需求( 返回 0 表示已成功处理，   返回 正数 表示不拦截,    返回负数表示 出错 )
		int HandleTargetMessage(uint32_t target, Data_r& dr) {
			if (target == 0xFFFFFFFF) return ReceveCommand(dr);										// 收到 command. 转过去处理
			if (PEERTHIS->ioc.cppServerId == target) return 1; 												// 属于 cpp 的: passthrough
			int32_t serial;																			// 读出序号. 出错 返回负数行号( 掐线 )
			if (dr.Read(serial)) return -__LINE__;
			PEERTHIS->ioc.recvDatas.emplace_back(target, serial, Data(dr.buf + dr.offset, dr.len - dr.offset));	// 属于 lua: 放入 data 容器
			return 0;																				// 返回 已处理
		}
		int ReceveCommand(Data_r& dr) {
			std::string_view cmd;
			if (dr.Read(cmd)) return -__LINE__;
			if (cmd == "open"sv) {																	// 收到 打开服务 指令
				uint32_t serviceId = -1;
				if (dr.Read(serviceId)) return -__LINE__;

#if XX_ASIO_TCPKCP_DEBUG_INFO_OUTPUT
				std::cout << "\nrecv cmd: open " << serviceId << std::endl;
#endif

				PEERTHIS->ioc.openServerIds.insert(serviceId);										// 添加到 open列表
				if (!PEERTHIS->ioc.openWaitServerIds.empty()) {										// 判断是否正在 等open
					if (PEERTHIS->ioc.CheckOpens()) {												// 全 open 了: 清除参数 & cancel 等待器
						PEERTHIS->ioc.openWaiter.cancel();
					}
				}
			} else if (cmd == "close"sv) {															// 收到 关闭服务 指令
				uint32_t serviceId = -1;
				if (dr.Read(serviceId)) return -__LINE__;

#if XX_ASIO_TCPKCP_DEBUG_INFO_OUTPUT
				std::cout << "\nrecv cmd: close " << serviceId << std::endl;
#endif

				PEERTHIS->ioc.openServerIds.erase(serviceId);										// 从 open列表 移除
				if (PEERTHIS->ioc.openServerIds.empty()) return -__LINE__;							// 所有服务都关闭了：返回负数( 掐线 )
			}
			//else																					// 网关 echo 回来的东西 todo：计算到网关的 ping 值?
			return 0;																				// 返回 已处理
		}
		int ReceiveTargetRequest(uint32_t target, int32_t serial, ObjBase_s&& o_) {
			PEERTHIS->ioc.recvObjs[target].emplace_back(target, serial, std::move(o_));				// 放入 recvObjs
			if (PEERTHIS->ioc.waitingObject) {														// 如果发现正在等包
				PEERTHIS->ioc.waitingObject = false;												// 清理标志位
				PEERTHIS->ioc.objectWaiter.cancel();												// 放行
			}
			return 0;
		}
		int ReceiveTargetPush(uint32_t target, ObjBase_s&& o_) {
			return ReceiveTargetRequest(target, 0, std::move(o_));
		}
	};
}
