#pragma once
#include "xx_sqlite.h"
#include "xx_threadpool.h"
#include "server.h"

struct DB {
    explicit DB(Server* server);
    struct Env {
        Env();
        void operator()(std::function<void(Env&)>& job);

        Server* server = nullptr;
        xx::ThreadPool2<Env, 1>* tp = nullptr;
        xx::Shared<xx::SQLite::Connection> conn;


        // 共享：加持 & 封送
        template<typename T>
        xx::Ptr<T> ToPtr(xx::Shared<T> const& s) {
            return xx::Ptr<T>(s, [this](T **p) { server->Dispatch([p] { xx::Shared<T> o; o.pointer = *p; }); });
        }

        // 如果独占：不加持 不封送 就地删除
        template<typename T>
        xx::Ptr<T> ToPtr(xx::Shared<T> && s) {
            if (s.header()->useCount == 1) return xx::Ptr<T>(std::move(s), [this](T **p) { xx::Shared<T> o; o.pointer = *p; });
            else return xx::Ptr<T>(s, [this](T **p) { server->Dispatch([p] { xx::Shared<T> o; o.pointer = *p; }); });
        }


        xx::Shared<xx::SQLite::Query> qGetAccountIdByUsernamePassword;
        // get account id by username & password. cb( accountId == -1: not found
        void GetAccountIdByUsernamePassword(std::string_view const& username, std::string_view const& password
                                            , std::function<void(int accountId)>&& cb);

        // todo: more logic func here
    };
    xx::ThreadPool2<Env, 1> tp;
};
