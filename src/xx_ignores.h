#pragma once
#include "xx_string.h"
#include "xx_file.h"
#include <ranges>
namespace xx {

	// 仿 .gitignore 功能的 简单实现。
	// 只支持下列情况：
	// 1. 目录名 或 文件名. 前面 可加 \ 以限定为根目录
	// 2. * 通配 文件名, 只支持 1 个. * 右侧的内容 被认为 一定出现在最右边
	// 3. 以 ! 开头 的 1, 2 情况为 最优先级 白名单

	// 匹配器基类
	struct IgnoreMacther {
		virtual ~IgnoreMacther() {}
		virtual bool Math(std::string_view const& tar) = 0;
	};

	// 匹配 \目录名
	struct IgnoreMatcher_RootDir : IgnoreMacther {
		std::string_view s;
		explicit IgnoreMatcher_RootDir(std::string_view const& s) : s(s) {}
		virtual bool Math(std::string_view const& t) override {
			if (t.size() < s.size()) return false;													// 长度不足: false
			if (t.starts_with(s)) {																	// 以 s 打头
				if (t.size() == s.size()) return true;												// 右边没了, 完全相同: true
				return t[s.size()] == '\\';															// 返回 右边目录是否完整
			}
			return false;
		}
	};

	// 匹配 目录名
	struct IgnoreMatcher_Dir : IgnoreMacther {
		std::string_view s;
		explicit IgnoreMatcher_Dir(std::string_view const& s) : s(s) {}
		virtual bool Math(std::string_view const& t) override {
			if (t.size() < s.size()) return false;													// 长度不足: false
			if (auto i = t.find(s); i != t.npos) {													// 找到了
				if (t.size() == s.size()) return true;												// 完全相同: true
				if (i == 0) return t[s.size()] == '\\';												// 同 RootDir 逻辑
				else {
					if (t[i - 1] != '\\') return false;												// 左边目录不完整: false
					if (i + s.size() == t.size()) return true;										// 右边没了, 完全相同: true
					return t[i + s.size()] == '\\';													// 返回 右边目录是否完整
				}
			}
			return false;
		}
	};

	// 匹配 \[目录名\目录名...\]*扩展名
	struct IgnoreMatcher_RootFiles : IgnoreMacther {
		std::string_view path, ext;
		size_t minSize;
		explicit IgnoreMatcher_RootFiles(std::string_view const& s) {
			path = s.substr(0, s.find('*'));
			ext = s.substr(path.size() + 1);
			minSize = path.size() + ext.size();
		}
		virtual bool Math(std::string_view const& t) override {
			if (t.size() < minSize) return false;													// 长度不足: false
			if (t.starts_with(path) && t.ends_with(ext)) {											// 以 path 打头, ext 结束
				auto fn = t.substr(path.size(), t.size() - minSize);
				return fn.find('\\') == fn.npos;													// 返回 fn 中是否不含 /
			}
			return false;
		}
	};

	// 匹配 [目录名\目录名...\]*扩展名
	struct IgnoreMatcher_Files : IgnoreMacther {
		std::string_view path, ext;
		size_t minSize;
		explicit IgnoreMatcher_Files(std::string_view const& s) {
			path = s.substr(0, s.find('*'));
			ext = s.substr(path.size() + 1);
			minSize = path.size() + ext.size();
		}
		virtual bool Math(std::string_view const& t) override {
			if (t.size() < minSize) return false;													// 长度不足: false
			if (!t.ends_with(ext)) return false;													// 并非 ext 结束: false
			auto left = t.substr(0, t.size() - ext.size());											// 切掉 ext
			if (auto i = left.find_last_of('\\'); i == left.npos) return !path.size(); 				// 左边整个都是文件名. path 为空: true
			else {
				left = left.substr(0, i);															// 切掉 /fn
				return left.ends_with(path);														// 以 path 结尾: true
			}
			if (auto i = t.find(path); i == t.npos) return false;									// 未找到 path: false
			else if (i == 0 || t[i - 1] != '\\') {													// 左边目录完整
				auto fn = t.substr(i + path.size(), t.size() - i - minSize);
				return fn.find('\\') == fn.npos;													// 返回 fn 中是否不含 /
			}
			return false;
		}
	};

	// 匹配管理器
	struct IgnoresManager {
		xx::Data d;																					// 所有 string hold 在此
		std::vector<std::unique_ptr<IgnoreMacther>> matchers;										// 黑名单适配器集合
		std::vector<std::unique_ptr<IgnoreMacther>> unmatchers;										// 白名单...

		// 从文件加载 ignore 文本. 失败返回 行号。 成功返回 0
		int LoadFromFile(std::filesystem::path const& ign) {
			if (!std::filesystem::exists(ign)) return __LINE__;
			if (int r = xx::ReadAllBytes(ign, d)) return __LINE__;
			if (!d.len) return 0;
			if (d[0] == 0xef && d[1] == 0xbb && d[2] == 0xbf) {
				return LoadFromText({ (char*)d.buf + 3, d.len - 3 });
			}
			return LoadFromText({ (char*)d.buf, d.len }, true);
		}

		// 从 string 加载 ignore 文本
		int LoadFromText(std::string_view sv, bool dFilled = false) {
			if (!dFilled) {
				d.Clear();
				d.WriteBuf(sv.data(), sv.size());
				sv = { (char*)d.buf, d.len };
			}
			std::vector<std::string_view> strs;
			std::ranges::for_each(sv | std::views::split('\n'), [&](auto const& v) {
				if (auto siz = v.size()) {
					if (v.back() == '\r') --siz;
					if (siz) {
						std::string_view s(v.data(), siz);
						auto a = s.find_first_not_of(" \t");
						auto b = s.find_last_not_of(" \t");
						s = { v.data() + a, b - a + 1 };											// trim
						if (s.size() && s[0] != '#') {												// 跳过注释和空行
							strs.emplace_back(s);
						}
					}
				}
			});

			for (auto& s : strs) {
				ToLower((char*)s.data(), s.size());													// A -> a, / -> \ 
			}

			// 已知问题，这里并没有多少更严格的 check, 写错也不报错
			for (auto& o : strs) {
				std::string_view s(o);
				std::vector<std::unique_ptr<IgnoreMacther>>* container;
				if (s[0] == '!') {
					container = &unmatchers;
					s = s.substr(1);
				} else {
					container = &matchers;
				}
				if (s[0] == '\\') {
					s = s.substr(1);
					if (s.find('*') == s.npos) {
						container->emplace_back(std::make_unique<IgnoreMatcher_RootDir>(s));
					} else {
						container->emplace_back(std::make_unique<IgnoreMatcher_RootFiles>(s));
					}
				} else {
					if (s.find('*') == s.npos) {
						container->emplace_back(std::make_unique<IgnoreMatcher_Dir>(s));
					} else {
						container->emplace_back(std::make_unique<IgnoreMatcher_Files>(s));
					}
				}
			}

			return 0;
		}

		// 适配判断( 线程安全 )
		bool Math(std::string_view tar, bool ignoreUpperCase = true) const {
			if (matchers.empty()) return false;
			std::string tmp;
			if (ignoreUpperCase) {
				tmp = tar;
				ToLower(tmp.data(), tmp.size());
				tar = tmp;
			}
			for (auto& h : unmatchers) {
				if (h->Math(tar)) return false;
			}
			for (auto& h : matchers) {
				if (h->Math(tar)) return true;
			}
			return false;
		}

		// 适配判断( 线程安全 )
		bool operator()(std::string_view tar, bool ignoreUpperCase = true) const {
			return Math(tar);
		}

	protected:
		// 大写转小写, / 转 \ 
		void ToLower(char* b, size_t len) const {
			for (auto e = b + len; b != e; ++b) {
				if (*b == '/') {
					*b = '\\';
				} else if (*b >= 'A' && *b <= 'Z') {
					*b += 'a' - 'A';
				}
			}
		}
	};
}

// test cases


//	im.LoadFromText(R"(
//     \sdf
//)sv");
//	assert(true == im("sdf"));
//	assert(false == im("asdf"));
//	assert(false == im("sdfg"));
//	assert(false == im("asdfg"));
//	assert(true == im("sdf\\qwer"));
//	assert(false == im("asdf\\qwer"));
//	assert(false == im("sdfb\\qwer"));
//	assert(false == im("asdfb\\qwer"));
//	assert(false == im("qwer\\sdf"));
//	assert(false == im("qwer\\sdfa"));
//	assert(false == im("qwer\\bsdf"));
//	assert(false == im("qwer\\bsdfa"));
//	assert(false == im("qwer\\sdf\\zxcv"));


//	im.LoadFromText(R"(
//     sdf
//)sv");
//	assert(true == im("sdf"));
//	assert(false == im("asdf"));
//	assert(false == im("sdfg"));
//	assert(false == im("asdfg"));
//	assert(true == im("sdf\\qwer"));
//	assert(false == im("asdf\\qwer"));
//	assert(false == im("sdfb\\qwer"));
//	assert(false == im("asdfb\\qwer"));
//	assert(true == im("qwer\\sdf"));
//	assert(false == im("qwer\\sdfa"));
//	assert(false == im("qwer\\bsdf"));
//	assert(false == im("qwer\\bsdfa"));
//	assert(true == im("qwer\\sdf\\zxcv"));


//	im.LoadFromText(R"(
//     *.xxx
//)sv");
//	assert(true == im(".xxx"));
//	assert(true == im("a.xxx"));
//	assert(true == im("sdf\\.xxx"));
//	assert(true == im("sdf\\a.xxx"));
//	assert(false == im(".xxxx"));
//	assert(false == im("a.xxxx"));
//	assert(false == im("sdf\\.xxxx"));
//	assert(false == im("sdf\\a.xxxx"));


//	im.LoadFromText(R"(
//     \*.xxx
//)sv");
//	assert(true == im(".xxx"));
//	assert(true == im("a.xxx"));
//	assert(false == im("sdf\\.xxx"));
//	assert(false == im("sdf\\a.xxx"));
//	assert(false == im(".xxxx"));
//	assert(false == im("a.xxxx"));
//	assert(false == im("sdf\\.xxxx"));
//	assert(false == im("sdf\\a.xxxx"));


//	im.LoadFromText(R"(
//.vs
//*.mp3
//!1.mp3
//)sv");
//
//	assert(true == im(".vs"));
//	assert(true == im(".mp3"));
//	assert(true == im("a.mp3"));
//	assert(true == im("a/b.mp3"));
//	assert(false == im("1.mp3"));
//	assert(false == im(".vs1"));
//	assert(false == im("1.vs"));
//	assert(false == im("a\\1.mp3"));
