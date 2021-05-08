#include "db.h"
#include "xx_logger.h"

DB::DB(Server *server) {
    tp.envs[0].server = server;
    tp.envs[0].tp = &tp;
}

void DB::Env::operator()(std::function<void(DB::Env &)> &job) {
    try {
        job(*this);
    }
    catch (int const &e) {
        LOG_ERR("catch (int const& e) e = ", e);
    }
    catch (std::exception const &e) {
        LOG_ERR("std::exception const& e", e.what());
    }
    catch (...) {
        LOG_ERR("catch (...)");
    }
}

DB::Env::Env() {
    conn.Emplace("data.db3");
    if (!*conn) {
        throw std::runtime_error("the database file can't open.");
    }
    if (conn->GetTableCount() == 0) {
        conn->Call(R"#(
CREATE TABLE player (
    id              int         primary key     not null,
    username        text                        not null,
    password        text                        not null
)
)#");
    }
    if (conn->Execute<int64_t>("select count(*) from player") == 0) {
        conn->Call(R"#(
insert into player(id, username, password) values (1, "a", "a")
)#");
        conn->Call(R"#(
insert into player(id, username, password) values (2, "b", "b")
)#");
    }
}

void DB::Env::GetAccountIdByUsernamePassword(std::string_view const &username, std::string_view const &password
                                             , std::function<void(int accountId)> &&cb) {
    tp->Add([this, u = std::string(username), p = std::string(password), cb = std::move(cb)](Env &env) mutable {
        auto &q = env.qGetAccountIdByUsernamePassword;
        if (!q) {
            q.Emplace(*env.conn, "select id from player where username = ? and password = ?");
        }
        q->SetParameters(u, p);
        int aid;
        if (!q->ExecuteTo(aid)) {
            aid = -1;
        }
        server->Dispatch([aid, cb = std::move(cb)] {
            cb(aid);
        });
    });
}
