#pragma once
#include <xx_asio_tcp_base.h>
#include <xx_string_http.h>
#include <picohttpparser.h>

namespace xx {
	// 路由 server / client 的 peer http code 使用性质
	enum class HttpType {
		Request,
		Response
	};

	// 决定 html 头部内容组合
	enum class HtmlHeaders {
		OK_200_Text_Cache,
		OK_200_Text,
		OK_200_Html_Cache,
		OK_200_Html,
		NotFound_404_Text_Cache,
		NotFound_404_Text,
		NotFound_404_Html_Cache,
		NotFound_404_Html,
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

	// http peer base struct
	template<typename PeerDeriveType, HttpType httpType = HttpType::Request, size_t readBufLen = 1024 * 32, size_t headersCap = 32>
	struct PeerHttpCode {

		std::string_view method;	// for request
		std::string_view url;	// for request
		std::string_view msg;	// for response
		std::string_view body;	// for response / request ( POST mode store args )
		SVPairArray<headersCap> headers;	// for all
		size_t headersLen;
		int minorVersion;	// for all
		int status;	// for response
		std::string_view path;	// url 的 ? 号 之前的部分
		std::string_view args;	// url 的 ? 号 之后的部分
		std::string_view head;	// 指向整个 http 请求的原始内容( 不包含 body 部分 )
		Data bodyData;	// 如果 body 部分大于 read buf 长度, 将转储到该容器
		Data out;	// 用于逐步拼接构造 要发送的数据
		ssize_t outHeadLen = 0;	// 存储 head 部分长度( 用于算 Content-Length 填充留白位置 )

		// 清空 out 并写入 head. 可指定默认的内存预分配尺度 以便后续 填充 Content-Length
		template<HtmlHeaders h>
		void OutBegin(size_t const& contentCap = 1024 * 16) {
			out.Clear();
			out.Reserve(300 + contentCap);	// 300: 头部预计长度
			if constexpr (h == HtmlHeaders::OK_200_Text_Cache || h == HtmlHeaders::OK_200_Text || h == HtmlHeaders::OK_200_Html_Cache || h == HtmlHeaders::OK_200_Html) {
				out.WriteBuf<false>("HTTP/1.1 200 OK"sv);
			}
			if constexpr (h == HtmlHeaders::NotFound_404_Text_Cache || h == HtmlHeaders::NotFound_404_Text || h == HtmlHeaders::NotFound_404_Html_Cache || h == HtmlHeaders::NotFound_404_Html) {
				out.WriteBuf<false>("HTTP/1.1 404 Not Found"sv);
			}
			if constexpr (h == HtmlHeaders::OK_200_Text_Cache || h == HtmlHeaders::OK_200_Text || h == HtmlHeaders::NotFound_404_Text_Cache || h == HtmlHeaders::NotFound_404_Text) {
				out.WriteBuf<false>("\r\nContent-Type: text/plain;charset=utf-8"sv);
			}
			if constexpr (h == HtmlHeaders::OK_200_Html_Cache || h == HtmlHeaders::OK_200_Html || h == HtmlHeaders::NotFound_404_Html_Cache || h == HtmlHeaders::NotFound_404_Html) {
				out.WriteBuf<false>("\r\nContent-Type: text/html;charset=utf-8"sv);
			}
			out.WriteBuf<false>("\r\nConnection: keep-alive\r\nContent-Length:             \r\n\r\n"sv);	// 预留 12 个内容长度填充空格
			outHeadLen = out.len;	// 记录 head 总长度
		}

		// 不断向 out 写入 contents ( 可调用多次。超过 out buf 长度会自动扩容 )
		template<typename...StringViewContents>
		void Out(StringViewContents const&... contents) {
			assert(outHeadLen);
			(out.WriteBuf(contents), ...);
		}

		// 写入内容长度并发送
		void OutEnd() {
			std::to_chars((char*)out.buf + outHeadLen - 16, (char*)out.buf + outHeadLen - 4, out.len - outHeadLen);	// 将内容长度填充到相应位置
			PEERTHIS->Send(std::move(out));
			outHeadLen = 0;
		}

		// 一把梭, 拼接 head + contents 并发送
		template<HtmlHeaders h, typename...StringViewContents>
		void OutOnce(StringViewContents const&... contents) {
			auto contentLen = (contents.size() + ...);
			OutBegin<h>(contentLen);
			(out.WriteBuf<false>(contents), ...);
			OutEnd();
		}

		// 解析 key=value&key=value&.... 并转为 array<pair<key, value>
		// 通常直接使用 GetArgsArray 函数来得到填充好的容器
		// out 参数类型示例: SVPairArray<2> args = { std::pair{ "name"sv, ""sv}, { "repeat_times"sv, ""sv} };
		template<typename ArgsContainer>
		void FillArgsTo(ArgsContainer& out) {
			std::string_view sv(method == "POST"sv ? body : args);
			for (size_t i = 0; i < out.size(); i++) {
				if (auto key = xx::SplitOnce(sv, "="); !key.empty()) {
					for (auto& kv : out) {
						if (kv.first == key) {
							kv.second = xx::SplitOnce(sv, "&");
						}
					}
				}
			}
		}

		// 向 SVPairArray 填充 name 部分
		template<typename A, typename T, std::size_t...I>
		static void FillArgsNames(A& a, T const& t, std::index_sequence<I...>) {
			((a[I].first = std::get<I>(t)), ...);
		}

		// 根据 "name"sv 列表创建 SVPairArray 并填充后返回
		template<typename ... ArgNames>
		auto GetArgsArray(ArgNames const& ... names) ->SVPairArray<sizeof...(ArgNames)> {
			SVPairArray<sizeof...(ArgNames)> out;
			FillArgsNames(out, std::make_tuple(names...), std::make_index_sequence<sizeof...(ArgNames)>());
			FillArgsTo(out);
			return out;
		}

		// 向 s 填充 dump 信息
		void AppendDumpStr(std::string& s, size_t keyWidth = 40, bool dumpContent = false) {
			if (keyWidth < 40) {
				keyWidth = 40;
			}
			if (dumpContent) {
				xx::Append(s,
					"***************************************************************\n"
					"*                            head                             *\n"
					"***************************************************************\n"
					, head, '\n');
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
					"url( path + ? args )", CharRepeater{ ' ', keyWidth - 20 }, url, "\n"
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
			if (httpType == HttpType::Response || method == "POST"sv) {
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
					head = std::string_view(buf, httpLen);

					for (size_t i = 0; i < headersLen; ++i) {
						headers[i].first = std::string_view(z.headers[i].name, z.headers[i].name_len);
						headers[i].second = std::string_view(z.headers[i].value, z.headers[i].value_len);
					}

					bool needReadContent = httpType == HttpType::Response;
					if constexpr (httpType == HttpType::Request) {
						method = std::string_view(z.method, z.methodLen);
						url = std::string_view(z.url, z.urlLen);
						auto i = url.find('?');
						path = url.substr(0, i);
						args = i != url.npos ? url.substr(i + 1) : std::string_view{};
						if (method == "POST"sv) {
							needReadContent = true;
						}
					}
					else {
						msg = std::string_view(z.url, z.urlLen);
					}
					if (needReadContent) {
						if (auto cl = headers["Content-Length"sv]; cl.empty())
							break;	// 内容缺失: 内容长度 header 找不到
						else if (auto n = xx::SvToNumber<int>(cl, -1); n == -1)
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

	// attach handlers code snippets
	template<typename PeerDeriveType>
	struct PeerHttpRequestHandlersCode {

		// path 对应的 处理函数 容器( 扫 array 比 扫 map 快得多的多 )
		inline static std::array<std::pair<std::string_view, std::function<int(PeerDeriveType&)>>, 32> handlers;
		inline static size_t handlersLen;

		// 注册处理函数. 格式为 [&](SHPeer& p)->int   返回非 0 就掐线
		template<typename F>
		inline static void RegisterHttpRequestHandler(std::string_view const& path, F&& func) {
			handlers[handlersLen++] = std::make_pair(path, std::forward<F>(func));
		}

		// 处理收到的 http 请求( 会被 PeerHttpCode 基类调用 )
		inline int ReceiveHttp() {
			for (size_t i = 0; i < PeerDeriveType::handlersLen; i++) {			// 在 path 对应的 处理函数 容器 中 定位并调用
				if (PEERTHIS->path == PeerDeriveType::handlers[i].first) {
					return PeerDeriveType::handlers[i].second(*PEERTHIS);
				}
			}
			PEERTHIS->template OutOnce<xx::HtmlHeaders::NotFound_404_Text>("404"sv);	// 返回 资源不存在 的说明
			return 0;
		}
	};
}
