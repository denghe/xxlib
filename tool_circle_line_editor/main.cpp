#include <string>
#include <vector>

namespace ff {
	// 锁定点
	struct LockPoint {
		float x = 0.0f;
		float y = 0.0f;
	};
	// 碰撞圆
	struct CDCircle {
		float x = 0.0f;
		float y = 0.0f;
		float r = 0.0f;
	};
	// 时间点--碰撞圆集合
	struct TimePoint_CDCircles {
		// 起始时间( 秒 )
		float time = 0.0f;
		// 最大碰撞圆范围（外包围圆），用于碰撞检测粗判
		::ff::CDCircle maxCDCircle;
		// 具体碰撞圆列表，用于碰撞检测遍历细判
		::std::vector<::ff::CDCircle> cdCircles;
	};
	// 时间点--锁定点线
	struct TimePoint_LockPoints {
		// 起始时间( 秒 )
		float time = 0.0f;
		// 主锁定点。如果出屏幕，则锁定 锁定线与屏幕边缘形成的 交点
		::ff::LockPoint mainLockPoint;
		// 锁定线
		::std::vector<::ff::LockPoint> lockPoints;
	};
	// 针对  等动画文件, 附加 碰撞 & 锁定 等数据
	struct Anim {
		// 动画名
		::std::string name;
		// 皮肤(有些spine动画需要)
		::std::string skin;
		// 总时长( 秒 )
		float totalSeconds = 0.0f;
		// 播放完后自动切换的动画名
		::std::string nextAnim;
		// 时间点--锁定点线 集合
		::std::vector<::ff::TimePoint_LockPoints> lps;
		// 时间点--碰撞圆 集合
		::std::vector<::ff::TimePoint_CDCircles> cds;
	};
	// 动画配置 存盘文件 *.anims
	struct File_Anims {
		// 指向原始资源文件名( atlas/spine, c3b, frames )
		::std::string resFileName;
		// 动画集合
		::std::vector<::ff::Anim> anims;
		float shadowX = 0.0f;
		float shadowY = 0.0f;
		float shadowScale = 0.0f;
	};
	// 时间点--帧
	struct TimePoint_Frame {
		// 起始时间( 秒 )
		float time = 0.0f;
		// 帧名
		::std::string picName;
	};
	// 帧动画
	struct FrameAnim {
		// 动画名
		::std::string name;
		// 总时长( 秒 )
		float totalSeconds = 0.0f;
		// 时间点--帧 集合
		::std::vector<::ff::TimePoint_Frame> frames;
	};
	// 帧动画 存盘文件 .frames
	struct File_Frames {
		// 帧动画集合
		::std::vector<::ff::FrameAnim> frameAnims;
		// 图位于哪些 plist
		::std::vector<::std::string> plists;
	};
}

#include "ajson.hpp"
AJSON(::ff::LockPoint, x, y);
AJSON(::ff::CDCircle, x, y, r);
AJSON(::ff::TimePoint_CDCircles, time, maxCDCircle, cdCircles);
AJSON(::ff::TimePoint_LockPoints, time, mainLockPoint, lockPoints);
AJSON(::ff::Anim, name, skin, totalSeconds, nextAnim, lps, cds);
AJSON(::ff::File_Anims, resFileName, anims, shadowX, shadowY, shadowScale);
AJSON(::ff::TimePoint_Frame, time, picName);
AJSON(::ff::FrameAnim, name, totalSeconds, frames);
AJSON(::ff::File_Frames, frameAnims, plists);

struct Item {
	float x, y, r;
};
AJSON(Item, x, y, r);
struct Data {
	std::vector<Item> items;
};
AJSON(Data, items);


#include <filesystem>
namespace fs = std::filesystem;
#include <iostream>
#include <cassert>

int FillCDCircles(fs::path const& path, ::std::vector<::ff::TimePoint_CDCircles>& cds, float const& t) {
	if (fs::exists(path)) {
		// 加载数据
		Data d;
		ajson::load_from_file(d, path.string().c_str());

		// 简单检查
		if (d.items.empty()) {
			std::cout << "bad data at file: " << path << std::endl;
			return __LINE__;
		}

		// 开始填充
		// 如果只有 1 个圆，则重复使用它作为 包围盒 & 圆列表
		// 如果只有 2 个圆，则忽略第一个圆, 重复使用第二个圆作为 包围盒 & 圆列表
		auto& cd = cds.emplace_back();
		cd.time = t;
		if (d.items.size() == 1) {
			cd.maxCDCircle = { d.items[0].x, d.items[0].y, d.items[0].r };
			cd.cdCircles.push_back({ d.items[0].x, d.items[0].y, d.items[0].r });
		}
		else if (d.items.size() == 2) {
			cd.maxCDCircle = { d.items[1].x, d.items[1].y, d.items[1].r };
			cd.cdCircles.push_back({ d.items[1].x, d.items[1].y, d.items[1].r });
		}
		else {
			cd.maxCDCircle = { d.items[0].x, d.items[0].y, d.items[0].r };
			for (size_t i = 1; i < d.items.size(); ++i) {
				cd.cdCircles.push_back({ d.items[i].x, d.items[i].y, d.items[i].r });
			}
		}
	}
	else {
		//std::cout << "miss data: " << ncircles << std::endl;
	}
	return 0;
}
int FillLockPoints(fs::path const& path, ::std::vector<::ff::TimePoint_LockPoints>& lps, float const& t) {
	if (fs::exists(path)) {
		// 加载数据
		Data d;
		ajson::load_from_file(d, path.string().c_str());

		// 简单检查. 没数据或者只有 2 个点为异常. 因为第一个点为 主锁定点，剩下一个点无法构成一条线
		if (d.items.empty() || d.items.size() == 2) {
			std::cout << "bad data at file: " << path << std::endl;
			return __LINE__;
		}

		// 开始填充
		auto& lp = lps.emplace_back();
		lp.time = t;
		lp.mainLockPoint = { d.items[0].x ,d.items[0].y };
		for (size_t i = 1; i < d.items.size(); ++i) {
			lp.lockPoints.push_back({ d.items[i].x ,d.items[i].y });
		}
	}
	else {
		//std::cout << "miss data: " << nlines << std::endl;
	}
	return 0;
}

int main(int argc, char const* const* argv) {
	fs::path tarPath, resPath;
	if (argc > 1) {
		if (argc < 3 || !fs::is_directory(argv[1]) || !fs::is_directory(argv[2])) {
			std::cout << "need args. args[1]: .anims dir  args[2]: .circles & .lines dir" << std::endl;
			return __LINE__;
		}
		else {
			tarPath = fs::path(argv[1]);
			resPath = fs::path(argv[2]);
		}
	}
	else {
#ifndef NDEBUG
		tarPath = resPath = fs::path(R"(C:\Codes\fisheditor\build\bin\fisheditor\Release\Resources)");
#else
		std::cout << "need args. args[1]: .anims dir  args[2]: .circles & .lines dir" << std::endl;
		return __LINE__;
#endif
	}

	// 扫描所有 .anims
	for (auto&& entry : fs::directory_iterator(tarPath)) {
		if (entry.is_regular_file() && entry.path().has_extension()
			&& entry.path().extension().string() == ".anims") {

			::ff::File_Anims anims;
			ajson::load_from_file(anims, entry.path().string().c_str());

			fs::path res(anims.resFileName);
			assert(res.has_filename() && res.has_extension());

			auto fn = res.filename().string();
			auto partialFn = fn.substr(0, fn.size() - 6);
			auto ext = res.extension().string();

			if (ext == ".frames") {
				if (!fs::exists(resPath / res)) {
					std::cout << "can't find res file: " << fn << " for " << entry << std::endl;
					continue;
				}

				// 读入 .frames 文件备用
				::ff::File_Frames frames;
				ajson::load_from_file(frames, (resPath / res).string().c_str());

				// 遍历所有动作
				for (auto& a : anims.anims) {
					// 清理老数据
					a.lps.clear();
					a.cds.clear();

					// 从 frames 定位到 a.name 并遍历
					ff::FrameAnim* fa = nullptr;
					for (auto& o : frames.frameAnims) {
						if (o.name == a.name) {
							fa = &o;
							break;
						}
					}
					if (!fa) {
						std::cout << "can't find action name: " << a.name << " at file: " << fn << std::endl;
						continue;
					}
					for (auto& f : fa->frames) {
						// 拼接文件名
						auto nlines = f.picName + ".lines";
						auto ncircles = f.picName + ".circles";

						FillCDCircles(resPath / ncircles, a.cds, f.time);
						FillLockPoints(resPath / nlines, a.lps, f.time);
					}

					if (a.cds.empty()) {
						std::cout << "fill    " << fn << "    " << a.name << "    cd circles failed: no any data found." << std::endl;
					}
					if (a.lps.empty()) {
						std::cout << "fill    " << fn << "    " << a.name << "    lock points failed: no any data found." << std::endl;
					}
				}
			}
			else if (ext == ".atlas") {
				// 遍历所有动作
				for (auto& a : anims.anims) {
					// 清理老数据
					a.lps.clear();
					a.cds.clear();

					// 按播放时长 30 fps切割
					int j = 0;
					for (float t = 0.f; t < a.totalSeconds; t += 1.f / 30.f) {
						// 拼接文件名
						auto n = partialFn + "_" + a.name + "_" + std::to_string(j);
						auto nlines = n + ".lines";
						auto ncircles = n + ".circles";

						FillCDCircles(resPath / ncircles, a.cds, t);
						FillLockPoints(resPath / nlines, a.lps, t);

						j++;
					}

					if (a.cds.empty()) {
						std::cout << "fill    " << fn << "    " << a.name << "    cd circles failed: no any data found." << std::endl;
					}
					if (a.lps.empty()) {
						std::cout << "fill    " << fn << "    " << a.name << "    lock points failed: no any data found." << std::endl;
					}
				}
			}
			else if (ext == ".c3b") {
				// todo
			}

			ajson::save_to_file(anims, entry.path().string().c_str());
		}
	}

	return 0;
}
