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

	// ��ʼ��
	bool init() override;

	// cocos �ײ�֡����
	void update(float delta) override;

	// ������ʾ����, ��� �Լ� ����
	float zoom = 0.5f;
	cocos2d::Node* container = nullptr;
	cocos2d::Node* ui = nullptr;
	cocos2d::Sprite* cursor = nullptr;

	// ����ӳ��( ÿ֡���жϲ���䣬�γ� ControlState )
	cocos2d::Point mousePos;
	std::array<bool, 9> mouseKeys;
	std::array<bool, 166> keyboards;

	// ���ݵ�ǰ����״̬. ÿ�θ����������һ���µģ���������Ƚϣ���һ�¾ͷ���
	xx::Shared<SS_C2S::Cmd> cmd;

	// �ۼ�ʱ���( ������֡ )
	double totalDelta = 0;

	// ��ʱ timer ������
	double secs = 0;

	// ���ſͻ���, ͨ�Ų�
	xx::Asio::Tcp::Cpp::Client c;

	// ָ������Լ��� clientId ( �յ� EnterResult ʱ��� )
	uint32_t selfId = 0;
	// ָ������Լ��� shooter( �յ� Sync ʱ��λ��� )
	SS::Shooter* self = nullptr;

	// Э��
	awaitable<void> Logic();

	// Э������ GUI ����
	void DrawInit();
	void DrawResolve();
	void DrawDial();
	void DrawPlay();

	// ��Ϸ������
	xx::Shared<SS::Scene> scene;

	// ÿһ֡ update ��ı��ر���, ���յ��� events С�� scene.frameNumber ʱ��������ع�
	xx::Queue<xx::Data> frameBackups;

	// ����ͷ�����ݱ��
	int frameBackupsFirstFrameNumber = 0;

	// ��󱸷ݳ���
	int maxBackupCount = 600;

	// ����
	void Backup();

	// ��ԭ��ָ��֡. ʧ�ܷ��� ��0
	int Rollback(int const& frameNumber);

	// ��λ����������
	void Reset();
};
