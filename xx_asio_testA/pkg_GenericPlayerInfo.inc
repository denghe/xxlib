std::weak_ptr<VPeer> vpeer;	// 指向当前连接( 双向 bind ). vpeer 有个 指针存 this
bool isOnline = true;	// 是否在线。true: 位于 server.players 容器. 否则位于 unloadingPlayers
int64_t lastActiveTime = 0;		// 最后活动时间点
xx::Shared<Lobby_Client::Scene> scene;	// 属于当前玩家的独立场景
