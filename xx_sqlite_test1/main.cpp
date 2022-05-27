// 尝试扫 sqlite 表结构, 生成 c++ boost multi index container 的相关代码, 实现快速内存数据库建模
#include "xx_sqlite.h"
#include "xx_string.h"

void TestSQLite() {
	xx::CoutN("test sqlite3 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");

	xx::SQLite::Connection conn;
	conn.Open(1 ? "test1.db3" : ":memory:");
	if (!conn) {
		xx::CoutN("xx::SQLite::Connection.Open( test1.db3 ) failed. error code = ", conn.lastErrorCode);
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

		xx::CoutN("begin select");
		xx::SQLite::Query qAccSelectCoinById(conn, "select coin from acc where id = ?");
		double totalCoin = 0;
		secs = xx::NowSteadyEpochSeconds();

		// * 2: 这里 故意使用 超范围的 不存在的 id 查询 多一倍次数
		for (i = 0; i < n * 2; ++i) {
			if (double coin; qAccSelectCoinById.SetParameters(i).ExecuteTo(coin)) {
				totalCoin += coin;
			}
		}
		xx::CoutN("select finished. i = ", i, ". secs = ", xx::NowSteadyEpochSeconds() - secs);
		xx::CoutN("totalCoin = ", totalCoin);

		// ...
	}
	catch (std::exception const& ex) {
		xx::CoutN("throw exception after conn.Open. ex = ", ex.what());
	}
}



#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>

using boost::multi_index_container;
using namespace boost::multi_index;

struct Acc {
	Acc() = delete;
	Acc(Acc const&) = delete;
	Acc& operator=(Acc const&) = delete;

	int64_t id;
	std::string username;
	std::string password;
	std::string nickname;
	double coin;

	template<typename A1, typename A2, typename A3, typename A4, typename A5>
	explicit Acc(A1&& a1, A2&& a2, A3&& a3, A4&& a4, A5&& a5)
		: id(std::forward<A1>(a1))
		, username(std::forward<A2>(a2))
		, password(std::forward<A3>(a3))
		, nickname(std::forward<A4>(a4))
		, coin(std::forward<A5>(a5))
	{}
};

namespace tags {
	struct id {};
	struct username {};
	struct nickname {};
}

typedef multi_index_container<Acc,indexed_by<
	ordered_unique<tag<tags::id>, BOOST_MULTI_INDEX_MEMBER(Acc, int64_t, id)>
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
		accs.emplace(i, upn, upn, upn, i * 100);
	}
	xx::CoutN("insert finished. i = ", i, ". secs = ", xx::NowSteadyEpochSeconds() - secs);

	xx::CoutN("acc.count(*) = ", accs.size());

	xx::CoutN("begin select");
	double totalCoin = 0;
	secs = xx::NowSteadyEpochSeconds();

	auto&& col = accs.get<tags::id>();
	// * 2: 这里 故意使用 超范围的 不存在的 id 查询 多一倍次数
	for (i = 0; i < n * 2; ++i) {
		if (auto r = col.find(i); r != col.end()) {
			totalCoin += r->coin;
		}
	}
	xx::CoutN("select finished. i = ", i, ". secs = ", xx::NowSteadyEpochSeconds() - secs);
	xx::CoutN("totalCoin = ", totalCoin);
}

int main() {
	xx::SQLite::Init();	// 开机必做
	TestSQLite();
	TestBoostMultiIndexContainer();
	return 0;
}
