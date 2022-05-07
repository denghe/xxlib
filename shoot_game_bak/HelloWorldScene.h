#pragma once
#include "cocos2d.h"
#include <array>
#include <optional>
#include <vector>
#include <memory>
#include <xx_asio_tcp_client_cpp.h>
#include <ss.h>
#include <xx_queue.h>

class MainScene : public cocos2d::Scene {
public:
	// singleton
	inline static MainScene* instance = nullptr;

	// 初始化
	bool init() override;

	// cocos 底层帧驱动
	void update(float delta) override;

	// 基础显示容器, 光标 以及 缩放
	float zoom = 0.5f;
	cocos2d::Node* container = nullptr;
	cocos2d::Node* ui = nullptr;
	cocos2d::Sprite* cursor = nullptr;

	// 输入映射( 每帧会判断并填充，形成 ControlState )
	cocos2d::Point mousePos;
	std::array<bool, 9> mouseKeys;
	std::array<bool, 166> keyboards;

	// 备份当前控制状态. 每次根据输入产生一个新的，并和这个比较，不一致就发包
	xx::Shared<SS_C2S::Cmd> cmd;

	// 累计时间池( 用来稳帧 )
	double totalDelta = 0;

	// 临时 timer 计数器
	double secs = 0;

	// 拨号客户端, 通信层
	xx::Asio::Tcp::Cpp::Client c;

	// 指向玩家自己的 clientId ( 收到 EnterResult 时填充 )
	uint32_t selfId = 0;
	// 指向玩家自己的 shooter( 收到 Sync 时定位填充 )
	SS::Shooter* self = nullptr;

	// 协程
	awaitable<void> Logic();

	// 协程配套 GUI 交互
	void DrawInit();
	void DrawResolve();
	void DrawDial();
	void DrawPlay();

	// 游戏上下文
	xx::Shared<SS::Scene> scene;

	// 每一帧 update 后的本地备份, 当收到的 events 小于 scene.frameNumber 时，从这里回滚
	xx::Queue<xx::Data> frameBackups;

	// 队列头部备份编号
	int frameBackupsFirstFrameNumber = 0;

	// 最大备份长度
	int maxBackupCount = 600;

	// 备份
	void Backup();

	// 还原到指定帧. 失败返回 非0
	int Rollback(int const& frameNumber);

	// 复位各种上下文
	void Reset();
};
