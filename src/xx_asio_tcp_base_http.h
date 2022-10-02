#pragma once
#include <xx_asio_tcp_base.h>
#include <picohttpparser.h>

namespace xx {
	template<typename PeerDeriveType, size_t readBufLen = 4194304, size_t headersCap = 64>
	struct PeerHttpCode {
		std::string_view method;
		std::string_view url;
		std::array<std::pair<std::string_view, std::string_view>, headersCap> headers;
		// 用来硬转 上面这个结构以迎合 phr_parse_request 参数需求. 这两段代码内存一一对应
		struct Union {
			const char* method;
			size_t methodLen;
			const char* url;
			size_t urlLen;
			std::array<phr_header, headersCap> headers;
		};
		size_t headersLen;	// headers.len
		int minorVersion;
		std::string_view content;	// 指向整个 http 请求的原始内容
		std::string_view path;	// url 的 ? 号 之前的部分. 需要调用 FillPathAndArgs 来填充
		std::string_view args;	// url 的 ? 号 之后的部分. 需要调用 FillPathAndArgs 来填充
		std::string tmp;

		inline static constexpr std::string_view prefixText = "HTTP/1.1 200 OK\r\nContent-Type: text/plain;charset=utf-8\r\nConnection: keep-alive\r\nContent-Length: "sv;
		inline static constexpr std::string_view prefixHtml = "HTTP/1.1 200 OK\r\nContent-Type: text/html;charset=utf-8\r\nConnection: keep-alive\r\nContent-Length: "sv;
		inline static constexpr std::string_view prefix404 = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html;charset=utf-8\r\nConnection: keep-alive\r\nContent-Length: "sv;

		// 回发 文本 或 http
		void SendResponse(std::string_view const& prefix, std::string_view const& content) {
			xx::Data d(prefix.size() + content.size() + 32);
			d.WriteBuf(prefix.data(), prefix.size());
			tmp.clear();
			xx::Append(tmp, content.size());
			d.WriteBuf(tmp.data(), tmp.size());
			d.WriteBuf("\r\n\r\n", 4);
			d.WriteBuf(content.data(), content.size());
			PEERTHIS->Send(std::move(d));
		};

		// 根据 url 填充 path & args
		void FillPathAndArgs() {
			auto i = url.find('?');
			args = i != url.npos ? url.substr(i + 1) : std::string_view{};
			path = url.substr(0, i);
		}

		// 将 headers 数组 的东西 填充到 目标字典容器( 需携带 operator[ k ] = v 函数 )
		// 如果需要查询, 直接 FindHeader 最快, 比先填字典再查, 快得多得多
		template<typename T>
		void FillHeadersTo(T& map) {
			for (size_t i = 0; i < headersLen; i++) {
				map[headers[i].first] = headers[i].second;
			}
		}

		// 在 headers 数组 中扫描获取 key 对应的 value
		std::optional<std::string_view> FindHeader(std::string_view const& key) {
			for (size_t i = 0; i < headersLen; i++) {
				if (headers[i].first == key) return headers[i].second;
			}
			return {};
		}

		// 向 s 填充 dump 信息
		void AppendDumpInfo(std::string& s, size_t keyWidth = 40, bool dumpContent = false, bool dumpMinorVersion = false, bool dumpHeadersLen = false) {
			if (keyWidth < 40) {
				keyWidth = 40;
			}
			if (dumpContent) {
				xx::Append(s,
					"***************************************************************\n"
					"*                      original content                       *\n"
					"***************************************************************\n"
					, content, '\n');
			}
			xx::Append(s,
				"***************************************************************\n"
				"*                         parsed info                         *\n"
				"===============================================================\n"
				"method", CharRepeater{ ' ', keyWidth - 6 }, method, "\n"
				"url", CharRepeater{ ' ', keyWidth - 3 }, url, "\n"
				"---------------------------------------------------------------\n"
			);
			if (dumpMinorVersion) {
				xx::Append(s,
					"minorVersion", CharRepeater{ ' ', keyWidth - 12 }, minorVersion, "\n"
				);
			}
			if (dumpHeadersLen) {
				xx::Append(s,
					"headersLen", CharRepeater{ ' ', keyWidth - 10 }, headersLen, '\n'
				);
			}
			for (size_t i = 0; i < headersLen; ++i) {
				auto kSiz = headers[i].first.size();
				xx::Append(s, headers[i].first, CharRepeater{ ' ', kSiz > keyWidth ? 1 : (keyWidth - kSiz) }, headers[i].second, '\n');
			}
			xx::Append(s,
				"==============================================================="
			);
		}

		// 读 协程函数主体( protected )
		awaitable<void> Read() {
			auto block = std::make_unique<char[]>(readBufLen);
			auto buf = block.get();
			auto bufEnd = buf + readBufLen;
			size_t bufLen = 0, prevBufLen = 0;
#ifdef _WIN32
			auto& z = *(Union*)&method;
#else
            Union z;
#endif

			for (;;) {
				auto [ec, recvLen] = co_await PEERTHIS->socket.async_read_some(asio::buffer(buf + bufLen, readBufLen - bufLen), use_nothrow_awaitable);
				if (ec) break;
				if (PEERTHIS->stoped) co_return;
				prevBufLen = bufLen;
				bufLen += recvLen;

			LabBeginParse:
				headersLen = headersCap;
				// -1: parse failed   -2: data is partial   >0 && s.size(): successful
				if (int httpLen = phr_parse_request(buf, bufLen, &z.method, &z.methodLen, &z.url, &z.urlLen, &minorVersion, z.headers.data(), &headersLen, prevBufLen); httpLen > 0) {
#ifndef _WIN32
                    method = std::string_view(z.method, z.methodLen);
                    url = std::string_view(z.url, z.urlLen);
                    for(size_t i = 0; i < headersLen; ++i) {
                        headers[i].first = std::string_view(z.headers[i].name, z.headers[i].name_len);
                        headers[i].second = std::string_view(z.headers[i].value, z.headers[i].value_len);
                    }
#endif
					content = std::string_view(buf, httpLen);
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
