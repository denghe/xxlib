// 尝试扫 sqlite 表结构, 生成 c++ boost multi index container 的相关代码, 实现快速内存数据库建模
#include "xx_sqlite.h"
#include "xx_string.h"

void TestSQLite() {
	xx::CoutN("test sqlite3 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");

	xx::SQLite::Connection conn;
	conn.Open(1 ? "test_sqlite_db.sqlite3" : ":memory:");
	if (!conn) {
		xx::CoutN("xx::SQLite::Connection.Open( test_sqlite_db.sqlite3 ) failed. error code = ", conn.lastErrorCode);
		return;
	}

	// 设置一组常用参数
	conn.SetPragmaSynchronousType(xx::SQLite::SynchronousTypes::Normal);	// Full Normal Off
	conn.SetPragmaJournalMode(xx::SQLite::JournalModes::WAL);
	conn.SetPragmaLockingMode(xx::SQLite::LockingModes::Exclusive);
	conn.SetPragmaTempStoreType(xx::SQLite::TempStoreTypes::Memory);
	// 如果是大size库, 可能还要关注 cache size page size mmapsize ...

	try {
		// 每次都删 免得改了字段 不生效
		if (conn.TableExists("acc")) {
			conn.Call("drop table acc");
		}

		// 建表
		conn.Call(R"#(
CREATE TABLE acc (
    id              int		primary key			not null,
    username        text                        not null,
    password        text                        not null,
    nickname        text                        not null,
    coin            float                       not null
)
)#");

		// 建索引
		conn.Call(R"#(
CREATE UNIQUE INDEX idx_unique_username on acc (username);
CREATE INDEX idx_coin on acc (coin);
)#");

		xx::CoutN("begin insert");
		xx::SQLite::Query qAccInsert(conn, "insert into acc(id, username, password, nickname, coin) values (?, ?, ?, ?, ?)");
		auto secs = xx::NowSteadyEpochSeconds();

		int n = 100000;
		int i = 0;
		// conn.BeginTransaction();
		for (; i < n; ++i) {
			auto upn = std::to_string(i);
			qAccInsert.SetParameters(i, upn, upn, upn, i * 100).Execute();
		}
		// conn.Commit();
		xx::CoutN("insert finished. i = ", i, ". secs = ", xx::NowSteadyEpochSeconds() - secs);


		auto numRows = conn.Execute<int64_t>("select count(*) from acc");
		xx::CoutN("acc.count(*) = ", numRows);


		xx::CoutN("begin select coin by id");
		xx::SQLite::Query qAccSelectCoinById(conn, "select coin from acc where id = ?");
		double totalCoin = 0;
		secs = xx::NowSteadyEpochSeconds();
		for (i = 0; i < n * 2; ++i) {
			if (double coin; qAccSelectCoinById.SetParameters(i).ExecuteTo(coin)) {
				totalCoin += coin;
			}
		}
		xx::CoutN("select finished. i = ", i, ". secs = ", xx::NowSteadyEpochSeconds() - secs);
		xx::CoutN("totalCoin = ", totalCoin);


		xx::CoutN("begin select coin by username");
		xx::SQLite::Query qAccSelectCoinByUsername(conn, "select coin from acc where username = ?");
		totalCoin = 0;
		secs = xx::NowSteadyEpochSeconds();
		for (i = 0; i < n * 2; ++i) {
			auto upn = std::to_string(i);
			if (double coin; qAccSelectCoinByUsername.SetParameters(upn).ExecuteTo(coin)) {
				totalCoin += coin;
			}
		}
		xx::CoutN("select finished. i = ", i, ". secs = ", xx::NowSteadyEpochSeconds() - secs);
		xx::CoutN("totalCoin = ", totalCoin);


		xx::CoutN("begin select top 3 most coin");
		xx::SQLite::Query qAccSelectTopCoin(conn, "select id, coin from acc order by coin desc limit ?");
		secs = xx::NowSteadyEpochSeconds();
		std::vector<std::pair<int64_t, double>> top3;
		for (i = 0; i < n; ++i) {
			top3.clear();
			qAccSelectTopCoin.SetParameters(3).Execute([&](auto& r) {
				auto& o = top3.emplace_back();
				r.Reads(o.first, o.second);
				return 0;
			});
		}
		xx::CoutN("select finished. i = ", i, ". secs = ", xx::NowSteadyEpochSeconds() - secs);
		xx::CoutN(top3);
	}
	catch (std::exception const& ex) {
		xx::CoutN("throw exception after conn.Open. ex = ", ex.what());
	}
}



#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>

using boost::multi_index_container;
using namespace boost::multi_index;

struct Acc {
	int64_t id;
	std::string username;
	std::string password;
	std::string nickname;
	double coin;
};

namespace tags {
	struct id {};
	struct username {};
	struct coin {};
}

typedef multi_index_container<Acc,indexed_by<
	hashed_unique<tag<tags::id>, BOOST_MULTI_INDEX_MEMBER(Acc, int64_t, id)>,
	hashed_unique<tag<tags::username>, BOOST_MULTI_INDEX_MEMBER(Acc, std::string, username)>,
	ordered_non_unique<tag<tags::coin>, BOOST_MULTI_INDEX_MEMBER(Acc, double, coin)>
>> Accs;

void TestBoostMultiIndexContainer() {
	xx::CoutN("test multi_index_container ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
	Accs accs;

	xx::CoutN("begin insert");
	auto secs = xx::NowSteadyEpochSeconds();

	int n = 100000;
	int i = 0;
	for (; i < n; ++i) {
		auto upn = std::to_string(i);
		accs.emplace(Acc{ i, upn, upn, upn, (double)i * 100 });
	}
	xx::CoutN("insert finished. i = ", i, ". secs = ", xx::NowSteadyEpochSeconds() - secs);


	xx::CoutN("acc.count(*) = ", accs.size());


	xx::CoutN("begin select coin by id");
	double totalCoin = 0;
	secs = xx::NowSteadyEpochSeconds();
	auto&& col_id = accs.get<tags::id>();
	for (i = 0; i < n * 2; ++i) {
		if (auto r = col_id.find(i); r != col_id.end()) {
			totalCoin += r->coin;
		}
	}
	xx::CoutN("select finished. i = ", i, ". secs = ", xx::NowSteadyEpochSeconds() - secs);
	xx::CoutN("totalCoin = ", totalCoin);


	xx::CoutN("begin select coin by username");
	totalCoin = 0;
	secs = xx::NowSteadyEpochSeconds();
	auto&& col_username = accs.get<tags::username>();
	for (i = 0; i < n * 2; ++i) {
		auto upn = std::to_string(i);
		if (auto r = col_username.find(upn); r != col_username.end()) {
			totalCoin += r->coin;
		}
	}
	xx::CoutN("select finished. i = ", i, ". secs = ", xx::NowSteadyEpochSeconds() - secs);
	xx::CoutN("totalCoin = ", totalCoin);


	xx::CoutN("begin select top 3 most coin");
	secs = xx::NowSteadyEpochSeconds();
	auto&& col_coin = accs.get<tags::coin>();
	std::vector<std::pair<int64_t, double>> top3;
	for (i = 0; i < n; ++i) {
		top3.clear();
		auto iter = col_coin.rbegin();
		for (int j = 0; j < 3; ++j) {
			if (iter == col_coin.rend()) break;
			top3.emplace_back(iter->id, iter->coin);
            ++iter;
		}
	}
	xx::CoutN("select finished. i = ", i, ". secs = ", xx::NowSteadyEpochSeconds() - secs);
	xx::CoutN(top3);
}

int main() {
	xx::SQLite::Init();	// 开机必做
	TestSQLite();
	TestBoostMultiIndexContainer();
    std::cin.get();
	return 0;
}
