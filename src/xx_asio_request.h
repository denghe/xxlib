#pragma once
#include <xx_asio_ioc.h>

namespace xx {

	// member func exists checkers
	template<typename T> concept Has_FillSendDataHeader = requires(T t) { t.FillSendDataHeader(std::declval<xx::Data&>()); };

	// for attach codes: Send Request Response Push
	template<typename PeerDeriveType, size_t headerReserveLen = 16>
	struct PeerRequestCode {
		int32_t reqAutoId = 0;
		std::unordered_map<int32_t, std::pair<asio::steady_timer, Data>> reqs;

		// current package struct：datalen: fixed uint32_t [ + receive target: fixed uint32_t ] + serial: int32 + data
		template<typename F>
		DataShared MakeSendData(int32_t const& serial, size_t const& reserveLen, F&& dataFiller) {
			Data d;
			d.Reserve(headerReserveLen + reserveLen);									// preallocate memory ( header + data )
			auto headerOffset = d.WriteJump<false>(sizeof(uint32_t));					// skip head area & store offset
			if constexpr (Has_FillSendDataHeader<PeerDeriveType>) {						// call if exists FillSendDataHeader for fill header
				PEERTHIS->FillSendDataHeader(d);
			}
			d.WriteVarInteger<false>(serial);											// fill serial
			dataFiller(d);																// fill data
			d.WriteFixedAt(headerOffset, (uint32_t)(d.len - sizeof(uint32_t)));			// write datalen
			return DataShared(std::move(d));
		}

		// serial > 0
		template<typename F>
		void SendResponse(int32_t const& serial, size_t const& reserveLen, F&& dataFiller) {
			if (PEERTHIS->IsStoped()) return;
			PEERTHIS->Send(MakeSendData(serial, reserveLen, std::forward<F>(dataFiller)));
		}

		// serial == 0
		template<typename F>
		void SendPush(size_t const& reserveLen, F&& dataFiller) {
			if (PEERTHIS->IsStoped()) return;
			PEERTHIS->Send(MakeSendData(0, reserveLen, std::forward<F>(dataFiller)));
		}

		// serial < 0
		template<typename F>
		awaitable<Data> SendRequest(std::chrono::steady_clock::duration d, size_t const& reserveLen, F&& dataFiller) {
			if (PEERTHIS->IsStoped()) co_return Data();

			// gen serial
			reqAutoId = (reqAutoId + 1) % 0x7FFFFFFF;
			auto serial = reqAutoId;

			// store serial, coroutine blocker + data container
			auto [iter, ok] = reqs.emplace(serial, std::make_pair(asio::steady_timer(PEERTHIS->ioc, std::chrono::steady_clock::now() + d), Data()));
			assert(ok);
			auto& [timer, data] = iter->second;

			// send & wait, receive data -> container, cancel blocker
			PEERTHIS->Send(MakeSendData(-serial, reserveLen, std::forward<F>(dataFiller)));
			auto [e] = co_await timer.async_wait(use_nothrow_awaitable);

			// issue handle
			if (PEERTHIS->stoped || (e && (iter = reqs.find(serial)) == reqs.end())) co_return Data();

			// cleanup & return data
			auto r = std::move(data);
			reqs.erase(iter);
			co_return r;
		}

		// for base class
		int HandleMessage(Data_r& d) {
			int32_t serial;
			if (d.ReadVarInteger(serial)) return __LINE__;

			if (serial > 0) {
				if (auto iter = reqs.find(serial); iter != reqs.end()) {
					iter->second.second = Data(d.buf, d.len, d.offset);				// memcpy
					iter->second.first.cancel();
				}
			} else {
				if (serial == 0) {
					if (PEERTHIS->ReceivePush(d)) return __LINE__;						// call PeerDeriveType member function: int ReceivePush(xx::Data_r& d)
				} else {
					if (PEERTHIS->ReceiveRequest(-serial, d)) return __LINE__;			// call PeerDeriveType member function: int ReceiveRequest(int32_t serial, xx::Data_r& d)
				}
			}
			return 0;
		}
	};

}
