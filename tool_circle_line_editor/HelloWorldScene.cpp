#pragma execution_character_set("utf-8")
#include "AppDelegate.h"
#include "HelloWorldScene.h"
#include <spine/spine-cocos2dx.h>
#include "ImGuiEXT/CCImGuiEXT.h"
#include <filesystem>
#include "xx_string.h"

// todo:
// 
// 提供 .circles .lines 到当前所用 .anims 等动画文件格式的 填充、转换 命令行工具


// 将 string 里数字部分移除
inline std::string RemoveNumbers(std::string const& s) {
	std::string t;
	for (auto&& c : s) {
		if (c >= '0' && c <= '9') continue;
		t += c;
	}
	return t;
}

// 加载 json 文件到 o
template<typename T>
static int LoadJson(T& o, std::string const& fn) {
	char const* buf = nullptr;
	size_t len = 0;
	auto d = cocos2d::FileUtils::getInstance()->getDataFromFile(fn);
	// 错误检查
	if (d.getSize() == 0) return -1;
	buf = (char*)d.getBytes();
	len = d.getSize();
	ajson::load_from_buff(o, buf, len); // try catch?
	return 0;
}

// 圆绘制控件
struct Circle : public cocos2d::Node {
	// 指向总上下文，方便取显示参数
	HelloWorld* scene = nullptr;

	// 用于绘制的节点( 直接继承它似乎会导致出现绘制 bug )
	cocos2d::DrawNode* drawNode = nullptr;

	cocos2d::Color4F highlightColor = cocos2d::Color4F::BLUE;
	cocos2d::Color4F defaultColor = cocos2d::Color4F::RED;
	cocos2d::Color4F lineColor = cocos2d::Color4F::GREEN;
	cocos2d::Color4F color = cocos2d::Color4F::RED;

	// true: circle 变 point. 半径不可改变. 自动查找 上一个 point, 连线
	bool pointMode = false;

	// 当前半径
	float r = 0;

	// 键盘改变半径的 delta 乘法系数
	float rChange = 0;

	// 上次 touch 位置
	cocos2d::Vect lastPos;

	// 是否正在 touch
	bool touching = false;

	using cocos2d::Node::Node;
	static Circle* create(bool pointMode = false) {
		auto o = new (std::nothrow) Circle();
		if (o) {
			o->pointMode = pointMode;
		}
		if (o && o->init()) {
			o->autorelease();
			return o;
		}
		CC_SAFE_DELETE(o);
		return nullptr;
	}

	// 初始化 touch event listener, 实现拖拽功能. 拖到某区域就是删除
	bool init() override {
		auto touchListener = cocos2d::EventListenerTouchOneByOne::create();
		touchListener->setSwallowTouches(true);
		touchListener->onTouchBegan = [this](cocos2d::Touch* t, cocos2d::Event* e) {
			auto&& tL = t->getLocation();
			auto&& p = convertToNodeSpace(tL);
			// 如果点击在圈内则认为选中
			if (p.distance({ 0,0 }) <= r) {
				touching = true;
				scene->currCircle = this;
				lastPos = p;
				color = highlightColor;
				Draw(r);
				return true;
			}
			return false;
		};
		touchListener->onTouchMoved = [this](cocos2d::Touch* t, cocos2d::Event* e) {
			if (!touching) return false;
			auto&& tL = t->getLocation();
			auto&& p = convertToNodeSpace(tL);
			auto&& diff = p - lastPos;
			setPosition(getPosition() + diff);
			if (pointMode) {
				Draw(r);
			}
			return true;
		};
		touchListener->onTouchEnded = [this](cocos2d::Touch* t, cocos2d::Event* e) {
			if (!touching) return false;
			touching = false;
			scene->currCircle = nullptr;
			color = defaultColor;
			Draw(r);
			auto&& p = getPosition();
			//// 如果拖拽到区域外就自杀
			//if (p.x < -scene->DW_2 || p.x > scene->DW_2 || p.y < -scene->DH_2 || p.y > scene->DH_2) {
			//	removeFromParentAndCleanup(true);
			//}
			scene->Save();
			return true;
		};
		touchListener->onTouchCancelled = touchListener->onTouchEnded;
		cocos2d::Director::getInstance()->getEventDispatcher()->addEventListenerWithSceneGraphPriority(touchListener, this);

		drawNode = cocos2d::DrawNode::create();
		addChild(drawNode);

		scheduleUpdate();
		return true;
	}

	// 根据 r 半径更新显示
	void Draw(float const& r) {
		this->r = r;
		drawNode->clear();
		if (pointMode) {
			DrawLine();
			drawNode->drawSolidCircle({ 0,0 }, r, 0, 100, color);
		}
		else {
			drawNode->drawCircle({ 0,0 }, r, 0, 100, false, color);
		}
	}

	// 设置默认颜色并修改当前颜色
	void SetColor(cocos2d::Color4F const& c) {
		color = defaultColor = c;
		Draw(r);
	}

	// 找到上一个 Circle 与其连线( pointMode == true )
	void DrawLine() {
		// 过滤出所有同级
		auto ncs = scene->GetCircles();

		// 如果是第一个就直接退出
		if (ncs[0] == this) return;

		// 找到自己的下标
		size_t i = 1;
		for (; i < ncs.size(); ++i) {
			if (ncs[i] == this) break;
		}

		// 定位到上一个
		if (i > 1) {
			auto&& p = ncs[i - 1];
			auto&& pPos = p->getPosition();
			drawNode->drawLine({ 0,0 }, pPos - this->getPosition(), lineColor);
		}

		// 定位到下一个
		if (i + 1 < ncs.size()) {
			auto&& p = ncs[i + 1];
			auto&& pPos = p->getPosition();
			p->drawNode->clear();
			p->drawNode->drawSolidCircle({ 0,0 }, p->r, 0, 100, p->color);
			p->drawNode->drawLine({ 0,0 }, this->getPosition() - pPos, lineColor);
		}
	}

	// 响应键盘按下 + - 不断改变大小的效果
	void update(float delta) override {
		if (rChange != 0) {
			r += delta * rChange;
			if (rChange < 0 && r < 1) {
				r = 1;
			}
			Draw(r);
		}
	}
};

std::vector<Circle*> HelloWorld::GetCircles() {
	std::vector<Circle*> ncs;
	for (auto&& n : nodeCircles->getChildren()) {
		if (auto&& nc = dynamic_cast<Circle*>(n)) {
			ncs.push_back(nc);
		}
	}
	return ncs;
}

bool HelloWorld::init() {
	if (!this->BaseType::init()) return false;
	scheduleUpdate();

	{
		W = AppDelegate::designWidth;
		H = AppDelegate::designHeight;
		W_2 = W / 2;
		H_2 = H / 2;
	}
	{
		nodeCircles = cocos2d::Node::create();
		nodeCircles->setPosition(450 + (AppDelegate::designWidth - 450) / 2, (AppDelegate::designHeight - 100) / 2);
		addChild(nodeCircles);
	}
	{
		auto kbListener = cocos2d::EventListenerKeyboard::create();
		kbListener->onKeyPressed = [this](cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event) {
			if (keyCode == cocos2d::EventKeyboard::KeyCode::KEY_Z) {
				if (zoom >= 3.0f) return;
				zoom += 0.1f;
				nodeCircles->setScale(zoom);
				return;
			}
			if (keyCode == cocos2d::EventKeyboard::KeyCode::KEY_X) {
				if (zoom <= 0.2f) return;
				zoom -= 0.1f;
				nodeCircles->setScale(zoom);
				return;
			}

			if (playing) return;

			if (!currCircle && currPic) {
				switch (keyCode) {
				case cocos2d::EventKeyboard::KeyCode::KEY_W:
					MoveUp();
					return;
				case cocos2d::EventKeyboard::KeyCode::KEY_S:
					MoveDown();
					return;
				case cocos2d::EventKeyboard::KeyCode::KEY_C:
					Clear();
					CopyPrevCircles();
					return;
				case cocos2d::EventKeyboard::KeyCode::KEY_F:
					Clear();
					return;
				}
			}

			if (!currCircle) return;

			switch (keyCode) {
			case cocos2d::EventKeyboard::KeyCode::KEY_A:
				if (currCircle && currExt == SAVE_FILE_EXT_CIRCLES) {
					currCircle->rChange = 30 / zoom;
				}
				break;
			case cocos2d::EventKeyboard::KeyCode::KEY_D:
				if (currCircle && currExt == SAVE_FILE_EXT_CIRCLES) {
					currCircle->rChange = -30 / zoom;
				}
				break;
			case cocos2d::EventKeyboard::KeyCode::KEY_E:
				currCircle->removeFromParent();
				currCircle = nullptr;
				Save();
				if (currExt != SAVE_FILE_EXT_CIRCLES) {
					Draw();
				}
				break;
			}
		};

		kbListener->onKeyReleased = [this](cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event) {
			if (!currCircle) return;
			if (currCircle) {
				currCircle->rChange = 0;
			}
		};

		cocos2d::Director::getInstance()->getEventDispatcher()->addEventListenerWithSceneGraphPriority(kbListener, this);
	}
	{
		auto touchListener = cocos2d::EventListenerTouchOneByOne::create();
		touchListener->setSwallowTouches(true);
		touchListener->onTouchBegan = [this](cocos2d::Touch* t, cocos2d::Event* e) {
			if (playing) return false;
			if (!currPic) return false;

			auto&& tL = t->getLocation();
			auto&& p = nodeCircles->convertToNodeSpace(tL);

			//if (p.x >= -DW_2 / zoom && p.x <= DW_2 / zoom && p.y >= -DH_2 / zoom && p.y <= DH_2 / zoom) {
				lastPos = p;
				assert(!currCircle);
				currCircle = CreateCircle(p, currExt == SAVE_FILE_EXT_CIRCLES ? 1.f : 7.f);
				if (currData.items.empty()) {
					currCircle->SetColor(cocos2d::Color4F::WHITE);
				}
				return true;
			//}
			//return false;
		};
		touchListener->onTouchMoved = [this](cocos2d::Touch* t, cocos2d::Event* e) {
			if (!currCircle || currExt != SAVE_FILE_EXT_CIRCLES) return false;
			auto&& tL = t->getLocation();
			auto&& p = nodeCircles->convertToNodeSpace(tL);
			auto&& d = p.getDistance(lastPos);
			currCircle->Draw(d);
			return true;
		};
		touchListener->onTouchEnded = [this](cocos2d::Touch* t, cocos2d::Event* e) {
			if (currCircle->r <= 3) {
				currCircle->removeFromParentAndCleanup(true);
			}
			else {
				Save();
			}
			currCircle = nullptr;
			return false;
		};
		touchListener->onTouchCancelled = touchListener->onTouchEnded;
		cocos2d::Director::getInstance()->getEventDispatcher()->addEventListenerWithSceneGraphPriority(touchListener, nodeCircles);
	}
	ok = LoadPics();
	return true;
}

void HelloWorld::onEnter() {
	this->BaseType::onEnter();
	//...
	cocos2d::extension::ImGuiEXT::getInstance()->addFont("c:/windows/fonts/simhei.ttf", 16, cocos2d::extension::ImGuiEXT::CHS_GLYPH_RANGE::FULL);
	cocos2d::extension::ImGuiEXT::getInstance()->addRenderLoop("#test", CC_CALLBACK_0(HelloWorld::onDrawImGui, this), this);
}

void HelloWorld::onExit() {
	cocos2d::extension::ImGuiEXT::getInstance()->removeRenderLoop("#test");
	cocos2d::extension::ImGuiEXT::getInstance()->clearFonts();

	cocos2d::extension::ImGuiEXT::destroyInstance();
	//...
	this->BaseType::onExit();
}

void HelloWorld::onDrawImGui() {
	ImGui::StyleColorsDark();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	auto sgStyleVar = xx::MakeScopeGuard([] { ImGui::PopStyleVar(1); });

	static const ImVec4 normalColor{ 0, 0, 0, 1.0f };
	static const ImVec4 pressColor{ 0.5f, 0, 0, 1.0f };
	static const ImVec4 releaseColor{ 0, 0.5f, 0, 1.0f };

	ImGui::PushStyleColor(ImGuiCol_ButtonActive, pressColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, releaseColor);
	auto sgStyleColor = xx::MakeScopeGuard([] { ImGui::PopStyleColor(2); });

	ImVec2 p = ImGui::GetMainViewport()->Pos;
	if (!ok) {
		p.x += 300;
		p.y += 450;
		ImGui::SetNextWindowPos(p);
		ImGui::SetNextWindowSize(ImVec2(AppDelegate::designWidth - 300 * 2, AppDelegate::designHeight - 450 * 2));
		ImGui::Begin("错误", nullptr, ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoInputs |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize);
		ImGui::Text(errorMessage.c_str());
		ImGui::End();
		return;
	}

	bool anyTreeNodeExtracted = false;
	p.x += 20;
	p.y += 20;
	ImGui::SetNextWindowPos(p);
	ImGui::SetNextWindowSize(ImVec2(400, AppDelegate::designHeight - 40));
	ImGui::Begin("精灵帧组", nullptr, ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize);
	for (auto& kv : this->picGroups) {
		if (ImGui::TreeNode(kv.first.c_str())) {
			anyTreeNodeExtracted = true;
			auto&& pics = kv.second;

			for (auto& p : pics) {
				ImGui::PushStyleColor(ImGuiCol_Button, p == currPic ? pressColor : normalColor);
				auto sg = xx::MakeScopeGuard([] { ImGui::PopStyleColor(1); });
				if (std::filesystem::exists(std::filesystem::path(AppDelegate::resPath) / (p->name + currExt))) {
					ImGui::Text("*");
				}
				else {
					ImGui::Text(" ");
				}
				ImGui::SameLine();
				if (ImGui::Button(p->name.c_str())) {
					if (p == currPic) continue;
					currPic = p;
					Draw();
				}
			}
			ImGui::TreePop();
		}
	}
	ImGui::End();
	if (!anyTreeNodeExtracted) {
		currPic = nullptr;
		Draw();
	}

	p.x += 420;
	ImGui::SetNextWindowPos(p);
	ImGui::SetNextWindowSize(ImVec2(1400, 40));
	ImGui::Begin("tips1", nullptr, ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoScrollbar);
	if (ImGui::Button(currExt.c_str())) {
		auto n = nodeCircles->getChildrenCount();
		if (currExt == SAVE_FILE_EXT_CIRCLES) {
			currExt = SAVE_FILE_EXT_LINES;
		}
		else {
			currExt = SAVE_FILE_EXT_CIRCLES;
		}
		currPic = nullptr;
		Draw();
	}
	ImGui::SameLine();
	ImGui::Text(currExt == SAVE_FILE_EXT_CIRCLES ? " ZX 缩放，W S 帧上下切换，C 复制上一帧, F 清空。 鼠标 左键 拖拽 画圆。圈选中后：A D 调整大小, E 删除"
		: " ZX 缩放，W S 帧上下切换，C 复制上一帧, F 清空。 鼠标 左键 点击画锁定点。多点连线。点选中后：E 删除");
	ImGui::SameLine();
	if (playing) {
		if (ImGui::Button("停止播放")) {
			playing = false;
			currPic = bak_currPic;
			Draw();
		}
	}
	else {
		if (ImGui::Button("30帧播放")) {
			Play(1.f / 30.f);
		}
		ImGui::SameLine();
		if (ImGui::Button("60帧播放")) {
			Play(1.f / 60.f);
		}
		ImGui::SameLine();
		if (ImGui::Button("15帧播放")) {
			Play(1.f / 15.f);
		}
	}
	ImGui::End();
}

void HelloWorld::Play(float const& delaySecs) {
	if (!currPic) return;
	bak_currPic = currPic;
	currPic = currPic->container->at(0);
	playingTimePool = 0;
	playingFrameDelay = delaySecs;
	playing = true;
}

void HelloWorld::update(float delta) {
	if (currPic && playing) {
		Draw();
		playingTimePool += delta;
		if (playingTimePool >= playingFrameDelay) {
			playingTimePool -= playingFrameDelay;

			currPic = currPic->next;
		}
	}
}

cocos2d::Sprite* HelloWorld::CreateSpriteFrame(cocos2d::Vec2 const& pos, cocos2d::Size const& siz, std::string const& spriteFrameName, cocos2d::Node* const& container) {
	auto s = cocos2d::Sprite::create();
	s->setSpriteFrame(spriteFrameName);
	s->setAnchorPoint({ 0.5, 0.5 });
	s->setPosition(pos);
	if (siz.width > 0 && siz.height > 0) {
		auto&& cs = s->getContentSize();
		if (cs.width > cs.height) {
			s->setScale(siz.width / cs.width);
		}
		else {
			s->setScale(siz.height / cs.height);
		}
	}
	if (container) {
		container->addChild(s);
	}
	else {
		this->addChild(s);
	}
	return s;
}

void HelloWorld::MoveUp() {
	if (!currPic->IsFirst()) {
		currPic = currPic->prev;
		Draw();
	}
}

void HelloWorld::MoveDown() {
	if (!currPic->IsLast()) {
		currPic = currPic->next;
		Draw();
	}
}

void HelloWorld::CopyPrevCircles() {
	if (!currPic || allPics.empty() || currPic->IsFirst()) return;

	// 如果 没有上一张，继续往上找，直到出现。缺失文件通通拷贝填充
	auto p = currPic;
	do {
		p = p->prev;
		auto path = std::filesystem::path(AppDelegate::resPath) / (p->name + currExt);
		if (std::filesystem::exists(path)) {
			do {
				p = p->next;
				std::filesystem::copy_file(path, std::filesystem::path(AppDelegate::resPath) / (p->name + currExt));
			} while (p != currPic);
			break;
		}
	} while (!p->IsFirst());

	Draw();
	return;
}

Circle* HelloWorld::CreateCircle(cocos2d::Vec2 const& pos, float const& r) {
	auto&& nc = Circle::create(currExt == ".lines");
	nodeCircles->addChild(nc);
	nc->scene = this;
	nc->setPosition(pos);
	nc->Draw(r);
	return nc;
};

void HelloWorld::Draw() {
	// 清场
	nodeCircles->removeAllChildrenWithCleanup(true);
	lastPos = {};
	touching = false;
	currCircle = nullptr;
	// todo: more state cleanup here?

	if (!currPic) return;

	// 画屏幕裁剪范围背景框
	{
		auto dn = cocos2d::DrawNode::create();
		dn->drawRect({ -DW / 2, -DH / 2 }, { DW / 2, DH / 2 }, cocos2d::Color4F::MAGENTA);
		nodeCircles->addChild(dn);
	}

	// todo: switch case typeId for support spine, c3b
	switch (currPic->type) {
	case PicTypes::SpriteFrame: {
		// 画本体
		/*auto spr = */CreateSpriteFrame({}, {}, currPic->name, nodeCircles);
		break;
	}
	case PicTypes::Spine: {
		spine::SkeletonAnimation* sa;
		if (currPic->fileName2.ends_with(".json")) {
			sa = spine::SkeletonAnimation::createWithJsonFile(currPic->fileName2, currPic->fileName, 1);
		}
		else {
			sa = spine::SkeletonAnimation::createWithBinaryFile(currPic->fileName2, currPic->fileName, 1);
		}
		sa->setAnimation(0, currPic->actionName, false);
		sa->getCurrent()->trackTime = currPic->timePoint;
		sa->getCurrent()->timeScale = 0;
		if (auto skc = sa->getSkeleton()->data->skinsCount) {
			sa->setSkin(sa->getSkeleton()->data->skins[skc-1]->name);
		}

		nodeCircles->addChild(sa);
		break;
	}
	case PicTypes::C3b: {
		// todo
		break;
	}
	}

	// 加载圆数据
	if (Load()) return;

	// 画圆
	for (size_t i = 0; i < currData.items.size(); ++i) {
		auto& c = currData.items[i];
		auto n = CreateCircle({ c.x, c.y }, c.r);
		if (i == 0) {
			n->SetColor(cocos2d::Color4F::WHITE);
		}
	}
}

bool HelloWorld::LoadPics() {
	auto&& fu = cocos2d::FileUtils::getInstance();

	// 逐个加载所有 plist
	for (auto&& entry : std::filesystem::directory_iterator(AppDelegate::resPath)) {
		if (entry.is_regular_file() && entry.path().has_extension()
			&& entry.path().extension().string() == ".plist") {
			auto plistfn = entry.path().filename().string();
			auto fullPath = entry.path().string();
			cocos2d::SpriteFrameCache::getInstance()->addSpriteFramesWithFile(fullPath);

			// 找出所有 sprite frame name. 写法确保与 addSpriteFramesWithFile 函数内部一致
			std::vector<std::string> names;
			auto&& dict = fu->getValueMapFromFile(fullPath);
			auto&& framesDict = dict["frames"].asValueMap();
			for (auto&& iter : framesDict) {
				auto&& frameDict = iter.second.asValueMap();
				names.push_back(iter.first);
			}

			// 排序( 如果含有数字，以其中的数字部分大小来排 )
			std::sort(names.begin(), names.end(), [](std::string const& a, std::string const& b)->bool {
				return xx::InnerNumberToFixed(a) < xx::InnerNumberToFixed(b);
				});

			// 存储
			for (auto& n : names) {
				// 重复检测
				for (auto& p : allPics) {
					if (p.name == n) {
						errorMessage += "重复的帧图名 \"" + n + "\" 位于 \"" + plistfn + "\"";
						return false;
					}
				}

				// 创建存储节点并填充 name, type
				auto& p = allPics.emplace_back();
				p.name = std::move(n);
				p.type = PicTypes::SpriteFrame;
			}
		}
	}

	// 逐个加载所有 atlas
	for (auto&& entry : std::filesystem::directory_iterator(AppDelegate::resPath)) {
		if (entry.is_regular_file() && entry.path().has_extension()
			&& entry.path().extension().string() == ".atlas") {
			auto fn = entry.path().filename().string();
			auto partialFn = fn.substr(0, fn.size() - 6);
			std::string dataFn;
			auto atlasFullPath = entry.path().string();
			auto tmp = atlasFullPath.substr(0, atlasFullPath.size() - 6);
			auto skelFullPath = tmp + ".skel";
			auto jsonFullPath = tmp + ".json";

			spine::SkeletonAnimation* s;
			if (std::filesystem::exists(skelFullPath)) {
				s = spine::SkeletonAnimation::createWithBinaryFile(skelFullPath, atlasFullPath, 1);
				dataFn = partialFn + ".skel";
			}
			else if (std::filesystem::exists(jsonFullPath)) {
				s = spine::SkeletonAnimation::createWithJsonFile(jsonFullPath, atlasFullPath, 1);
				dataFn = partialFn + ".json";
			}
			else {
				errorMessage += "找不到 spine 文件: \"" + fn + "\" 的配套 .skel 或 .json";
				return false;
			}

			auto&& as = s->getSkeleton()->data->animations;
			auto asSiz = s->getSkeleton()->data->animationsCount;
			for (int i = 0; i < asSiz; ++i) {
				auto&& a = as[i];
				auto d = a->duration;
				std::string an(a->name);

				int j = 0;
				for (float t = 0.f; t < d; t += 1.f / 30.f) {
					auto n = partialFn + "_" + an + "_" + std::to_string(j);
					// 重复检测
					for (auto& p : allPics) {
						if (p.name == n) {
							errorMessage += "重复的帧图名 \"" + n + "\" 位于 \"" + fn + "\"";
							return false;
						}
					}

					// 创建存储节点并填充 name, type
					auto& p = allPics.emplace_back();
					p.name = std::move(n);
					p.type = PicTypes::Spine;
					p.fileName = fn;
					p.fileName2 = dataFn;
					p.actionName = an;
					p.timePoint = t;
					j++;
				}
			}
		}
	}

	// todo: 逐个加载所有 c3b

	// 填充到分组容器. 填充 idx, container
	for (auto& p : allPics) {
		auto& pics = picGroups[RemoveNumbers(p.name)];
		p.idx = (int)pics.size();
		pics.push_back(&p);
		p.container = &pics;
	}

	// 填充 prev, next
	for (auto& g : picGroups) {
		for (auto& p : g.second) {
			p->prev = g.second[p->IsFirst() ? g.second.size() - 1 : p->idx - 1];
			p->next = g.second[p->IsLast() ? 0 : p->idx + 1];
		}
	}

	return true;
}

int HelloWorld::Load() {
	auto fu = cocos2d::FileUtils::getInstance();
	auto fn = currPic->name + currExt;
	currData.items.clear();
	return LoadJson(currData, fn);
}

void HelloWorld::Save() {
	assert(currPic);

	// 清空
	currData.items.clear();

	// 从 nodeCircles 过滤出所有 Circle 节点 保存数据到 currData
	for (auto&& n : nodeCircles->getChildren()) {
		if (auto&& nc = dynamic_cast<Circle*>(n)) {
			auto& c = currData.items.emplace_back();
			c.x = nc->getPositionX();
			c.y = nc->getPositionY();
			c.r = nc->r;
		}
	}

	// 存盘
	auto fn = currPic->name + currExt;
	auto path = std::filesystem::path(AppDelegate::resPath) / fn;
	if (currData.items.empty()) {
		std::filesystem::remove(path);
	}
	else {
		ajson::save_to_file(currData, path.string().c_str());
	}
}

void HelloWorld::Clear() {
	nodeCircles->removeAllChildrenWithCleanup(true);
	Save();
	Draw();
}
