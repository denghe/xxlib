#pragma once
#ifdef _WIN32
#include <mysql.h>
//#pragma comment(lib, "C:/Program Files/MariaDB/MariaDB Connector C 64-bit/lib/libmariadb.lib")
#else
#include <mariadb/mysql.h>
#endif

#include "xx_data.h"
#include "xx_string.h"
#include <mutex>        // for mysql_init

// 注意：除非函数标记了 noexcept, 否则就需要 try
// todo: + noexcept

// 当前已支持的数据类型：int, int64_t, double, std::string, Data


namespace xx::MySql {
    // 仅供打印 / dump 等需求的通用数据容器
    struct Result {
        // 受影响行数
        my_ulonglong affectedRows = 0;
        // 如果有数据的话，这是 字段名 列表
        std::vector<std::string> columns;
        // 如果有数据的话，这是 行数据 列表
        std::vector<std::vector<std::string>> rows;
    };
    // std::vector<Result>   多结果集
}
namespace xx {
    template<>
    struct StringFuncs<xx::MySql::Result, void> {
        static inline void Append(std::string &s, xx::MySql::Result const &in) {
            xx::Append(s, "{\"affectedRows\":", in.affectedRows, ",\"columns\":", in.columns, ",\"rows\":", in.rows, '}');
        }
    };
}

namespace xx::MySql {
    // string, Data 字串转义 for Append(string
    struct EscapeWrapper {
        char const* buf;
        unsigned long len;
        EscapeWrapper(std::string const& s) : buf((char*)s.data()), len((unsigned long)s.size()) {}
        EscapeWrapper(std::string_view const& s) : buf((char*)s.data()), len((unsigned long)s.size()) {}
        EscapeWrapper(Data const& d) : buf((char*)d.buf), len((unsigned long)d.len) {}
        EscapeWrapper(DataView const& d) : buf((char*)d.buf), len((unsigned long)d.len) {}
        EscapeWrapper(char const* const& buf,  size_t const& len) : buf(buf), len((unsigned long)len) {}
    };
}
namespace xx {
    template<>
    struct StringFuncs<xx::MySql::EscapeWrapper, void> {
        static inline void Append(std::string &s, xx::MySql::EscapeWrapper const &in) {
            auto siz = s.size();
            s.resize(siz + in.len * 2 + 1);
            auto buf = (char*)s.data() + siz;
            auto &&n = mysql_escape_string(buf, in.buf, in.len);
            s.resize(siz + n);
        }
    };
}

namespace xx::MySql {
    // 字串转义
    std::string Escape(std::string const &in);

    // 发现 mysql_init 非线程安全 故全局 lock 一下
    inline std::mutex mutex_connOpen;

    struct Connection;

    // 查询结果（结果集中的一个）上下文
    struct Info {
        Connection &conn;
        // 一共有多少个字段
        unsigned int numFields = 0;
        // 一共有多少行数据
        my_ulonglong numRows = 0;
        // 受影响行数
        my_ulonglong affectedRows = 0;
        // 字段信息数组
        MYSQL_FIELD *fields = nullptr;

        explicit Info(Connection &conn);

        // 查看某字段具体信息( 比较安全的访问 fields 数组 )
        MYSQL_FIELD const &operator[](int const &colIdx) const;
    };

    // 数据读取器
    struct Reader {
        Info &info;

        MYSQL_ROW data = nullptr;
        unsigned long *lengths = nullptr;

        explicit Reader(Info &info);

        bool IsDBNull(int const &colIdx) const;

        int ReadInt32(int const &colIdx) const;

        int64_t ReadInt64(int const &colIdx) const;

        double ReadDouble(int const &colIdx) const;

        std::string ReadString(int const &colIdx) const;

        Data ReadData(int const &colIdx) const;

        // todo: ato? 替换为更快的实现?
        // todo: more types support?

        // 填充( 不做 数据类型校验. char* 能成功转为目标类型就算成功 )
        template<typename T>
        void Read(int const &colIdx, T &outVal) const;

        template<typename T>
        void Read(char const *const &colName, T &outVal);

        template<typename T>
        void Read(std::string const &colName, T &outVal);

        // 一次填充多个
        template<typename...Args>
        void Reads(Args &...args);

    protected:
        template<typename Arg, typename...Args>
        void ReadsCore(int &colIdx, Arg &arg, Args &...args);

        void ReadsCore(int &colIdx);
    };

    // mysql 连接主体
    struct Connection {
        // c api 上下文指针
        MYSQL *ctx = nullptr;

        // 上次执行所产生的错误码( 可能是 __LINE__ 或 mysql_errno )
        int errCode = 0;

        // 上次执行所产生的错描述( 可能是 自定义 或 mysql_error )
        std::string errText;

        Connection() = default;

        Connection(Connection const &) = delete;

        Connection(Connection &&o) noexcept;

        Connection &operator=(Connection const &) = delete;

        Connection &operator=(Connection &&o);

        ~Connection();

        // 使用指定参数 打开 mysql 数据库连接
        void
        Open(std::string const &host, int const &port, std::string const &username, std::string const &password,
             std::string const &db);

        // 关闭 mysql 数据库连接
        void Close();

        // 调用 mysql_ping
        int Ping();

        // 执行一段 SQL 脚本. 后续 *必须* 使用 Fetch 处理所有结果集( 如果多个的话 ), 或执行 ClearResult 抛弃结果集. 出错抛异常
        template<size_t len>
        void Execute(char const(&sql)[len]);

        void Execute(std::string const &sql);

        // 多个参数拼接为 SQL 并执行. 后续 *必须* 使用 Fetch 处理所有结果集( 如果多个的话 ), 或执行 ClearResult 抛弃结果集. 出错抛异常
        template<typename ...Args>
        void Execute(Args const &...args);

        // 抛弃查询结果( 解决一次性执行多条 分号间隔语句 啥的并不关心结果, 下次再执行出现 2014 错误 的问题 )
        // 单条 sql 并不需要每次抛弃结果就可以反复执行
        void ClearResult();

        /*
    while(conn.Fetch([](xx::MySql::Info &info) -> bool {
        return true;
    }, [](xx::MySql::Reader &reader) -> bool {
        return true;
    })) {}
        */
        // 填充一个结果集, 并产生相应回调. 返回 是否存在下一个结果集. 出错抛异常
        // infoHandler 返回 true 将继续对每行数据发起 rowHandler 调用. 返回 false, 将终止调用
        bool Fetch(std::function<bool(Info &)> &&infoHandler, std::function<bool(Reader &)> &&rowHandler);

        // 填充 遇到的第一个有数据的结果集的第一行的 sizeof(args...) 个字段的值 到 args 变量
        template<typename ...Args>
        void FetchTo(Args &...args);

        // 填充 遇到的第一个有数据的结果集的第一行的 sizeof(args...) 个字段的值 到 tuple
        template<typename ...Args>
        void FetchTo(std::tuple<Args...> &tuple);

        // 针对只有 1 行 1 列的单结果集快速读取. 出错抛异常
        template<typename T>
        T FetchScalar();

        // 返回所有结果集
        std::vector<Result> FetchResults();

        // 返回遇到的第一个有数据的结果集的第一列
        template<typename T>
        std::vector<T> FetchList();

        // 针对只有 1 行 1 列的单结果集查询，执行并读取. 出错抛异常
        template<typename T, typename ...Args>
        T ExecuteScalar(Args const &...args);

        // 执行查询并返回所有结果集
        template<typename ...Args>
        std::vector<Result> ExecuteResults(Args const &...args);

        // 执行查询，清除所有结果( 不必再 Fetch )，返回第一个结果集的受影响行数
        template<typename ...Args>
        my_ulonglong ExecuteNonQuery(Args const &...args);

        // 执行查询并返回遇到的第一个有数据的结果集的第一列
        template<typename T, typename ...Args>
        std::vector<T> ExecuteList(Args const &...args);

        // 执行查询并填充 遇到的第一个有数据的结果集的第一行的 sizeof(args...) 个字段的值 到 args 变量
        template<typename ...Args>
        void ExecuteTo(std::string const &sql, Args &...args);

        // 执行查询并填充 遇到的第一个有数据的结果集的第一行的 sizeof(args...) 个字段的值 到 tuple
        template<typename ...Args>
        void ExecuteTo(std::string const &sql, std::tuple<Args...> &tuple);

        // 通用抛异常函数
        void Throw(int const &code, std::string &&desc);
    };
}


/******************************************************************************************************************/
// impls
/******************************************************************************************************************/

namespace xx::MySql {
    inline std::string Escape(std::string const &in) {
        std::string s;
        s.resize(in.size() * 2 + 1);
        auto &&n = mysql_escape_string((char *) s.data(), (char *) in.data(), (unsigned long) in.size());
        s.resize(n);
        return s;
    }

    inline Info::Info(Connection &conn)
            : conn(conn) {
    }

    inline MYSQL_FIELD const &Info::operator[](int const &colIdx) const {
        // 前置检查
        if (colIdx < 0 || colIdx >= (int) numFields) {
            conn.Throw(__LINE__, xx::ToString("colIdx: ", colIdx, " out of range. numFields = ", numFields));
        }
        return fields[colIdx];
    }

    inline Reader::Reader(Info &info)
            : info(info) {
    }

    inline bool Reader::IsDBNull(int const &colIdx) const {
        return !data[colIdx];
    }

    inline int Reader::ReadInt32(int const &colIdx) const {
        return atoi(data[colIdx]);
    }

    inline int64_t Reader::ReadInt64(int const &colIdx) const {
        return atoll(data[colIdx]);
    }

    inline double Reader::ReadDouble(int const &colIdx) const {
        return atof(data[colIdx]);
    }

    inline std::string Reader::ReadString(int const &colIdx) const {
        return std::string(data[colIdx], lengths[colIdx]);
    }

    inline Data Reader::ReadData(int const &colIdx) const {
        return Data(data[colIdx], lengths[colIdx]);
    }

    template<typename T>
    inline void Reader::Read(int const &colIdx, T &outVal) const {
        // 前置检查
        if (colIdx < 0 || colIdx >= (int) info.numFields) {
            info.conn.Throw(__LINE__, xx::ToString("colIdx: ", colIdx, " out of range. numFields = ", info.numFields));
        }

        if constexpr (std::is_same_v<T, int>) {
            outVal = IsDBNull(colIdx) ? 0 : ReadInt32(colIdx);
        } else if constexpr (std::is_same_v<T, int64_t>) {
            outVal = IsDBNull(colIdx) ? 0 : ReadInt64(colIdx);
        } else if constexpr (std::is_same_v<T, double>) {
            outVal = IsDBNull(colIdx) ? 0 : ReadDouble(colIdx);
        } else if constexpr (std::is_same_v<T, std::optional<int>>) {
            if (IsDBNull(colIdx)) {
                outVal.reset();
            } else {
                outVal = ReadInt32(colIdx);
            }
        } else if constexpr (std::is_same_v<T, std::optional<int64_t>>) {
            if (IsDBNull(colIdx)) {
                outVal.reset();
            } else {
                outVal = ReadInt64(colIdx);
            }
        } else if constexpr (std::is_same_v<T, std::optional<double>>) {
            if (IsDBNull(colIdx)) {
                outVal.reset();
            } else {
                outVal = ReadDouble(colIdx);
            }
        } else if constexpr (std::is_same_v<T, std::string>) {
            outVal.assign(data[colIdx], lengths[colIdx]);
        } else if constexpr (std::is_same_v<T, std::optional<std::string>>) {
            if (IsDBNull(colIdx)) {
                outVal.reset();
            } else {
                outVal = std::string(data[colIdx], lengths[colIdx]);
            }
        } else if constexpr (std::is_same_v<T, Data>) {
            outVal.Clear();
            outVal.WriteBuf(data[colIdx], lengths[colIdx]);
        } else if constexpr (std::is_same_v<T, std::optional<std::string>>) {
            if (IsDBNull(colIdx)) {
                outVal.reset();
            } else {
                outVal = Data(data[colIdx], lengths[colIdx]);
            }
        } else {
            info.conn.Throw(__LINE__, xx::ToString("unhandled value type: ", typeid(T)));
        }
        // more?
    }

    template<typename T>
    inline void Reader::Read(char const *const &colName, T &outVal) {
        for (unsigned int i = 0; i < info.numFields; ++i) {
            if (strcmp(info.fields[i].name, colName) == 0) {
                Read(i, outVal);
                return;
            }
        }
        info.conn.Throw(__LINE__, std::string("could not find field name = ") + colName);
    }

    template<typename T>
    inline void Reader::Read(std::string const &colName, T &outVal) {
        Read(colName.c_str(), outVal);
    }

    // 一次填充多个
    template<typename...Args>
    inline void Reader::Reads(Args &...args) {
        int colIdx = 0;
        ReadsCore(colIdx, args...);
    }

    template<typename Arg, typename...Args>
    inline void Reader::ReadsCore(int &colIdx, Arg &arg, Args &...args) {
        Read(colIdx, arg);
        ReadsCore(++colIdx, args...);
    }

    inline void Reader::ReadsCore(int &colIdx) {
        (void) colIdx;
    }


    inline Connection::Connection(Connection &&o) noexcept {
        operator=(std::move(o));
    }

    inline Connection &Connection::operator=(Connection &&o) {
        std::swap(ctx, o.ctx);
        std::swap(errCode, o.errCode);
        std::swap(errText, o.errText);
        return *this;
    }

    inline Connection::~Connection() {
        Close();
    }

    inline void
    Connection::Open(std::string const &host, int const &port, std::string const &username, std::string const &password,
                     std::string const &db) {
        if (ctx) {
            Throw(-__LINE__, "Connection Open connection already opened.");
        }
        ctx = (MYSQL*)malloc(sizeof(MYSQL));
        if (!ctx) {
            Throw(-__LINE__, "Connection Open ctx malloc failed.");
        }
        void* tmp;
        {
            std::lock_guard<std::mutex> lg(mutex_connOpen);
            tmp = mysql_init(ctx);
        }
        assert(tmp == ctx);
        if (!tmp) {
            free(ctx);
            ctx = nullptr;
            Throw(-__LINE__, "Connection Open mysql_init failed.");
        }
        my_bool reconnect = 1;
        if (int r = mysql_options(ctx, MYSQL_OPT_RECONNECT, &reconnect)) {
            Throw(r, " Connection Open mysql_options error.");
        }
        // todo: 关 SSL 的参数, 限制连接超时时长
        if (!mysql_real_connect(ctx, host.c_str(), username.c_str(), password.c_str(), db.c_str(), port, nullptr,
                                CLIENT_MULTI_STATEMENTS)) {
            auto sg = xx::MakeScopeGuard([this] { Close(); });
            Throw((int) mysql_errno(ctx), mysql_error(ctx));
        }
    }

    inline void Connection::Close() {
        if (ctx) {
            mysql_close(ctx);
            free(ctx);
            ctx = nullptr;
        }
    }

    inline int Connection::Ping() {
        if (!ctx) {
            Throw(__LINE__, "connection is closed.");
        }
        return mysql_ping(ctx);
    }

    template<size_t len>
    void Connection::Execute(char const(&sql)[len]) {
        if (!ctx) {
            Throw(__LINE__, "connection is closed.");
        }
        if (mysql_real_query(ctx, sql, len - 1)) {
            Throw((int) mysql_errno(ctx), mysql_error(ctx));
        }
    }

    inline void Connection::Execute(std::string const &sql) {
        if (!ctx) {
            Throw(__LINE__, "connection is closed.");
        }
        if (mysql_real_query(ctx, sql.c_str(), (unsigned long)sql.size())) {
            Throw((int) mysql_errno(ctx), mysql_error(ctx));
        }
    }

    template<typename ...Args>
    void Connection::Execute(Args const &...args) {
        Execute(xx::ToString(args...));
    }

    inline void Connection::ClearResult() {
        while (Fetch(nullptr, nullptr)) {}
    }

    inline bool
    Connection::Fetch(std::function<bool(Info &)> &&infoHandler = nullptr,
                      std::function<bool(Reader &)> &&rowHandler = nullptr) {
        if (!ctx) {
            Throw(__LINE__, "connection is closed.");
        }

        Info info(*this);

        // 存储结果集
        auto &&res = mysql_store_result(ctx);

        // 有结果集
        if (res) {
            // 确保 res 在出这层 {} 时得到回收
            auto sgResult = xx::MakeScopeGuard([&] {
                mysql_free_result(res);
            });

            // 各种填充
            info.numFields = mysql_num_fields(res);
            info.numRows = mysql_num_rows(res);
            info.fields = mysql_fetch_fields(res);

            // 如果确定要继续读, 且具备读函数, 就遍历并调用
            if (rowHandler && (!infoHandler || (infoHandler && infoHandler(info)))) {
                Reader reader(info);
                while ((reader.data = mysql_fetch_row(res))) {
                    reader.lengths = mysql_fetch_lengths(res);
                    if (!reader.lengths) {
                        Throw((int) mysql_errno(ctx), mysql_error(ctx));
                    }
                    if (!rowHandler(reader)) break;
                }
            }
        }
            // 没有结果有两种可能：1. 真没有. 2. 有但是内存爆了网络断了之类. 用获取字段数量方式推断是否应该返回结果集
        else if (!mysql_field_count(ctx)) {
            // 存储受影响行数
            info.affectedRows = mysql_affected_rows(ctx);
            if (infoHandler) {
                infoHandler(info);
            }
        } else {
            // 有结果集 但是出错
            Throw((int) mysql_errno(ctx), mysql_error(ctx));
        }

        // n = 0: 有更多结果集.  -1: 没有    >0: 出错
        auto &&n = mysql_next_result(ctx);
        if (n == 0) return true;
        if (n == -1) return false;
        Throw((int) mysql_errno(ctx), mysql_error(ctx));
        return false;   // 让编译器闭嘴
    }

    template<typename ...Args>
    void Connection::FetchTo(Args &...args) {
        static_assert(sizeof...(args) > 0);
        // 可能存在多个结果集，并且前面几个可能都没有数据. 于是需要跳过 直到遇到一个有数据的，读一行，扫尾退出
        // 将出参的指针打包存 tuple
        auto &&tuple = std::make_tuple(&args...);
        bool filled = false;
        LabRetry:
        auto &&hasMoreResult = Fetch([&](Info &info) {
            // 跳过没有数据行的
            if (info.numRows) {
                filled = true;
                return true;
            }
            return false;
        }, [&](Reader &r) {
            // 将出参的指针tuple解包调函数
            std::apply([&](auto &... args) {
                r.Reads(*args...);
            }, tuple);
            // 只填充 1 行
            return false;
        });
        if (!filled) {
            if (hasMoreResult) goto LabRetry;
            Throw(__LINE__, "no data found, fetch failed.");
        }
        if (hasMoreResult) {
            ClearResult();
        }
    }

    template<typename ...Args>
    void Connection::FetchTo(std::tuple<Args...> &tuple) {
        bool filled = false;
        LabRetry:
        auto &&hasMoreResult = Fetch([&](Info &info) {
            if (info.numRows) {
                filled = true;
                return true;
            }
            return false;
        }, [&](Reader &r) {
            std::apply([&](auto &... args) {
                r.Reads(args...);
            }, tuple);
            return false;
        });
        if (!filled) {
            if (hasMoreResult) goto LabRetry;
            Throw(__LINE__, "no data found, fetch failed.");
        }
        if (hasMoreResult) {
            ClearResult();
        }
    }

    template<typename T>
    inline T Connection::FetchScalar() {
        T v{};
        FetchTo(v);
        return v;
    }

    template<typename T, typename ...Args>
    T Connection::ExecuteScalar(Args const &...args) {
        Execute(args...);
        return FetchScalar<T>();
    }

    inline std::vector<Result> Connection::FetchResults() {
        std::vector<Result> rtv;
        bool hasNextResult = false;
        do {
            auto &&result = rtv.emplace_back();
            hasNextResult = Fetch([&](xx::MySql::Info &info) -> bool {
                result.affectedRows = info.affectedRows;
                for (int i = 0; i < (int) info.numFields; ++i) {
                    result.columns.emplace_back(info.fields[i].name);
                }
                return true;
            }, [&](xx::MySql::Reader &reader) -> bool {
                auto &&row = result.rows.emplace_back();
                auto &&n = result.columns.size();
                for (size_t i = 0; i < n; ++i) {
                    row.emplace_back(reader.data[i]);
                }
                return true;
            });
        } while (hasNextResult);
        return rtv;
    }

    template<typename T>
    std::vector<T> Connection::FetchList() {
        std::vector<T> rtv;
        bool filled = false;
        LabRetry:
        auto &&hasMoreResult = Fetch([&](Info &info) {
            // 跳过没有数据列的（不一定有数据行）
            if (info.numFields) {
                filled = true;
                return true;
            }
            return false;
        }, [&](Reader &r) {
            r.Reads(rtv.emplace_back());
            return true;
        });
        if (!filled) {
            if (hasMoreResult) goto LabRetry;
            Throw(__LINE__, "execute no result, fetch failed.");
        }
        if (hasMoreResult) {
            ClearResult();
        }
        return rtv;
    }

    template<typename ...Args>
    std::vector<Result> Connection::ExecuteResults(Args const &...args) {
        Execute(args...);
        return FetchResults();
    }

    template<typename ...Args>
    my_ulonglong Connection::ExecuteNonQuery(Args const &...args) {
        Execute(args...);
        my_ulonglong rtv = 0;
        if (Fetch([&](Info &info) {
            rtv = info.affectedRows;
            return false;
        }, nullptr)) {
            ClearResult();
        }
        return rtv;
    }

    template<typename T, typename ...Args>
    std::vector<T> Connection::ExecuteList(Args const &...args) {
        Execute(args...);
        return FetchList<T>();
    }

    template<typename ...Args>
    void Connection::ExecuteTo(std::string const &sql, Args &...args) {
        Execute(sql);
        FetchTo(args...);
    }

    template<typename ...Args>
    void Connection::ExecuteTo(std::string const &sql, std::tuple<Args...> &tuple) {
        Execute(sql);
        FetchTo(tuple);
    }

    inline void Connection::Throw(int const &code, std::string &&desc) {
        errCode = code;
        errText = std::move(desc);
        throw std::runtime_error(errText);
    }
}
