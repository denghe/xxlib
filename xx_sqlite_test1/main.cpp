// 尝试扫 sqlite 表结构, 生成 c++ boost multi index container 的相关代码, 实现快速内存数据库建模
#include "xx_sqlite.h"
#include "xx_string.h"
#include "boost/multi_index_container.hpp"

int main() {
	xx::SQLite::Init();
	xx::SQLite::Connection conn;
	conn.Open("test1.db3");
	if (!conn) {
		xx::CoutN("xx::SQLite::Connection.Open( test1.db3 ) failed. error code = ", conn.lastErrorCode);
		return __LINE__;
	}

	conn.SetPragmaSynchronousType(xx::SQLite::SynchronousTypes::Normal);	// Full Normal Off
	conn.SetPragmaJournalMode(xx::SQLite::JournalModes::WAL);
	conn.SetPragmaLockingMode(xx::SQLite::LockingModes::Exclusive);
	conn.SetPragmaTempStoreType(xx::SQLite::TempStoreTypes::Memory);
	//conn.SetPragmaCacheSize(400000);
	//conn.SetPragmaSchemaAutoVacuum(0);
	try {
		if (!conn.TableExists("acc")) {
			xx::CoutN("table: acc not found. creat");
			conn.Call(R"#(
CREATE TABLE acc (
    id              int							not null,
    username        text                        not null,
    password        text                        not null,
    nickname        text                        not null,
    coin            float                       not null
)
)#");
		}

		auto numRows = conn.Execute<int64_t>("select count(*) from acc");
		if (numRows) {
			xx::CoutN("acc.count(*) == ", numRows, "\r\nclean up");
			conn.TruncateTable("acc", false);
		}
		xx::CoutN("acc.insert(...)");
		xx::SQLite::Query qAccInsert(conn, "insert into acc(id, username, password, nickname, coin) values (?, ?, ?, ?, ?)");
		auto secs = xx::NowSteadyEpochSeconds();
		//conn.BeginTransaction();
		int i = 0;
		for (; i < 100000; ++i) {
			auto upn = std::to_string(i);
			qAccInsert.SetParameters(i, upn, upn, upn, i * 100).Execute();
		}
		// todo: 给 id 建索引
		//conn.Commit();

		xx::CoutN("random insert ", i, " rows. secs = ", xx::NowSteadyEpochSeconds() - secs);

		numRows = conn.Execute<int64_t>("select count(*) from acc");
		xx::CoutN("acc.count(*) == ", numRows, "");
	}
	catch (std::exception const& ex) {
		xx::CoutN("throw exception after conn.Open. ex = ", ex.what());
	}
	return 0;
}
