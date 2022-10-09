#pragma once
#include <xx_asio_ioc.h>

namespace xx {

	// 各种成员函数是否存在的检测模板
	template<class T, class = void> struct _Has_FillSendDataHeader : std::false_type {};
	template<class T> struct _Has_FillSendDataHeader<T, std::void_t<decltype(std::declval<T&>().FillSendDataHeader(std::declval<xx::Data&>()))>> : std::true_type {};
	template<class T> constexpr bool Has_FillSendDataHeader = _Has_FillSendDataHeader<T>::value;

	// 通过多重继承，附加 Send Request Response Push 等基础代码
	template<typename PeerDeriveType, size_t headerReserveLen = 16>
	struct PeerRequestCode {
		int32_t reqAutoId = 0;
		std::unordered_map<int32_t, std::pair<asio::steady_timer, Data>> reqs;

		// 现有包结构：数据长:fixed uint32_t [ + 接收目标:fixed uint32_t ] + 序列号:int32 + 数据
		// 遇到广播的需求可以用这个函数来得到 DataShared 发送以减少序列化和 copy 次数
		template<typename F>
		DataShared MakeSendData(int32_t const& serial, size_t const& reserveLen, F&& dataFiller) {
			Data d;
			d.Reserve(headerReserveLen + reserveLen);									// 预分配内存( 头 + 数据 )
			auto headerOffset = d.WriteJump<false>(sizeof(uint32_t));					// 跳过 定长 长度头 并记录 偏移
			if constexpr (Has_FillSendDataHeader<PeerDeriveType>) {						// 如果派生类 有 FillSendDataHeader 这个函数，就 call, 对 data 的 header 部分进一步填充
				PEERTHIS->FillSendDataHeader(d);
			}
			d.WriteVarInteger<false>(serial);											// 填充 变长 序列号
			dataFiller(d);																// 填充 数据
			d.WriteFixedAt(headerOffset, (uint32_t)(d.len - sizeof(uint32_t)));			// 在 长度头偏移处 写入 不算长度头的包长
			return DataShared(std::move(d));
		}

		template<typename F>
		void SendResponse(int32_t const& serial, size_t const& reserveLen, F&& dataFiller) {
			if (PEERTHIS->IsStoped()) return;
			PEERTHIS->Send(MakeSendData(serial, reserveLen, std::forward<F>(dataFiller)));
		}

		template<typename F>
		void SendPush(size_t const& reserveLen, F&& dataFiller) {
			if (PEERTHIS->IsStoped()) return;
			PEERTHIS->Send(MakeSendData(0, reserveLen, std::forward<F>(dataFiller)));
		}

		template<typename F>
		awaitable<Data> SendRequest(std::chrono::steady_clock::duration d, size_t const& reserveLen, F&& dataFiller) {
			if (PEERTHIS->IsStoped()) co_return Data();

			// 正整数自增产生 序号key 备用
			reqAutoId = (reqAutoId + 1) % 0x7FFFFFFF;
			auto key = reqAutoId;

			// 将 序号key, 协程阻塞器, 数据接收容器 放入字典
			auto [iter, ok] = reqs.emplace(key, std::make_pair(asio::steady_timer(PEERTHIS->ioc, std::chrono::steady_clock::now() + d), Data()));
			assert(ok);
			auto& [timer, data] = iter->second;

			// 发出数据 并等 Read 协程那边读到后 将数据 填入容器 并 cancel 阻塞器
			PEERTHIS->Send(MakeSendData(-key, reserveLen, std::forward<F>(dataFiller)));
			auto [e] = co_await timer.async_wait(use_nothrow_awaitable);

			// 如果 已断开 或 cancel 后 key 已不存在, 短路返回( 字典.clear 行为会导致这种情况的发生 )
			if (PEERTHIS->stoped || (e && (iter = reqs.find(key)) == reqs.end())) co_return Data();

			// 将数据挪出返回，删除相关字典项
			auto r = std::move(data);
			reqs.erase(iter);
			co_return r;
		}

		// 被基类模板调用
		int HandleMessage(Data_r& d) {
			int32_t serial;
			if (d.ReadVarInteger(serial)) return __LINE__;

			if (serial > 0) {
				if (auto iter = reqs.find(serial); iter != reqs.end()) {
					iter->second.second = Data(d.buf, d.len, d.offset);				// 这里会产生一次 copy
					iter->second.first.cancel();
				}
			} else {
				if (serial == 0) {
					if (PEERTHIS->ReceivePush(d)) return __LINE__;						// PeerDeriveType 需提供函数: int ReceivePush(xx::Data_r& d)
				} else {
					if (PEERTHIS->ReceiveRequest(-serial, d)) return __LINE__;			// PeerDeriveType 需提供函数: int ReceiveRequest(int32_t serial, xx::Data_r& d)
				}
			}
			return 0;
		}
	};

}
