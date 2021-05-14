#include "xx_signal.h"
#include "config.h"
#include "server.h"
#include "glistener.h"
#include "timer.h"
#include "gpeer.h"
#include "vpeer.h"
#include "dbdialer.h"
#include "dbpeer.h"
#include "ldialer.h"
#include "lpeer.h"

int main() {
	// 禁掉 SIGPIPE 信号避免因为连接关闭出错
	xx::IgnoreSignal();

    // 加载配置
    ajson::load_from_file(::config, "config.json");

    // 显示配置内容
    std::cout << ::config << std::endl;

	// 创建类实例
	Server s;

    if (int r = s.Init()) return r;

	// 开始运行
	return s.Run();
}
