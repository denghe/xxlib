#pragma once
#include "xx_sqlite.h"
#include "xx_threadpool.h"
#include "server.h"

struct DB {
    Server* server;
    explicit DB(Server* server);

    // generic query func return value
    template<typename T>
    struct Rtv {
        T value {};

        bool success = true;
        operator bool() {
            return success;
        }

        int errorCode = 0;
        std::string errorMessage;

        template<typename U>
        explicit Rtv(U&& v){
            value = std::forward<U>(v);
        }

        Rtv() = default;
        Rtv(Rtv const&) = default;
        Rtv(Rtv&&) noexcept = default;
        Rtv& operator=(Rtv const&) = default;
        Rtv& operator=(Rtv&&) noexcept = default;
    };

    struct AccountInfo {
        XX_SIMPLE_STRUCT_DEFAULT_CODES(AccountInfo);
        int32_t accountId = -1;
        std::string nickname;
        double coin = 0;
    };

    struct Env {
        Env();
        void operator()(std::function<void(Env&)>& job);

        // easy access
        DB* db = nullptr;
        Server* server = nullptr;
        xx::ThreadPool2<Env, 1>* tp = nullptr;
        xx::Shared<xx::SQLite::Connection> conn;

        // try get accountId, nickname, coin from table: acc  by  username & password. value == -1: not found
        xx::Shared<xx::SQLite::Query> qTryGetAccountInfoByUsernamePassword;
        Rtv<AccountInfo> TryGetAccountInfoByUsernamePassword(std::string_view const& username, std::string_view const& password);

        // todo: more logic func here
    };
    xx::ThreadPool2<Env, 1> tp;

    // S->db->AddJob([ ...... ](DB::Env& env) mutable { ...... });
    template<typename F>
    void AddJob(F&& f) {
        tp.Add(std::forward<F>(f));
    }
};
