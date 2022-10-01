#pragma once
#include <xx_asio_tcp_base.h>
#include <picohttpparser.h>

namespace xx {
	template<typename PeerDeriveType, size_t readBufLen = 4194304, size_t headersCap = 32>
	struct PeerHttpCode {
		struct {
			std::string_view method;
			std::string_view path;
			std::array<std::pair<std::string_view, std::string_view>, headersCap> headers;
		};
		// 用来硬转 上面这个结构以迎合 phr_parse_request 参数需求. 这两段代码内存一一对应
		struct Union {
			const char* method;
			size_t methodLen;
			const char* path;
			size_t pathLen;
			std::array<phr_header, headersCap> headers;
		};
		size_t numHeaders;	// parse 完成后将存储 headers 的实际成员个数
		int minorVersion;
		std::string tmp;

		inline static constexpr std::string_view prefixText = "HTTP/1.1 200 OK\r\nContent-Type: text/plain;charset=utf-8\r\nConnection: keep-alive\r\nContent-Length: "sv;
		inline static constexpr std::string_view prefixHtml = "HTTP/1.1 200 OK\r\nContent-Type: text/html;charset=utf-8\r\nConnection: keep-alive\r\nContent-Length: "sv;
		inline static constexpr std::string_view prefix404 = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html;charset=utf-8\r\nConnection: keep-alive\r\nContent-Length: "sv;

		// 回发 文本 或 http
		int SendResponse(std::string_view const& prefix, std::string_view const& content) {
			xx::Data d(prefix.size() + content.size() + 32);
			d.WriteBuf(prefix.data(), prefix.size());
			tmp.clear();
			xx::Append(tmp, content.size());
			d.WriteBuf(tmp.data(), tmp.size());
			d.WriteBuf("\r\n\r\n", 4);
			d.WriteBuf(content.data(), content.size());
			PEERTHIS->Send(std::move(d));
			return 0;
		};

		// 读 协程函数主体( protected )
		awaitable<void> Read() {
			auto block = std::make_unique<char[]>(readBufLen);
			auto buf = block.get();
			auto bufEnd = buf + readBufLen;
			size_t bufLen = 0, prevBufLen = 0;
			auto& z = *(Union*)&method;

			for (;;) {
				auto [ec, recvLen] = co_await PEERTHIS->socket.async_read_some(asio::buffer(buf + bufLen, readBufLen - bufLen), use_nothrow_awaitable);
				if (ec) break;
				if (PEERTHIS->stoped) co_return;
				prevBufLen = bufLen;
				bufLen += recvLen;

			LabBeginParse:
				numHeaders = headersCap;
				// -1: parse failed   -2: data is partial   >0 && s.size(): successful
				if (int httpLen = phr_parse_request(buf, bufLen, &z.method, &z.methodLen, &z.path, &z.pathLen, &minorVersion, z.headers.data(), &numHeaders, prevBufLen); httpLen > 0) {
					if (int r = PEERTHIS->ReceiveHttpRequest()) break;
					prevBufLen = 0;
					if ((bufLen -= httpLen)) {	// 如果 bufLen 消费掉 httpLen 后 不为 0: 多段 http ( 通常不太可能发生 )
						buf += httpLen;	// 指向下一段
						goto LabBeginParse;	// 跳转继续处理
					}
					buf = block.get();	// 没了: 重置并继续收包
					bufLen = 0;
					continue;
				}
				else if (httpLen == -1) break;	// parse 失败
				if (buf > block.get()) {	// 处理过多段 http 内容: 将剩余内容置顶
					memmove(block.get(), buf, bufLen);
					buf = block.get();
				}
				else if (bufLen == readBufLen) break;	// buf 填满了
			}
			PEERTHIS->Stop();
		}
	};
}
