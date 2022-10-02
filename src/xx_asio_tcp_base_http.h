#pragma once
#include <xx_asio_tcp_base.h>
#include <picohttpparser.h>

namespace xx {
	enum class HttpType {
		Request,
		Response
	};

	// for phr_parse_request/response funcs
	template<size_t headersCap>
	struct PicoHttpParserArgs {
		const char* method;
		size_t methodLen;
		const char* url;
		size_t urlLen;
		std::array<phr_header, headersCap> headers;
	};

	template<typename PeerDeriveType, HttpType httpType = HttpType::Request, size_t readBufLen = 1024 * 32, size_t headersCap = 32>
	struct PeerHttpCode {

		std::string_view method;	// for request
		std::string_view url;	// for request
		std::string_view msg;	// for response
		std::string_view body;	// for response
		std::array<std::pair<std::string_view, std::string_view>, headersCap> headers;	// for all
		size_t headersLen;
		int minorVersion;	// for all
		int status;	// for response

		std::string_view content;	// 指向整个 http 请求的原始内容

		std::string_view path;	// url 的 ? 号 之前的部分. 需要调用 FillPathAndArgs 来填充
		std::string_view args;	// url 的 ? 号 之后的部分. 需要调用 FillPathAndArgs 来填充
		std::string tmp;
		Data bodyData;

		inline static constexpr std::string_view prefixText = "HTTP/1.1 200 OK\r\nContent-Type: text/plain;charset=utf-8\r\nConnection: keep-alive\r\nContent-Length: "sv;
		inline static constexpr std::string_view prefixHtml = "HTTP/1.1 200 OK\r\nContent-Type: text/html;charset=utf-8\r\nConnection: keep-alive\r\nContent-Length: "sv;
		inline static constexpr std::string_view prefix404 = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html;charset=utf-8\r\nConnection: keep-alive\r\nContent-Length: "sv;

		// 回发 文本 或 http
		void SendHtml(std::string_view const& prefix, std::string_view const& content) {
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

		// 在 headers 数组 中扫描获取 key 对应的 value
		std::optional<std::string_view> FindHeader(std::string_view const& key) {
			for (size_t i = 0; i < headersLen; i++) {
				if (headers[i].first == key) return headers[i].second;
			}
			return {};
		}

		// 向 s 填充 dump 信息
		void AppendDumpStr(std::string& s, size_t keyWidth = 40, bool dumpContent = false) {
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
			);
			if constexpr (httpType == HttpType::Request) {
				xx::Append(s,
					"method", CharRepeater{ ' ', keyWidth - 6 }, method, "\n"
				);
				xx::Append(s,
					"url", CharRepeater{ ' ', keyWidth - 3 }, url, "\n"
				);
			}
			else {
				xx::Append(s,
					"msg", CharRepeater{ ' ', keyWidth - 3 }, msg, "\n"
				);
				xx::Append(s,
					"status", CharRepeater{ ' ', keyWidth - 6 }, minorVersion, "\n"
				);
			}
			xx::Append(s,
				"minorVersion", CharRepeater{ ' ', keyWidth - 12 }, minorVersion, "\n"
			);
			xx::Append(s,
				"headersLen", CharRepeater{ ' ', keyWidth - 10 }, headersLen, '\n'
			);
			xx::Append(s,
				"---------------------------------------------------------------\n"
			);
			for (size_t i = 0; i < headersLen; ++i) {
				auto kSiz = headers[i].first.size();
				xx::Append(s, headers[i].first, CharRepeater{ ' ', kSiz > keyWidth ? 1 : (keyWidth - kSiz) }, headers[i].second, '\n');
			}
			if constexpr (httpType == HttpType::Response) {
				xx::Append(s,
					"---------------------------------------------------------------\n"
					"|                             body                            |\n"
					"---------------------------------------------------------------\n"
					, body, "\n"
				);
			}
			xx::Append(s,
				"==============================================================="
			);
		}

		// AppendDumpInfo 的直接返回版
		std::string GetDumpStr(size_t keyWidth = 40, bool dumpContent = false) {
			std::string s;
			AppendDumpStr(s, keyWidth, dumpContent);
			return s;
		}

		// 读 协程函数主体( protected )
		awaitable<void> Read() {
			auto block = std::make_unique<char[]>(readBufLen);
			auto buf = block.get();
			auto bufEnd = buf + readBufLen;
			size_t bufLen = 0, prevBufLen = 0;
			int httpLen;
			PicoHttpParserArgs<headersCap> z;

			for (;;) {
				{
					auto [ec, recvLen] = co_await PEERTHIS->socket.async_read_some(asio::buffer(buf + bufLen, readBufLen - bufLen), use_nothrow_awaitable);
					if (ec)
						break;
					if (PEERTHIS->stoped)
						co_return;
					prevBufLen = bufLen;
					bufLen += recvLen;
				}

			LabBeginParse:
				headersLen = headersCap;

				// -1: parse failed   -2: data is partial   >0 && s.size(): successful
				if constexpr (httpType == HttpType::Request) {
					httpLen = phr_parse_request(buf, bufLen, &z.method, &z.methodLen, &z.url, &z.urlLen, &minorVersion, z.headers.data(), &headersLen, prevBufLen);
					if (httpLen == -1)
						break;	// parse 失败
				}
				else {
					httpLen = phr_parse_response(buf, bufLen, &minorVersion, &status, &z.url, &z.urlLen, z.headers.data(), &headersLen, prevBufLen);
					if (httpLen == -1)
						break;	// parse 失败
				}

				if (httpLen > 0) {
					content = std::string_view(buf, httpLen);

					for (size_t i = 0; i < headersLen; ++i) {
						headers[i].first = std::string_view(z.headers[i].name, z.headers[i].name_len);
						headers[i].second = std::string_view(z.headers[i].value, z.headers[i].value_len);
					}

					if constexpr (httpType == HttpType::Request) {
						method = std::string_view(z.method, z.methodLen);
						url = std::string_view(z.url, z.urlLen);
					}
					else {
						msg = std::string_view(z.url, z.urlLen);
						if (auto cl = FindHeader("Content-Length"sv); !cl.has_value())
							break;	// 内容缺失: 内容长度 header 找不到
						else if (auto n = xx::SvToNumber<int>(cl.value(), -1); n == -1)
							break;	// string 2 int 转换失败
						else if (httpLen + n <= bufLen) {	// buf 已含有完整内容: 设置到 body
							body = std::string_view{ buf + httpLen, (size_t)n };
							httpLen += n;
						}
						else {	// buf 中 body 部分不全: read 到 bodyData 再设置到 body
							// todo: check n limit
							bodyData.Clear(true);
							bodyData.Reserve(n);
							bodyData.len = n;
							auto siz = bufLen - httpLen;
							memcpy(bodyData.buf, buf + httpLen, siz);	// copy buf 中部分 body
							// todo: timeout check?
							auto [ec, recvLen] = co_await asio::async_read(PEERTHIS->socket, asio::buffer(bodyData.buf + siz, n - siz), use_nothrow_awaitable);
							if (ec)
								break;
							if (PEERTHIS->stoped)
								co_return;
							assert(recvLen == n - siz);
							body = std::string_view{ (char*)bodyData.buf, (size_t)n };
							httpLen = (int)bufLen;
						}
					}

					if (int r = PEERTHIS->ReceiveHttp())
						break;

					prevBufLen = 0;
					if ((bufLen -= httpLen)) {	// 如果 bufLen 消费掉 httpLen 后 不为 0: 多段 http ( 通常不太可能发生 )
						buf += httpLen;	// 指向下一段
						goto LabBeginParse;	// 跳转继续处理
					}

					buf = block.get();	// 没了: 重置并继续收包
					bufLen = 0;
					continue;
				}

				// -2
				if (buf > block.get()) {	// 处理过多段 http 内容: 将剩余内容置顶
					memmove(block.get(), buf, bufLen);
					buf = block.get();
				}
				else if (bufLen == readBufLen)
					break;	// buf 填满了
			}
			PEERTHIS->Stop();
		}
	};
}
