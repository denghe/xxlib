/*
此乃 tcp 协议无关 纯转发 无状态 网关

网络包 由 包头 + 数据区 构成。
包头结构：数据长(4字节int)
    数据长：
        小尾 int

数据区结构: 收发id(4字节int) + 其他（ 序列号 + 类型编号 + 数据 ）
    收发id：
        client -> server 该值为 接收方id
        server -> client 该值为 发送方id

    序列号：
        为模拟 问答机制 而设置，发起方自己生成的不重复的自增int, 接收方的返回数据携带该值，以便发起方在收到后能通过该值 定位到相应的处理代码

    类型编号：
        跨协议唯一 id, 由 内部分组编号(1字节) + 协议类型编号(1字节) + 指令编号(2字节ushort) 构成
        内部分组编号：根据开发时商定的模块分组依据来编号
        协议类型编号：可简单定为 1: pb   2: json   3: pkg   ......
        指令编号：组内唯一的指令自增标识

    数据：具体协议通过 类型编号 来探测

网关 只关注 数据长 + 收发id ( 前八字节 )

数据大体流转过程：
    client --发送--> gateway ( 按 "接收方id" 定位到目标服务器连接, 并篡改 "收发id" 为 "客户端连接id" )  --转发--> servers...
    server --返回--> gateway ( 按 "客户端连接id" 定位到目标客户端连接, 并篡改 "收发id" 为 "服务编号" )  --转发--> clients...

详细主线流程：
    1. client --拨号--> gateway
    2. gateway --请求--> server_base              ( 有某IP的客户端连入，是否允许？ )
    3. server_base --回应--> gateway              ( 允许：开放 lobby 服务id；  不允许：踢掉？重定向？。。。 )
    4. gateway --发送--> client                   ( 开放 lobby 服务id )
    5. client --等待--                            ( 等待 服务id开放 通知 )
    6. client --发送--> gateway                   ( 发送 携带有 lobby 服务id 的包，登录？信息采集？。。。 )
    7. gateway --转发--> server_lobby             ( 参看 数据大体流转过程 )
    这步之后，lobby 可能根据玩家登录信息 定位到 相应上下文，并进一步核查状态（在游戏中？在大厅？。。)
    之后可能与相应的服务通信，例如 通知 游戏服 某玩家断线重联，当前位于哪个 gateway 什么 客户端连接id
    游戏收到后，通过类似上面的 3 号步骤，向客户端下发 “开放 游戏服务id”,
    客户端收到该通知之后，就可以凭借该 服务id, 与游戏服务通信 ( 期间也可通过 lobby 的服务id 同时与 lobby 通信 )
    如果玩家退出游戏，游戏服务原则上需要 立刻开始拒收 来自目标玩家 连接id 的数据，并 通知网关 关闭 目标玩家的 服务id，于是网关也可自动忽略相关数据

要点：
    client 欲向 某服务 发送数据， 必须先 等到对方 开放 服务id，否则数据将被忽略

另：
    client 如果从 gateway 断开，gateway 将产生 断线通知，广播给所有与该 client 相关联的 服务
    gateway 通常会有多个，都连接到所有内部服务
    所有 lobby 都连接到 base 服务( 多大厅方案 )
    所有 游戏服务 都连接到相应的 lobby 服务( 游戏分服方案 )
    base 服务 可能和 lobby 二合一( 单大厅简化登录流程方案 )
*/

#include "xx_signal.h"
#include "config.h"
#include "server.h"
#include "listener.h"
#include "pingtimer.h"
#include "dialer.h"
#include "tasktimer.h"

int main() {
	// 禁掉 SIGPIPE 信号避免因为连接关闭出错
	xx::IgnoreSignal();

    // 加载配置
    ajson::load_from_file(::config, "config.json");

    // 显示配置内容
    std::cout << ::config << std::endl;

	// 创建类实例
	Server s;

    // kcp 需要更高帧率运行以提供及时的响应
	s.SetFrameRate(100);

	// 开始运行
	return s.Run();
}
