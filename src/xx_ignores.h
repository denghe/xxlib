#pragma once
#include "xx_string.h"
#include "xx_file.h"
#include <ranges>
namespace xx {

	// �� .gitignore ���ܵ� ��ʵ�֡�
	// ֻ֧�����������
	// 1. Ŀ¼�� �� �ļ���. ǰ�� �ɼ� \ ���޶�Ϊ��Ŀ¼
	// 2. * ͨ�� �ļ���, ֻ֧�� 1 ��. * �Ҳ������ ����Ϊ һ�����������ұ�
	// 3. �� ! ��ͷ �� 1, 2 ���Ϊ �����ȼ� ������

	// ƥ��������
	struct IgnoreMacther {
		virtual ~IgnoreMacther() {}
		virtual bool Math(std::string_view const& tar) = 0;
	};

	// ƥ�� \Ŀ¼��
	struct IgnoreMatcher_RootDir : IgnoreMacther {
		std::string_view s;
		explicit IgnoreMatcher_RootDir(std::string_view const& s) : s(s) {}
		virtual bool Math(std::string_view const& t) override {
			if (t.size() < s.size()) return false;													// ���Ȳ���: false
			if (t.starts_with(s)) {																	// �� s ��ͷ
				if (t.size() == s.size()) return true;												// �ұ�û��, ��ȫ��ͬ: true
				return t[s.size()] == '\\';															// ���� �ұ�Ŀ¼�Ƿ�����
			}
			return false;
		}
	};

	// ƥ�� Ŀ¼��
	struct IgnoreMatcher_Dir : IgnoreMacther {
		std::string_view s;
		explicit IgnoreMatcher_Dir(std::string_view const& s) : s(s) {}
		virtual bool Math(std::string_view const& t) override {
			if (t.size() < s.size()) return false;													// ���Ȳ���: false
			if (auto i = t.find(s); i != t.npos) {													// �ҵ���
				if (t.size() == s.size()) return true;												// ��ȫ��ͬ: true
				if (i == 0) return t[s.size()] == '\\';												// ͬ RootDir �߼�
				else {
					if (t[i - 1] != '\\') return false;												// ���Ŀ¼������: false
					if (i + s.size() == t.size()) return true;										// �ұ�û��, ��ȫ��ͬ: true
					return t[i + s.size()] == '\\';													// ���� �ұ�Ŀ¼�Ƿ�����
				}
			}
			return false;
		}
	};

	// ƥ�� \[Ŀ¼��\Ŀ¼��...\]*��չ��
	struct IgnoreMatcher_RootFiles : IgnoreMacther {
		std::string_view path, ext;
		size_t minSize;
		explicit IgnoreMatcher_RootFiles(std::string_view const& s) {
			path = s.substr(0, s.find('*'));
			ext = s.substr(path.size() + 1);
			minSize = path.size() + ext.size();
		}
		virtual bool Math(std::string_view const& t) override {
			if (t.size() < minSize) return false;													// ���Ȳ���: false
			if (t.starts_with(path) && t.ends_with(ext)) {											// �� path ��ͷ, ext ����
				auto fn = t.substr(path.size(), t.size() - minSize);
				return fn.find('\\') == fn.npos;													// ���� fn ���Ƿ񲻺� /
			}
			return false;
		}
	};

	// ƥ�� [Ŀ¼��\Ŀ¼��...\]*��չ��
	struct IgnoreMatcher_Files : IgnoreMacther {
		std::string_view path, ext;
		size_t minSize;
		explicit IgnoreMatcher_Files(std::string_view const& s) {
			path = s.substr(0, s.find('*'));
			ext = s.substr(path.size() + 1);
			minSize = path.size() + ext.size();
		}
		virtual bool Math(std::string_view const& t) override {
			if (t.size() < minSize) return false;													// ���Ȳ���: false
			if (!t.ends_with(ext)) return false;													// ���� ext ����: false
			auto left = t.substr(0, t.size() - ext.size());											// �е� ext
			if (auto i = left.find_last_of('\\'); i == left.npos) return !path.size(); 				// ������������ļ���. path Ϊ��: true
			else {
				left = left.substr(0, i);															// �е� /fn
				return left.ends_with(path);														// �� path ��β: true
			}
			if (auto i = t.find(path); i == t.npos) return false;									// δ�ҵ� path: false
			else if (i == 0 || t[i - 1] != '\\') {													// ���Ŀ¼����
				auto fn = t.substr(i + path.size(), t.size() - i - minSize);
				return fn.find('\\') == fn.npos;													// ���� fn ���Ƿ񲻺� /
			}
			return false;
		}
	};

	// ƥ�������
	struct IgnoresManager {
		xx::Data d;																					// ���� string hold �ڴ�
		std::vector<std::unique_ptr<IgnoreMacther>> matchers;										// ����������������
		std::vector<std::unique_ptr<IgnoreMacther>> unmatchers;										// ������...

		// ���ļ����� ignore �ı�. ʧ�ܷ��� �кš� �ɹ����� 0
		int LoadFromFile(std::filesystem::path const& ign) {
			if (!std::filesystem::exists(ign)) return __LINE__;
			if (int r = xx::ReadAllBytes(ign, d)) return __LINE__;
			if (!d.len) return 0;
			if (d[0] == 0xef && d[1] == 0xbb && d[2] == 0xbf) {
				return LoadFromText({ (char*)d.buf + 3, d.len - 3 });
			}
			return LoadFromText({ (char*)d.buf, d.len }, true);
		}

		// �� string ���� ignore �ı�
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
						if (s.size() && s[0] != '#') {												// ����ע�ͺͿ���
							strs.emplace_back(s);
						}
					}
				}
			});

			for (auto& s : strs) {
				ToLower((char*)s.data(), s.size());													// A -> a, / -> \ 
			}

			// ��֪���⣬���ﲢû�ж��ٸ��ϸ�� check, д��Ҳ������
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

		// �����ж�( �̰߳�ȫ )
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

		// �����ж�( �̰߳�ȫ )
		bool operator()(std::string_view tar, bool ignoreUpperCase = true) const {
			return Math(tar);
		}

	protected:
		// ��дתСд, / ת \ 
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
