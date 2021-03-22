#include "xx_mysql.h"
#include "xx_threadpool.h"

struct Conn : xx::MySql::Connection {
	Conn() {
		try {
			Open("192.168.1.135", 3306, "root", "1", "test1");
		}
		catch (std::exception const& e) {
			xx::CoutN("e.what() = ", e.what(), "conn.errCode = ", errCode);
		}
	}

	// for thread pool
	int threadId = 0;
	void operator()(std::function<void(Conn&)>& job) {
		try {
			job(*this);
		}
		catch (int const& e) {
			xx::CoutN("threadId = ", threadId, ": catch throw int: ", e);
		}
		catch (std::exception const& e) {
			xx::CoutN("threadId = ", threadId, ": catch throw std::exception: ", e.what());
		}
		catch (...) {
			xx::CoutN("threadId = ", threadId, ": catch ...");
		}
	}

};

// 单线程测试

void Test1() {
	Conn conn;
	if (!conn.ctx) return;

	try {

		{
			xx::CoutN(1, " open");
			if (!conn.ctx) {
				xx::CoutN("open failed");
				return;
			}

			xx::CoutN('\n', 2, " cleanup");
			// 粗暴清除数据
			conn.Execute(R"(
set foreign_key_checks = 0;
truncate table `acc_log`;
truncate table `acc`;
truncate table `dd`;
set foreign_key_checks = 1;
)");
			conn.ClearResult();
		}
		{
			xx::CoutN('\n', 3, " insert");
			// 模拟账号创建, 输出受影响行数
			auto&& affectedRows = conn.ExecuteNonQuery(R"(
insert into `acc` (`id`, `money`)
values
 (1, 0)
,(2, 0)
,(3, 0)
)");
			xx::CoutN("affectedRows = ", affectedRows);
		}
		int64_t tar_acc_id = 2;
		int64_t tar_add_money = 100;
		{
			xx::CoutN('\n', 4, " call sp");
			// 调用存储过程加钱   rtv：0 成功   -1 参数不正确   -2 找不到acc_id   -3 日志插入失败
			auto&& rtv = conn.ExecuteScalar<int>("CALL `sp_acc_add_money`(", tar_acc_id, ", ", tar_add_money, ", ",
				xx::NowEpoch10m(), ", @rtv); SELECT @rtv;");
			if (rtv) throw std::logic_error(xx::ToString("call sp: `sp_acc_add_money` return value : ", rtv));
		}
		{
			xx::CoutN('\n', 5, " show one row");
			conn.Execute("select `id`, `money` from `acc` where `id` = ", tar_acc_id);
			int64_t id, money;
			conn.FetchTo(id, money);
			xx::CoutN("id, money = ", id, ", ", money);
		}
		{
			xx::CoutN('\n', 6, " show id list");
			auto&& results = conn.ExecuteList<int64_t>("select `id` from `acc`");
			xx::CoutN(results);
		}
		{
			xx::CoutN('\n', 7, " fill to struct");
			conn.Execute(
				"select `id`, `money` from `acc`; select `acc_id`, `money_before`, `money_add`, `money_after`, `create_time` from `acc_log`;");
			struct Acc {
				int64_t id, money;
			};
			struct AccLog {
				int64_t acc_id, money_before, money_add, money_after, create_time;
			};
			std::vector<Acc> accs;
			std::vector<AccLog> acclogs;
			conn.Fetch(nullptr, [&](xx::MySql::Reader& reader) -> bool {
				auto&& o = accs.emplace_back();
				reader.Reads(o.id, o.money);
				return true;
				});
			conn.Fetch(nullptr, [&](xx::MySql::Reader& reader) -> bool {
				auto&& o = acclogs.emplace_back();
				reader.Reads(o.acc_id, o.money_before, o.money_add, o.money_after, o.create_time);
				return true;
				});
			xx::CoutN("accs.size() = ", accs.size());
			for (auto&& o : accs) {
				xx::CoutN("id = ", o.id, " money = ", o.money);
			}
			xx::CoutN("acclogs.size() = ", acclogs.size());
			for (auto&& o : acclogs) {
				xx::CoutN("acc_id = ", o.acc_id, " money_before = ", o.money_before, " money_add = ", o.money_add,
					" money_after = ", o.money_after, " create_time = ", o.create_time);
			}
		}
		{
			xx::CoutN('\n', 8, " direct show all");
			auto&& results = conn.ExecuteResults("select * from `acc`; select * from `acc_log`;");
			xx::CoutN(results);
		}
		{
			xx::CoutN('\n', 9, " test tuple");
			std::tuple<int, std::string> t;
			conn.ExecuteTo("select 1, 'asdf'", t);
			xx::CoutN(t);
		}
		{
			xx::CoutN('\n', 10, " test blob");
			auto r = conn.ExecuteNonQuery("insert into `dd` (`id`, `data`) values (1, '", xx::MySql::EscapeWrapper("abcde", 5), "')");
			xx::CoutN("insert blob affected rows = ", r);
			xx::Data d;
			conn.ExecuteTo("select `data` from `dd` where `id` = 1", d);
			xx::CoutN("d = ", d);
		}
	}
	catch (std::exception const& ex) {
		xx::CoutN("ex.what() = ", ex.what(), " e.conn.errCode = ", conn.errCode);
	}
}





// 线程池测试



void Test2() {
	{
		// 创建线程池. 4 个线程
		xx::ThreadPool2<Conn, 4> tp;
		int tid = 0;
		for (auto& c : tp.envs) {
			if (!c.ctx) {
				xx::CoutN("tp init failed.");
				return;
			}
			c.threadId = tid++;
		}

		// 模拟并行插入. 受 test1 影响，应该会出错 3 次
		for (size_t i = 0; i < 10000; i++) {
			tp.Add([i = i](Conn& conn) {
				conn.ExecuteNonQuery("insert into `acc` (`id`, `money`) values (", i, ", 0)");
				});
		}
	}
	{
		Conn conn;
		if (!conn.ctx) return;
		try {
			auto&& count = conn.ExecuteScalar<int>("select count(*) from `acc`");
			xx::CoutN("multi thread insert end. count = ", count);
		}
		catch (std::exception const& ex) {
			xx::CoutN("ex.what() = ", ex.what(), " e.conn.errCode = ", conn.errCode);
		}
	}
}



// 指定 线程 执行测试（业务上可能需要确保 同一个用户被分配到相同线程执行 SQL 以确保业务逻辑正确性 )

void Test3() {
	{
		// 创建线程池数组. 长度 4. 每个池 1 个线程
		std::array<xx::ThreadPool2<Conn, 1>, 4> tps;
		int tid = 0;
		for (auto& tp : tps) {
			for (auto& c : tp.envs) {
				if (!c.ctx) {
					xx::CoutN("tp init failed.");
					return;
				}
				c.threadId = tid++;
			}
		}

		// 模拟有序并行插入
		for (size_t i = 10001; i < 20000; i++) {
			// 根据 i 取模 选池
			auto& tp = tps[i % tps.size()];
			tp.Add([i = i](Conn& conn) {
				conn.ExecuteNonQuery("insert into `acc` (`id`, `money`) values (", i, ", 0)");
				});
		}
	}
	{
		Conn conn;
		if (!conn.ctx) return;
		try {
			auto&& count = conn.ExecuteScalar<int>("select count(*) from `acc`");
			xx::CoutN("multi thread insert end. count = ", count);
		}
		catch (std::exception const& ex) {
			xx::CoutN("ex.what() = ", ex.what(), " e.conn.errCode = ", conn.errCode);
		}
	}
}

int main() {
	Test1();
	Test2();
	Test3();
	return 0;
}
