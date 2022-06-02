#pragma once
#include "xx_sqlite.h"
#include "pkg.h"

struct DB {
	constexpr static const std::string_view dbName = "xx_asio_testd_db.sqlite3";

	// ��Ҫ�� main ��һʱ��ִ��, ������ɶ�Ļ�������
	inline static void MainInit() {
		xx::SQLite::Connection conn;
		conn.Open(dbName);	// NoMutex ���ö��̲߳���ģʽ
		assert(conn);
		// ÿ�ζ�ɾ ��ø����ֶ� ����Ч
		if (!conn.TableExists("acc")) {
			// ����
			conn.Call(R"#(
CREATE TABLE acc (
    id              int		primary key			not null,
    username        text                        not null,
    password        text                        not null,
    nickname        text                        not null,
    gold            int                         not null
)
)#");
			// ������
			conn.Call(R"#(
CREATE UNIQUE INDEX hashed_unique_acc_username on acc (username);
CREATE INDEX ordered_non_unique_acc_gold on acc (gold);
)#");
		}

		// ����һЩ��������
		if (auto numRows = conn.Execute<int64_t>("select count(*) from acc"); !numRows) {
			xx::SQLite::Query qAccInsert(conn, "insert into acc(id, username, password, nickname, gold) values (?, ?, ?, ?, ?)");
			qAccInsert.SetParameters(1, "un1", "pw1", "nn1", 100).Execute();
			qAccInsert.SetParameters(2, "un2", "pw2", "nn2", 200).Execute();
			qAccInsert.SetParameters(3, "un3", "pw3", "nn3", 300).Execute();
		}
	}

	xx::SQLite::Connection conn;
	xx::SQLite::Query qAccSelectIdByUsernamePassword;
	xx::SQLite::Query qAccSelectRowById;
	xx::SQLite::Query qAccUpdatePlayerGoldById;
	// ...

	DB()
		: qAccSelectIdByUsernamePassword(conn)
		, qAccSelectRowById(conn)
		, qAccUpdatePlayerGoldById(conn)
		// ...
	{
		using namespace xx::SQLite;
		conn.Open(dbName, OpenFlags::ReadWrite | OpenFlags::Create | OpenFlags::Wal | OpenFlags::NoMutex);	// NoMutex ���ö��̲߳���ģʽ
		assert(conn);

		qAccSelectIdByUsernamePassword.SetQuery("select id from acc where username = ? and password = ?");
		qAccSelectRowById.SetQuery("select id, username, password, nickname, gold from acc where id = ?");
		qAccUpdatePlayerGoldById.SetQuery("update acc set gold = ? where id = ?");
		// ...
	}

	int64_t GetPlayerIdByUsernamePassword(std::string_view un, std::string_view pw) {
		int64_t id = 0;
		qAccSelectIdByUsernamePassword.SetParameters(un, pw).ExecuteTo(id);
		return id;
	}

	auto GetPlayerInfoById(int64_t id) {
		auto r = xx::Make<Generic::PlayerInfo>();
		if (!qAccSelectRowById.SetParameters(id).ExecuteTo(r->id, r->username, r->password, r->nickname, r->gold)) {
			r.Reset();
		}
		return r;
	}
	
	bool UpdatePlayerGoldById(int64_t id, int64_t gold) {
		qAccUpdatePlayerGoldById.SetParameters(id, gold).Execute();
		return conn.GetAffectedRows64() == 1;
	}

	// ...
};
