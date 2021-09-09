#include "xx_signal.h"
#include "server.h"
#include "listener.h"

int main() {
	// 禁掉 SIGPIPE 信号避免因为连接关闭出错
	xx::IgnoreSignal();

	// 创建类实例
	Server s(1u << 15u);

    if (int r = s.Init()) return r;

	// 开始运行
	return s.Run();
}
