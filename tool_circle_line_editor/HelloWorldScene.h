#pragma once
#include "cocos2d.h"
#include <filesystem>
#include "xx_helpers.h"

struct Circle;
struct DraggingRes;

#include "ajson.hpp"

// json �浵��ʽ. �� .lines �У�r �뾶ֵû����. ��ǰΪ�򻯣����洢Ϊͳһ��ʽ
struct Item {
    XX_SIMPLE_STRUCT_DEFAULT_CODES(Item);
    float x, y, r;
};
AJSON(Item, x, y, r);
struct Data {
    XX_SIMPLE_STRUCT_DEFAULT_CODES(Data);
    std::vector<Item> items;
};
AJSON(Data, items);

enum class PicTypes {
    SpriteFrame, Spine, C3b
};

// ͼ�ڵ���Ϣ
struct PicInfo {
    XX_SIMPLE_STRUCT_DEFAULT_CODES(PicInfo);

    // ������������
    std::vector<PicInfo*>* container = nullptr;

    // ָ����һ������һ�� ( ��һ������һ��ָ�����һ��, ���һ������һ��ָ���һ�� )
    PicInfo* prev = nullptr, *next = nullptr;

    // λ��������±�ֵ
    int idx = -1;

    bool IsFirst() const { return idx == 0; }
    bool IsLast() const { return idx + 1 == container->size(); }

    // plist: sprite frame name
    // spine / c3b: fileName + "_" + actionName + "_" + idx
    std::string name;

    // 0: sprite frame     1: spine     2: c3b
    PicTypes type = PicTypes::SpriteFrame;

    // �ļ���������չ����
    std::string fileName;
    std::string fileName2;
    // ������
    std::string actionName;
    // ��ʼʱ���
    float timePoint = 0.f;
};

#define SAVE_FILE_EXT_CIRCLES ".circles"
#define SAVE_FILE_EXT_LINES ".lines"

struct HelloWorld : cocos2d::Scene {
    using BaseType = cocos2d::Scene;
    using BaseType::BaseType;
    bool init() override;
    void onEnter() override;
    void onExit() override;
    void onDrawImGui();

    // ��������ʱ�����Ƿ�ɹ�
    bool ok = false;

    // ok == false ��������
    std::string errorMessage;

    // ��������ʱ��� pics & picGroups���� Resources Ŀ¼�� *.plist, *.atlas, *.c3b
    // 
    // first: sprite frame name       second: plist file name
    std::vector<PicInfo> allPics;

    // key: ���˵����ֲ��ֵľ���֡��
    std::map<std::string, std::vector<PicInfo*>> picGroups;

    // ��ǰ���ݴ浵��չ���� �� .lines ֮���л� )
    std::string currExt = SAVE_FILE_EXT_CIRCLES;

    // ��ǰѡ�е�ͼ
    PicInfo* currPic;

    // ��ǰѡ�е�ͼ��Բ����
    Data currData;


    // ���� AppDelegate::designWidth/Height
    float W = 0, H = 0, W_2 = 0, H_2 = 0;

    // ��Ƴߴ����
    float DW = 1280, DH = 720, DW_2 = DW / 2, DH_2 = DH / 2;

    // touch ״̬��¼
    cocos2d::Vec2 lastPos;
    bool touching = false;
    Circle* currCircle = nullptr;

    // ͼ�������� ����ֵ
    float zoom = 1.0f;

    // ָ��ͼ������������
    cocos2d::Node* nodeCircles = nullptr;

    // ��ǰ����������
    void update(float delta) override;

    // �Ƿ����ڲ���
    bool playing = false;
    // ��ǰ����֡�ӳ�
    float playingFrameDelay = 1.f / 60.f;
    // ʱ���( ��֡�� )
    float playingTimePool = 0;
    // ����ǰ����
    PicInfo* bak_currPic = nullptr;

    // ����һ��Բ�ڵ㵽 nodeCircles
    Circle* CreateCircle(cocos2d::Vec2 const& pos, float const& r);

    // �� nodeCircles ��ȡ Circle* ����
    std::vector<Circle*> GetCircles();

    // ����
    void Play(float const& delaySecs);

    // ���� currPic ��ʾ�༭����
    void Draw();

    // �������ݵ� pics. �����Ƿ� ok
    bool LoadPics();

    // �������ݵ� currData. �ɹ� 0
    int Load();

    // �洢 currData Ϊ currPic + ".circle" �ļ���
    void Save();

    // ����ͼ����
    void MoveUp();

    // ����ͼ����
    void MoveDown();

    // ����һ��ͼ����Բ
    void CopyPrevCircles();

    // �����ǰͼ��Բ
    void Clear();

    // ���� 2d ֡ͼ( ê�� ���� ). ��ʾΪָ�� siz ��С
    cocos2d::Sprite* CreateSpriteFrame(cocos2d::Vec2 const& pos, cocos2d::Size const& siz, std::string const& spriteFrameName, cocos2d::Node* const& container = nullptr);
};
