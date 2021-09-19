#include "HelloWorldScene.h"

#include "xx_math.h"
#include "xx_ptr.h"
#include "xx_string.h"
#include "xx_randoms.h"


/*
(0,0)                      grid
������������������������������������������������������������������������������������������������������������������
��                          map                          ��
��   ��������������������������������������������������������������������������������������������������   ��
��   ��                                               ��   ��
��   ��          x                                    ��   ��
��   ��    x           screen           x             ��   ��
��   ��          ��������������������������������������            x     ��   ��
��   ��          ��                 ��                  ��   ��
��   ��          ��    x            ��  x               ��   ��
��   ��          ��        O        ��                  ��   ��
��   ��          ��              x  ��                  ��   ��
��   ��         x��                 ��          x       ��   ��
��   ��          ��������������������������������������                  ��   ��
��   ��    x                             x            ��   ��
��   ��               x                               ��   ��
��   ��                      x                        ��   ��
��   ��                                               ��   ��
��   ��           x                 x           x     ��   ��
��   ��                  x                            ��   ��
��   ��                                              x��   ��
��   ��������������������������������������������������������������������������������������������������   ��
��                                                       ��
������������������������������������������������������������������������������������������������������������������

���ͣ�x ����    O ���    screen ��Ļ��ʾ����    map ��Ϸ��ͼ    grid ����ϵͳ���긲�Ƿ�Χ
(0,0) ��ʾ����ԭ��. ���� ����ϵͳ / ��Ϸ��ͼ ��˵��ԭ�������Ͻǣ������� opengl ɶ��������ԭ�������½�, ��ʾ�� y ȡ��

��ͼ, ���� ��ɢ�ֲ��� ��Ϸ��ͼ ��, ��� ͨ��λ�� ��Ļ ����λ��, ������ ��Ļ Ҳֻ����ʾ ��ͼ�ı�ɽһ��
���Ϊȷ�� ����ϵͳ ������������ͼ ���������� ʵ���� Զ���� �߼���������( Ҫ�������������ݷ�Χ��Ҫ�ڸ����� )
	���������Ϊ�˼��ٱ�Ե���� if �Ƿ��и��ӵ��ж�, ������ִ��Ч��

�������:
	��Ļ 1280 * 720      ( �����ߴ� )
	���� 100 * 100       ( ��������Ķ���ĵ��ͳߴ�, ������ȥ���������ݵ����ֵ )
	��ͼ 4000 * 2000
	���� ?????

�������������� ���ӳߴ��Լ��ָ������
	���� ����ϵͳ ��˵������ ��ÿ���������װ��һ����( ����ĳ���� engine �ľ������� )
	������ 2 ���ô���
		1. �ƶ�����ʱ����������ӵ���Ϊ������̫Ƶ��
		2. ��������ʱ���󲿷�ʱ�򣬹�ע 9�� ��Χ�ھ����������ˣ�������ɸ��̫�಻��ɵ�

	���� ����ռ�ز��ص�( �����������ھ��ȷֲ���������ӣ�Ҳ���������ೣʶ )
	���������ܼ���������Ļ. ���Լ��㣬һ��ͼ������ 40 * 20 = 800 ��ֻ��

	�ɴ˿ɵõ� grid ��Ԫ��ֱ�� �� cap ������100, 800
	��һ���ģ�������Ҫȷ����Ե��ѯ 9�� ���ᳬ�� grid �����±꣬4����Ե�����һ��
	��ͼ 4�� ���գ��õ� 4200 * 2200����Ϸ����� 100, 100 ��ʼ�ֲ���ռ�� x = 100 ~ 4100, y = 100 ~ 2100 �����ݷ�Χ
	��ͼ��չ���ʵ�ʳߴ磬���� 100, �õ�������

���:
	��Ļ ����
	���� 100ֱ��
	��ͼ 4200�� * 2200��
	���� 22��, 42��, 800Ԥ��, 100����ֱ��( ��������� grid ϵͳ�����֣�Ҫ��һ����װ���� )
	���ǵ������������ܣ��ߴ��� 2 ���� ��£����ͼ�������������ߴ磬�����Ż�

���ս��:
	��Ļ ͨ�� scale ������������ʾ����
	���� 128ֱ��, ������40ֻ��������20ֻ����800ֻ
	��ͼ 128 * 2 + 128 * 40 ֻ, 128 * 2 + 128 * 20 ֻ = 5376 * 2816
	���� 22��, 42��, 800Ԥ��, 128����ֱ��

�����Ͻ������ӵ� ������Ҳ������ 2 ���� ��£������λ���� ����˳��� �����������ܡ����迼�Ƕ���ռ��������������Ч�Է�������

���ڹ������ҵ� �ӵ���Χǽʯͷ�ȸ���ɶ�ģ����������Ʋ������ٽ����� grid ���ݡ�
*/






/*****************************************************************************************************/
/*****************************************************************************************************/

namespace xx {

	// ���������( Ԥ���� )
	struct ItemMgr;

	// �����ڶ������
	struct Item {
		// ָ�����ڳ���( ��������ȷ����ָ���Ȼ��Ч )
		ItemMgr* im;
		// ���� grid �±�. ���� im->grid �󣬸�ֵ��Ϊ -1
		int gridIndex = -1;
		// ��ǰ����
		XY xy;

		// ������ im, �� copy
		Item() = delete;
		Item(Item const&) = delete;
		Item& operator=(Item const&) = delete;

		// Ĭ�Ϲ���( ������ͨ�� using Item::Item ֱ�Ӽ̳� )
		// �����������г�ʼ�� xy �� gridIndex = im->grid.Add(xy.x / nnnn, xy.y / nnnn, this);
		// ��;ͬ���� im->grid.Update(gridIndex, xy.x / nnnn, xy.y / nnnn);
		Item(ItemMgr* im) : im(im) {}

		// ����з��� im->grid�����Զ��Ƴ�
		virtual ~Item();

		// �����߼�����
		virtual size_t Update() = 0;
	};

	// ���������
	struct ItemMgr {

		// ����ϵͳ
		Grid2d<Item*> grid;

		// �������� ��Ҫ�� ����ϵͳ ���·���ȷ�� �� ����������������ȷ�Ĵ� ����ϵͳ �Ƴ�
		std::vector<Shared<Item>> objs;

		ItemMgr() = default;
		ItemMgr(ItemMgr const&) = delete;
		ItemMgr& operator=(ItemMgr const&) = delete;

		virtual size_t Update() {
			for (int i = (int)objs.size() - 1; i >= 0; --i) {
				auto& o = objs[i];
				if (o->Update()) {
					if (auto n = (int)objs.size() - 1; i < n) {
						o = std::move(objs[n]);
					}
					objs.pop_back();
				}
			}
			return objs.size();
		}
	};

	inline Item::~Item() {
		if (gridIndex != -1) {
			assert(im->grid[gridIndex].value == this);
			im->grid.Remove(gridIndex);
			gridIndex = -1;
		}
	}

}


/*****************************************************************************************************/
/*****************************************************************************************************/

namespace GameLogic {

	// ���д���ģ�� һ�� Բ�ζ���, λ�� 2d �ռ䣬һ��ʼ����һ��֮����������ſ�, ֱ����ֹ��ͳ��һ��Ҫ���೤ʱ�䡣
	// ���ݽṹ���治�������Ż��������� oo ���
	// Ҫ�Աȵ��� �� �� ���� grid ϵͳ��Ч��


	// ģ������. ���ӳߴ�, grid ������Ϊ 2^n ����λ����
	//static const int gridWidth = 32;
	//static const int gridHeight = 32;
	//static const int gridDiameter = 256;
	static const int gridWidth = 128;
	static const int gridHeight = 64;
	static const int gridDiameter = 128;
	//static const int gridWidth = 128;
	//static const int gridHeight = 128;
	//static const int gridDiameter = 64;

	static const int mapMinX = gridDiameter;							// >=
	static const int mapMaxX = gridDiameter * (gridWidth - 1) - 1;		// <=
	static const int mapMinY = gridDiameter;
	static const int mapMaxY = gridDiameter * (gridHeight - 1) - 1;
	static const xx::XY mapCenter = { (mapMinX + mapMaxX) / 2, (mapMinY + mapMaxY) / 2 };

	// Foo �İ뾶 / ֱ�� / �ƶ��ٶ�( ÿ֡�ƶ���λ���� ) / �ھӲ��Ҹ���
	static const int fooRadius = 50;
	static const int fooDiameter = fooRadius * 2;
	static const int fooSpeed = 5;
	static const int fooFindNeighborLimit = 20;	// 9��Χ��ͨ�������ɵĸ���



	struct Scene;
	struct Foo : xx::Item {
		using xx::Item::Item;
		int radius = fooRadius;
		int speed = fooSpeed;
#ifdef CC_STATIC
		cocos2d::Sprite* body = nullptr;
#endif
		Foo(Scene* scene, xx::XY const& xy);
		~Foo();
		Scene& scene();
		size_t Update() override;
	};

	struct Scene : xx::ItemMgr {
		using xx::ItemMgr::ItemMgr;

#ifdef CC_STATIC
		// ָ�� cocos ��ʾ��
		HelloWorld* hw = nullptr;
#endif

		// �����һö, ���ڶ�����ȫ�ص�������²���һ���ƶ�����
		xx::Random1 rnd;

		// ÿ֡ͳ�ƻ��ж��ٸ����������ƶ�
		int count = 0;
#ifdef CC_STATIC
		Scene(int const& fooCount, HelloWorld* const& hw) : hw(hw) {
#else
		Scene(int const& fooCount) {
#endif
			// ��ʼ������
			grid.Init(gridHeight, gridWidth, fooCount);
			objs.reserve(fooCount);

			// ֱ�ӹ���� n �� Foo
			for (int i = 0; i < fooCount; i++) {
				objs.emplace_back(xx::Make<Foo>(this, mapCenter));
			}
		}

		size_t Update() override {
			count = 0;
			return xx::ItemMgr::Update();
		}
	};

	inline Scene& Foo::scene() {
		return *(Scene*)im;
	}

	inline Foo::Foo(Scene* scene, xx::XY const& xy) : xx::Item(scene) {
		this->xy = xy;
		gridIndex = scene->grid.Add(xy.y / gridDiameter, xy.x / gridDiameter, this);
	}

	inline size_t Foo::Update() {
		// ��������
		auto xy = this->xy;

		// �ж���Χ�Ƿ��б�� Foo ���Լ��ص���ȡǰ n �������ݶԷ����Լ��ĽǶȣ���Ͼ��룬�õ� ����ʸ������� �õ��Լ���ǰ��������ٶȡ�
		// ��ؼ����������ƶ������ٶ���Ҫ�޶����ֵ����Сֵ����������ʸ��Ϊ 0���Ǿ�����Ƕ������ٶ��ƶ���
		// �ƶ���� xy ��������� ��ͼ��Ե����Ӳ����. 

		// ���� n ���ھ�
		int limit = fooFindNeighborLimit;
		int crossNum = 0;
		xx::XY v;
		im->grid.LimitFindNeighbor(limit, gridIndex, [&](auto o) {
			assert(o != (Item*)this);
			// ����ת���·���ʹ��
			auto& f = *(Foo*)o;
			// ���Բ����ȫ�ص����򲻲�������
			if (f.xy == this->xy) {
				++crossNum;
				return;
			}
			// ׼���ж��Ƿ����ص�. r1* r1 + r2 * r2 > (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y)
			auto r1 = f.radius;
			auto r2 = this->radius;
			auto r12 = r1 * r1 + r2 * r2;
			auto p1 = f.xy;
			auto p2 = this->xy;
			auto p12 = (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);
			// ���ص�
			if (r12 > p12) {
				// ���� f �� this �ĽǶ�( �Է��������������� ). 
				auto a = xx::GetAngle<(gridDiameter * 2 >= 1024)>(p1, p2);
				// �ص���Խ�࣬����Խ��?
				auto inc = xx::Rotate(xx::XY{ this->speed * r12 / p12, 0 }, a);
				v += inc;
				++crossNum;
			}
			});

		if (crossNum) {
			// ��� v == 0 �Ǿ�����Ƕ������ٶ��ƶ�
			if (v.IsZero()) {
				auto a = scene().rnd.Next() % xx::table_num_angles;
				auto inc = xx::Rotate(xx::XY{ speed, 0 }, a);
				xy += inc;
			}
			// ���� v �ƶ�
			else {
				auto a = xx::GetAngleXY(v.x, v.y);
				auto inc = xx::Rotate(xx::XY{ speed * std::max(crossNum, 5), 0 }, a);
				xy += inc;
			}

			// ��Ե�޶�( ��ǰ�߼����ص����ƶ����ʱ߽���������� )
			if (xy.x < mapMinX) xy.x = mapMinX;
			else if (xy.x > mapMaxX) xy.x = mapMaxX;
			if (xy.y < mapMinY) xy.y = mapMinY;
			else if (xy.y > mapMaxY) xy.y = mapMaxY;
		}

		// ��������ͬ��
		if (xy != this->xy) {
			++scene().count;
			this->xy = xy;
			assert(gridIndex != -1);
			im->grid.Update(gridIndex, xy.y / gridDiameter, xy.x / gridDiameter);
		}

#ifdef CC_STATIC
		// ����
		if (!body) {
			body = cocos2d::Sprite::create("b.png");
			//body->setScale(1.25);
			scene().hw->container->addChild(body);
		}
		body->setPosition(xy.x, xy.y);
#endif

		return 0;
	}

	inline Foo::~Foo() {
		if (body) {
			body->removeFromParent();
			body = nullptr;
		}
	}
}

bool HelloWorld::init() {
	if (!Scene::init()) return false;
	scheduleUpdate();

	auto siz = cocos2d::Director::getInstance()->getVisibleSize();

	container = cocos2d::Node::create();
	//container->setPosition(siz / 2);
	container->setScale(0.245);
	addChild(container);

	scene = new GameLogic::Scene(20000, this);
	return true;
}

void HelloWorld::update(float delta) {
	scene->Update();
}

HelloWorld::~HelloWorld() {
	if (scene) {
		delete(scene);
		scene = nullptr;
	}
}
