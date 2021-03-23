#pragma once
#include "xx_data.h"
#include "xx_scopeguard.h"
#include <fstream>
#include <filesystem>

namespace xx
{
	inline int ReadAllBytes(std::filesystem::path const& path, Data& d) noexcept {
		std::ifstream f(path, std::ifstream::binary);
		if (!f) return -1;						// not found? no permission? locked?
		xx::ScopeGuard sg([&] { f.close(); });
		f.seekg(0, f.end);
		auto&& siz = f.tellg();
		if ((uint64_t)siz > std::numeric_limits<size_t>::max()) return -2;	// too big
		f.seekg(0, f.beg);
		d.Clear();
		d.Reserve(siz);
		f.read((char*)d.buf, siz);
		if (!f) return -3;						// only f.gcount() could be read
		d.len = siz;
		return 0;
	}

	inline int ReadAllBytes(std::filesystem::path const& path, std::unique_ptr<uint8_t[]>& outBuf, size_t& outSize) noexcept {
		outBuf.reset();
		outSize = 0;
		std::ifstream f(path, std::ifstream::binary);
		if (!f) return -1;													// not found? no permission? locked?
		ScopeGuard sg([&] { f.close(); });
		f.seekg(0, f.end);
		auto&& siz = f.tellg();
		if ((uint64_t)siz > std::numeric_limits<size_t>::max()) return -2;	// too big
		f.seekg(0, f.beg);
		auto&& buf = new(std::nothrow) uint8_t[siz];
		if (!buf) return -3;												// not enough memory
		outBuf = std::unique_ptr<uint8_t[]>(buf);
		f.read((char*)buf, siz);
		if (!f) return -3;													// only f.gcount() could be read
		outSize = siz;
		return 0;
	}

	inline int WriteAllBytes(std::filesystem::path const& path, char const* const& buf, size_t const& len) noexcept {
		std::ofstream f(path, std::ios::binary | std::ios::trunc);
		if (!f) return -1;						// no create permission? exists readonly?
		xx::ScopeGuard sg([&] { f.close(); });
		f.write(buf, len);
		if (!f) return -2;						// write error
		return 0;
	}

	inline int WriteAllBytes(std::filesystem::path const& path, Data const& bb) noexcept {
		return WriteAllBytes(path, (char*)bb.buf, bb.len);
	}

	template<size_t len>
	inline int WriteAllBytes(std::filesystem::path const& path, char const(&s)[len]) noexcept {
		return WriteAllBytes(path, s, len - 1);
	}

	inline std::filesystem::path GetCurrentPath() {
		return std::filesystem::absolute("./");
	}


	// todo: more

	//std::cout << std::filesystem::current_path() << std::endl;
	//for (auto&& entry : std::filesystem::recursive_directory_iterator(std::filesystem::current_path())) {
	//	std::cout << entry << std::endl;
	//	std::cout << std::filesystem::absolute(entry) << std::endl;
	//	std::cout << entry.is_regular_file() << std::endl;
	//}
	//try {
	//	auto siz = std::filesystem::file_size(L"中文文件名.txt");
	//	std::cout << siz << std::endl;
	//}
	//catch (std::filesystem::filesystem_error& ex) {
	//	std::cout << ex.what() << std::endl;
	//}

}
