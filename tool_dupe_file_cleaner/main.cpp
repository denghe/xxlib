/*

功能：扫描当前工作目录下 *.JPG 和 *.HEIC, 如果 文件名相同，删掉 .JPG, 保留 .HEIC
主要用来解决 QNAP NAS 使用 QuMagie 上传一段时间 jpeg 之后 改设置 为 上传 heif, 从而产生很多重复内容文件的问题
另外还发现了 .jpg, .JPG 大小写两种文件名 同时存在 的问题，所以 删除时 数量可能 超过 .heic 的个数

执行后, QuMagie 软件里反应没那么快, 要等它索引, 甚至去清空垃圾桶

最后，dir *-1.* *-2.* *-3.* ... 可列出覆盖时自动重名 的重复文件. 可手工 del

*/
#include <filesystem>
namespace FS = std::filesystem;
#include <iostream>
#include <unordered_set>
#include <string>

int main() {
	auto&& cp = FS::current_path();
	std::cout << "working dir: " << cp.string() << std::endl;

	{
		std::unordered_set<std::string> paths;

		for (auto&& entry : FS::recursive_directory_iterator(cp)) {
			if (!entry.is_regular_file()) continue;
			auto&& path = entry.path();
			auto ext = path.extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
			if (ext == ".heic") {
				auto fullPath = FS::absolute(path).string();
				fullPath.erase(fullPath.size() - 5);	// cut file ext
				paths.insert(fullPath);
			}
		}
		std::cout << "found *.heic count = " << paths.size() << std::endl;

		size_t numSuccessRemovedFiles = 0;
		for (auto&& entry : FS::recursive_directory_iterator(cp)) {
			if (!entry.is_regular_file()) continue;
			auto&& path = entry.path();
			auto ext = path.extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
			if (ext == ".jpg") {
				auto fullPath = FS::absolute(path).string();
				fullPath.erase(fullPath.size() - 4);
				if (paths.contains(fullPath)) {
					if (FS::remove(path)) {
						++numSuccessRemovedFiles;
					}
				}
			}
		}
		std::cout << "delete dupe name .heic .jpg files count = " << numSuccessRemovedFiles << std::endl;
	}

	{
		std::unordered_set<std::string> paths;

		for (auto&& entry : FS::recursive_directory_iterator(cp)) {
			if (!entry.is_regular_file()) continue;
			auto&& path = entry.path();
			auto ext = path.extension().string();
			if (ext == ".JPG") {
				auto fullPath = FS::absolute(path).string();
				fullPath.erase(fullPath.size() - 4);
				paths.insert(fullPath);
			}
		}
		std::cout << "found *.JPG count = " << paths.size() << std::endl;

		size_t numSuccessRemovedFiles = 0;
		for (auto&& entry : FS::recursive_directory_iterator(cp)) {
			if (!entry.is_regular_file()) continue;
			auto&& path = entry.path();
			auto ext = path.extension().string();
			if (ext == ".jpg") {
				auto fullPath = FS::absolute(path).string();
				fullPath.erase(fullPath.size() - 4);
				if (paths.contains(fullPath)) {
					if (FS::remove(path)) {
						++numSuccessRemovedFiles;
					}
				}
			}
		}
		std::cout << "delete dupe name .JPG .jpg files count = " << numSuccessRemovedFiles << std::endl;
	}


	std::cout << "finished! press any key to continue..." << std::endl;
	std::cin.get();
	return 0;
}
