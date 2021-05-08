#pragma once
#include "xx_sqlite.h"
#include "xx_threadpool.h"
#include "server.h"

struct DB {
    Server* server;
    explicit DB(Server* server);

    struct Env {
        Env();
        void operator()(std::function<void(Env&)>& job);

        // easy access
        DB* db = nullptr;
        Server* server = nullptr;
        xx::ThreadPool2<Env, 1>* tp = nullptr;
        xx::Shared<xx::SQLite::Connection> conn;


        // get account id by username & password. cb( accountId == -1: not found
        xx::Shared<xx::SQLite::Query> qGetAccountIdByUsernamePassword;
        int GetAccountIdByUsernamePassword(std::string_view const& username, std::string_view const& password);

        // todo: more logic func here
    };
    xx::ThreadPool2<Env, 1> tp;

    // S->db->AddJob([ ...... ](DB::Env& env) mutable { ...... });
    template<typename F>
    void AddJob(F&& f) {
        tp.Add(std::forward<F>(f));
    }
};
