#pragma once

#include "xx_epoll.h"
#include "xx_string.h"
#include "http_parser.h"
#include <map>

namespace xx::Epoll {
    struct HttpPeer : TcpPeer {
        /** PRIVATE **/
        // 来自 libuv 作者的转换器及配置
        http_parser httpParser{};
        http_parser_settings httpParserSettings{};
        // 接收 header 中键值对的临时容器
        std::string lastKey, lastValue;
        // 标记是否正在接收 header 的键值对的值部分. 如果为 true，则下次再收到 key 就知道应该存档了
        bool recvingValue = false;

        /** PUBLIC **/
        // 下面这些是在 ReceiveHttp 中可以随便访问的东西。已经由 parser 填充完毕。
        std::string url;
        std::map<std::string, std::string> header;
        std::string body;
        std::string method;
        std::string status;
        int keepAlive = 0;

        // 初始化 http parser 相关
        HttpPeer(std::shared_ptr<Context> const &ec, int const &fd);

        // 实现收数据逻辑
        void Receive() override;

        // 断线事件( 清除自持有 )
        bool Close(int const &reason, char const* const& desc) override;

        // 为开始接收一段 http 做好准备( 各种 cleanup )
        virtual void Clear();

        // 被 Receive() 间接调用. 里面撰写收到一段 http 之后的处理逻辑
        virtual void ReceiveHttp() = 0;
    };


    inline HttpPeer::HttpPeer(std::shared_ptr<Context> const& ec, int const& fd)
            : TcpPeer(ec, fd) {
        // 初始化 parser. 绑定各种回调。通过 data 来传递 this
        http_parser_init(&httpParser, HTTP_BOTH);
        httpParser.data = this;
        http_parser_settings_init(&httpParserSettings);
        httpParserSettings.on_message_begin = [](http_parser* parser) noexcept {
            //auto&& self = *(HttpPeer*)parser->data;
            return 0;
        };
        httpParserSettings.on_url = [](http_parser* parser, const char* buf, size_t length) noexcept {
            auto&& self = *(HttpPeer*)parser->data;
            self.url.append(buf, length);
            return 0;
        };
        httpParserSettings.on_status = [](http_parser* parser, const char* buf, size_t length) noexcept {
            auto&& self = *(HttpPeer*)parser->data;
            self.status.append(buf, length);
            return 0;
        };
        httpParserSettings.on_header_field = [](http_parser* parser, const char* buf, size_t length) noexcept {
            auto&& self = *(HttpPeer*)parser->data;
            if (self.recvingValue) {
                self.recvingValue = false;
                self.header.insert(std::make_pair(std::move(self.lastKey), std::move(self.lastValue)));
            }
            self.lastKey.append(buf, length);
            return 0;
        };
        httpParserSettings.on_header_value = [](http_parser* parser, const char* buf, size_t length) noexcept {
            auto&& self = *(HttpPeer*)parser->data;
            self.recvingValue = true;
            self.lastValue.append(buf, length);
            return 0;
        };
        httpParserSettings.on_headers_complete = [](http_parser* parser) noexcept {
            auto&& self = *(HttpPeer*)parser->data;
            if (self.recvingValue) {
                self.recvingValue = false;
                self.header.insert(std::make_pair(std::move(self.lastKey), std::move(self.lastValue)));
            }
            self.method = http_method_str((http_method)parser->method);
            self.keepAlive = http_should_keep_alive(parser);
            return 0;
        };
        httpParserSettings.on_body = [](http_parser* parser, const char* buf, size_t length) noexcept {
            auto&& self = *(HttpPeer*)parser->data;
            self.body.append(buf, length);
            return 0;
        };
        httpParserSettings.on_message_complete = [](http_parser* parser) noexcept {
            auto&& self = *(HttpPeer*)parser->data;
            self.ReceiveHttp();
            self.Clear();
            return 0;
        };
        httpParserSettings.on_chunk_header = [](http_parser* parser) noexcept {
            //auto&& self = *(HttpPeer*)parser->data;
            // todo
            return 0;
        };
        httpParserSettings.on_chunk_complete = [](http_parser* parser) noexcept {
            //auto&& self = *(HttpPeer*)parser->data;
            // todo
            return 0;
        };
    }

    inline void HttpPeer::Receive() {
        auto len = recv.len;
        auto&& parsedLen = http_parser_execute(&httpParser, &httpParserSettings, recv.buf, len);
        recv.Clear();
        if (parsedLen < len) {
            Close(__LINE__, __FILE__);       // http_errno_description((http_errno)httpParser.http_errno)
        }
    }

    inline bool HttpPeer::Close(int const &reason, char const* const& desc) {
        if (!this->Item::Close(reason, desc)) return false;
        // 从 ec->holdItems 延迟移除 以 释放智能指针( 出函数后 )
        DelayUnhold();
        return true;
    }

    inline void HttpPeer::Clear() {
        url.clear();
        header.clear();
        body.clear();
        method.clear();
        status.clear();
        recvingValue = false;
        lastKey.clear();
        lastValue.clear();
        keepAlive = 0;
    }





    // 增强版。提供了一些简单的 url/post 解析 和输出功能
    struct HttpPeerEx : HttpPeer {
        using HttpPeer::HttpPeer;

        inline void Clear() override {
            this->HttpPeer::Clear();
            path.clear();
            fragment = nullptr;
            queries.clear();
            posts.clear();
            tmp.clear();
            tmp2.clear();
        }

        // 下面 3 个 field 在执行 ParseUrl 之后被填充

        // 前面不带 /, 不含 ? 和 #
        std::string path;

        // #xxxxxx 部分
        char* fragment = nullptr;

        // ?a=1&b=2....参数部分的键值对
        std::vector<std::pair<char*, char*>> queries;

        // POST 模式 body 内容 a=1&b=2....参数部分的键值对
        std::vector<std::pair<char*, char*>> posts;

        // url decode 数据容器, 不可以修改内容( queries 里面的 char* 会指向这里 )
        std::string tmp;

        // url decode 数据容器, 不可以修改内容( posts 里面的 char* 会指向这里 )
        std::string tmp2;

        // 依次在 queries 和 posts 中查找, 找到后立马返回. 未找到则返回 nullptr
        inline char const* operator[](char const* const& key) const noexcept {
            for (auto&& kv : queries) {
                if (!strcmp(kv.first, key)) return kv.second;
            }
            for (auto&& kv : posts) {
                if (!strcmp(kv.first, key)) return kv.second;
            }
            return nullptr;
        }


        // 从 queries 查找 queryKey 并转换数据类型填充 value
        template<typename T>
        inline bool TryParse(std::vector<std::pair<char*, char*>> const& kvs, char const* const& key, T& value) {
            for (auto&& item : kvs) {
                if (!strcmp(key, item.first)) {
                    xx::Convert(item.second, value);
                    return true;
                }
            }
            return false;
        }

        // 从 queries 下标定位 并转换数据类型填充 value
        template<typename T>
        inline bool TryParse(std::vector<std::pair<char*, char*>> const& kvs, size_t const& index, T& value) {
            if (index >= kvs.size()) return false;
            xx::Convert(kvs[index].second, value);
            return true;
        }

        // 会 urldecode 并 填充 path, queries, fragment. 需要手动调用
        inline void ParseUrl() noexcept {
            tmp.reserve(url.size());
            tmp.clear();
            xx::UrlDecode(url, tmp);
            auto u = (char*)tmp.c_str();

            // 从后往前扫
            fragment = FindAndTerminate(u, '#');
            auto q = FindAndTerminate(u, '?');
            path = FindAndTerminate(u, '/');

            FillQueries(queries, q);
        }

        // 会 解析 body 填充 posts. 需要手动调用
        inline void ParsePost() noexcept {
            tmp = body;
            FillQueries(posts, (char*)tmp.c_str());
        }


        static inline void FillQueries(std::vector<std::pair<char*, char*>>& kvs, char* q) noexcept {
            kvs.clear();
            if (!q || '\0' == *q) return;
            kvs.reserve(16);
            int i = 0;
            kvs.resize(i + 1);
            kvs[i++].first = q;
            while ((q = strchr(q, '&'))) {
                kvs.reserve(i + 1);
                kvs.resize(i + 1);
                *q = '\0';
                kvs[i].first = ++q;
                kvs[i].second = nullptr;

                if (i && (kvs[i - 1].second = strchr(kvs[i - 1].first, '='))) {
                    *(kvs[i - 1].second)++ = '\0';
                }
                i++;
            }
            if ((kvs[i - 1].second = strchr(kvs[i - 1].first, '='))) {
                *(kvs[i - 1].second)++ = '\0';
            }
        }

        inline static char* FindAndTerminate(char* s, char const& c) noexcept {
            s = strchr(s, c);
            if (!s) return nullptr;
            *s = '\0';
            return s + 1;
        }



        inline static std::string prefixText =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain;charset=utf-8\r\n"
                "Connection: keep-alive\r\n"
                "Content-Length: ";

        inline static std::string prefixHtml =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html;charset=utf-8\r\n"
                "Connection: keep-alive\r\n"
                "Content-Length: ";

        inline static std::string prefix404 =
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/html;charset=utf-8\r\n"
                "Connection: keep-alive\r\n"
                "Content-Length: ";

        // 输出用 http 容器
        std::string output;

        // 向 output 追加任意内容
        template<typename ...Args>
        inline void Append(Args const& ... content) {
            xx::Append(output, content...);
        }


        // 向 output 追加 <tag>...</tag> 或 <tag.../>  内容. 参数 tag 不能带 <>
        template<typename ...Args>
        inline void Tag(char const* const& tag, Args const& ... content) {
            if constexpr (sizeof...(content)) {
                Append("<", tag, ">", content..., "<", tag, "/>");
            }
            else {
                Append("<", tag, "/>");
            }
        }


        template<typename ...Args>
        inline void TableBegin(Args const& ... titles) {
            Append("<table border=\"1\">");
            Append("<thead><tr>");
            (Append("<th>", titles, "</th>"), ... );
            Append("</tr></thead><tbody>");
        }

        template<typename ...Args>
        inline void TableRow(Args const& ... columns) {
            Append("<tr>");
            (Append("<td>", columns, "</td>"), ... );
            Append("</tr>");
        }

        inline void TableEnd() {
            Append("</tbody></table>");
        }

        template<typename ...Args>
        inline void P(Args const& ... contents) {
            Append("<p>", contents..., "</p>");
        }

        template<typename ...Args>
        inline void A(char const* const& content, Args const& ... hrefs) {
            Append("<a href=\"", hrefs..., "\">", content, "</a>");
        }

        template<typename ...Args>
        inline void ABegin(Args const& ... hrefs) {
            Append("<a href=\"", hrefs..., "\">");
        }

        inline void AEnd() {
            Append("</a>");
        }

        template<typename ...Args>
        inline void FormBegin(Args const& ... actions) {
            Append(R"(<form method="post" action=")", actions..., "\">");
        }

        template<typename ...Args>
        inline void FormEnd(Args const& ... submitTexts) {
            Append(R"(<input type="submit" value=")", submitTexts...,"\" /></form>");
        }

        inline void Input(char const* const& title, char const* const& name = nullptr, char const* const& value = nullptr, char const* const& type = nullptr) {
            Append("<p>", (title ? title : name), ":<input type=\"", (type ? type : "text"), "\" name=\"", (name ? name : title), "\" value=\"", value, "\" /></p>");
        }


        // 将 output 以 html 格式发出
        inline int Flush() {
            return SendCore(prefixHtml, output.data(), output.size());
        }

        template<typename...Args>
        inline int Send(std::string const& prefix, Args const& ...args) noexcept {
            Append(args...);
            return SendCore(prefix, output.data(), output.size());
        }

        inline int SendHtml(std::string const& html) {
            return SendCore(prefixHtml, html.data(), html.size());
        }

        inline int SendText(std::string const& text) {
            return SendCore(prefixText, text.data(), text.size());
        }

        inline int SendJson(std::string const& json) {
            return SendCore(prefixText, json.data(), json.size());
        }

        inline int Send404(std::string const& html) {
            return SendCore(prefix404, html.data(), html.size());
        }

        inline int SendHtmlBody(std::string const& body) {
            return Send(prefixHtml, "<html><body>", body, "</body></html>");
        }

        inline int Send404Body(std::string const& body) {
            return Send(prefix404, "<html><body>", body, "</body></html>");
        }

    protected:
        inline int SendCore(std::string const& prefix, char const* const& buf, size_t const& len) {
            // calc buf max len
            auto cap = /* partial http header */prefix.size() + /* len */20 + /*\r\n\r\n*/4 + /* body */len + /* refs */8;

            // alloc memory
            auto data = (char*)::malloc(cap);
            size_t dataLen = 0;

            // append partial http header
            memcpy(data + dataLen, prefix.data(), prefix.size());
            dataLen += prefix.size();

            // append len
            auto&& lenStr = std::to_string(len);
            memcpy(data + dataLen, lenStr.data(), lenStr.size());
            dataLen += lenStr.size();

            // append \r\n\r\n
            memcpy(data + dataLen, "\r\n\r\n", 4);
            dataLen += 4;

            // append content
            ::memcpy(data + dataLen, buf, len);
            dataLen += len;

            // send
            return this->HttpPeer::Send(xx::Data(data, dataLen));
        };
    };


}
