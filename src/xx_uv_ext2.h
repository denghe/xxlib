#pragma once
#include "xx_uv.h"
#include <unordered_set>
namespace xx {
	// 包容器( 不封装到 lua )
	struct Package {
		Package() = default;
		Package(Package const&) = default;
		Package& operator=(Package const&) = default;
		Package(Package&& o) = default;
		Package& operator=(Package&& o) = default;

		uint32_t serviceId = 0;
		int serial = 0;
		Data data;

		Package(uint32_t const& serviceId, int const& serial, Data&& data)
			: serviceId(serviceId), serial(serial), data(std::move(data)) {}
	};

	// 连上 gateway 后产生的物理 peer( 不封装到 lua )
	struct UvToGatewayPeer : UvPeer {
		UvToGatewayPeer(xx::Uv& uv) : UvPeer(uv) {}

		// 存放已收到的包
		std::deque<Package> receivedPackages;
		// 存放已收到的包( for cpp )
		std::deque<Package> receivedCppPackages;

		// 已开放的 serviceId 列表( 发送时如果 目标id 不在列表里则忽略发送但不报错? 或是返回 操作失败? )
		std::unordered_set<uint32_t> openServerIds;

		// 检查某 serviceId 是否已 open
		inline bool IsOpened(uint32_t const& serviceId) const {
			return std::find(openServerIds.begin(), openServerIds.end(), serviceId) != openServerIds.end();
		}

		// 向某 serviceId 发数据( for C++ )
		inline int SendTo(uint32_t const& id, int32_t const& serial, ObjBase_s const& msg) {
			if (!peerBase) return -1;
			auto&& bb = uv.sendBB;
			peerBase->SendPrepare(bb, 1024);
			bb.WriteFixed(id);
			bb.WriteVarInteger(serial);
			uv.om.WriteTo(bb, msg);
			return peerBase->SendAfterPrepare(bb);
		}

		// 向某 serviceId 发数据( for lua )
		inline int SendTo(uint32_t const& id, int32_t const& serial, Data const& data) {
			if (!peerBase) return -1;
			auto&& bb = uv.sendBB;
			peerBase->SendPrepare(bb, 1024);
			bb.WriteFixed(id);
			bb.WriteVarInteger(serial);
			bb.WriteBuf(data.buf, data.len);
			return peerBase->SendAfterPrepare(bb);
		}

	protected:
		inline virtual int HandlePack(uint8_t* const& buf, uint32_t const& len) noexcept override {
			auto& bb = uv.recvBB;
			bb.Reset(buf, len);

			// 最上层结构：4字节长度 + 4字节服务id + 其他
			uint32_t serviceId = 0;
			if (bb.ReadFixed(serviceId)) return __LINE__;
			// 服务id 如果为 0xFFFFFFFFu 则为内部指令
			if (serviceId == 0xFFFFFFFFu) {
				// 其他结构：string cmd + args
				std::string cmd;
				if (bb.Read(cmd, serviceId)) return __LINE__;
				if (cmd == "open") {
					openServerIds.insert(serviceId);
				}
				else if (cmd == "close") {
					openServerIds.erase(serviceId);
				}
				else {
					return __LINE__;
				}
			}
			else {
				// 其他结构：int serial + PKG serial data
				int serial = 0;
				if(bb.Read(serial)) return __LINE__;
				// 剩余数据打包为 Data 塞到收包队列
				Data data;
				data.WriteBuf(buf + bb.offset, len - bb.offset);
				receivedPackages.emplace_back(serviceId, serial, std::move(data));
			}
			return 0;
		}
	};

	// client 连 gateways 专用。只有底层代码. 上层做 push request response 模拟( 封装到 lua )
	struct UvToGatewayDialerAndPeer : UvDialer {
		using UvDialer::UvDialer;

		// 物理链路 peer( Accept 之后存到这 )
		Shared<UvToGatewayPeer> peer;

		// 地址簿
		std::vector<std::pair<std::string, int>> addrs;

		inline virtual UvPeer_s CreatePeer() noexcept override {
			return TryMakeShared<UvToGatewayPeer>(uv);
		}

		inline virtual void Accept(UvPeer_s peer_) noexcept override {
			if (!peer_) return;
			PeerDispose();
			peer = As<UvToGatewayPeer>(peer_);
		}

		// 下面函数主要是为了 LUA 里封装方便. 一个拨号器 all in one
		// LUA 还要映射 Busy, Cancel, IsKcp, GetIP

		// 添加 ip & port 到地址簿
		inline void AddAddress(std::string const& ip, int const& port) {
			addrs.emplace_back(ip, port);
		}

		// 清空地址簿
		inline void ClearAddresses() {
			addrs.clear();
		}

		// 开始拨号
		inline int Dial(uint64_t const& timeoutMS = 5000) {
			return this->UvDialer::Dial(addrs, timeoutMS);
		}

		// 拨号后 while 等 !Busy() 之后判断是否 PeerAlive
		inline bool PeerAlive() const {
			return peer && !peer->Disposed();
		}

		// 直接干掉 peer
		inline void PeerDispose() {
			if (peer) {
				peer->Dispose();
				peer.Reset();
			}
		}

		// 检查某 serviceId 是否已 open
		inline bool IsOpened(uint32_t const& serviceId) const {
			if (!PeerAlive()) return false;
			return peer->IsOpened(serviceId);
		}

		// 向某 serviceId 发数据
		inline int SendTo(uint32_t const& id, int32_t const& serial, Data const& data) {
			if (!PeerAlive()) return -1;
			return peer->SendTo(id, serial, data);
		}

		// 尝试 move 出一条最前面的消息( lua 那边则将 Package 变为 table, 以成员方式压栈. 之后分发 )
		inline bool TryGetPackage(Package& pkg) {
			if (!PeerAlive()) return false;
			auto&& ps = peer->receivedPackages;
		LabLoop:
			if (ps.empty()) return false;
			// 遇到需要在 cpp 中处理的包就移走继续判断下一个
			if (ps.front().serviceId == cppServiceId) {
				peer->receivedCppPackages.emplace_back(std::move(ps.front()));
				ps.pop_front();
				goto LabLoop;
			}
			pkg = std::move(ps.front());
			ps.pop_front();
			return true;
		}

		// 下面代码 for CPP 处理消息

		// 存放 cpp 代码处理的 serviceId. 当 lua TryGetPackage 时, 判断如果 serviceId 相符，就移动到 receivedCppPackages
		uint32_t cppServiceId = -1;

		// 设置 cpp 代码处理的 serviceId( 映射到 lua )
		inline void SetCppServiceId(uint32_t const& cppServiceId) {
			this->cppServiceId = cppServiceId;
		}

		// 尝试 move 出一条最前面的消息( for cpp 调用 )
		inline bool TryGetCppPackage(Package& pkg) {
			if (!PeerAlive()) return false;
			auto&& ps = peer->receivedCppPackages;
			if (ps.empty()) return false;
			pkg = std::move(ps.front());
			ps.pop_front();
			return true;
		}
	};

	// 创建到 lua 中的类实例( lua 中最好只创建一份, 全局使用 )
	struct UvClient {
		Uv uv;
		Shared<UvToGatewayDialerAndPeer> client;
		Shared<UvResolver> resolver;
		UvClient() {
			client.Emplace(uv);
			resolver.Emplace(uv);
		}
		UvClient(UvClient const&) = delete;
		UvClient& operator=(UvClient const&) = delete;
	};
}
