// 扫描指定目录下所有文件，生成文件索引 json

#include <iostream>
#include "files_idx_json.h"
#include "xx_downloader.h"
#include "get_md5.h"
#include <xx_file.h>
namespace FS = std::filesystem;

int main(int numArgs, char** args) {

	// 参数个数检查
	if (numArgs < 3) {
		std::cout << R"(files index json generator tool. 
need 2/3 args: 
1. target directory
2. download base url with suffix '/'
3. "verify" tool will download from url and compare file's content.
example: files_idx_maker.exe C:\res\files http://abc.def/files/ verify
)" << std::endl;
		return -1;
	}

	// 参数转储
	FS::path rootPath(args[1]);
	std::string_view baseUrl(args[2]);
	std::string_view verify;
	if (numArgs > 3) {
		verify = args[3];
	}

	// 检查目录是否存在
	if (!FS::exists(rootPath) && !FS::is_directory(rootPath)) {
		std::cout << "error: arg1 target is not a directory.\n" << std::endl;
		return -2;
	}

	// 简单检查 url 是否合法
	if (!baseUrl.starts_with("http") || !baseUrl.ends_with("/")) {
		std::cout << "error: arg2 is not a valid url.\n" << std::endl;
		return -3;
	}

	// 检查是否校验
	bool needVerify = verify == "verify";

	// 准备数据容器 下载器啥的
	FilesIndexJson::Files fs;
	// 写 url
	fs.baseUrl = baseUrl;
	// 用来切割相对路径
	auto rootPathLen = rootPath.string().size() + 1;

	// for easy code
	auto& ff = fs.files;

	// 开始递归遍历并生成
	for (auto&& o : FS::recursive_directory_iterator(rootPath)) {
		auto&& p = o.path();

		// 不是文件, 没有文件名, 0字节 就跳过
		if (!o.is_regular_file() || !p.has_filename()) continue;

		// 创建一个 File 实例，开始填充
		auto& f = ff.emplace_back();

		// 写入 path 并换 \\ 为 /
		f.path = p.string().substr(rootPathLen);
		for (auto& c : f.path) if (c == '\\') c = '/';

	}

	// 并行校验线程数
	static const size_t numThreads = 8;

	// 临时读文件容器
	std::array<xx::Data, numThreads> ds;

	// 下载器
	std::array<xx::Downloader, numThreads> dls;

	// 初始化下载器基础 url
	for (auto& dl : dls) dl.SetBaseUrl(baseUrl);

	// 线程池
	std::array<std::thread, numThreads> ts;

	// 等待所有线程完工的条件变量
	std::atomic<int> counter;

	// 产生线程下标, 便于每个线程定位到自己的上下文容器
	for (size_t i = 0; i < numThreads; ++i) {

		// 第 i 个线程使用下标 i 的 ds & dls
		ts[i] = std::thread([&, i = i] {
			auto& d = ds[i];
			auto& dl = dls[i];

			// 每线程起始下标错开，类似隔行扫描
			for (size_t j = i; j < ff.size(); j += numThreads) {
				auto& f = ff[j];
				auto p = rootPath / f.path;

				// 读出文件所有数据( 这里比较粗暴，如果文件体积大于可用内存，那就没法搞了 )
				if (int r = xx::ReadAllBytes(p, d)) {
					std::cout << "file read error: r = " << r << std::endl;
					return;
				}

				// 按需校验
				if (needVerify) {
					//std::cout << "begin download " << (fs.baseUrl + f.path) << std::endl;
					dl.Download(f.path);
					//std::pair<size_t, size_t> curr, bak;
					//while (dl.TryGetProgress(curr)) {
					//	if (bak != curr) {
					//		bak = curr;
					//		std::cout << "downloading... " << curr.first << " / " << curr.second << std::endl;
					//	}
					//}
					while (dl.Busy()) Sleep(1);

					if (!dl.Finished()) {
						std::cout << "url: " << (fs.baseUrl + f.path) << " download error." << std::endl;
						return;
					}

					if (d != dl.data) {
						std::cout << "file: " << p << " verify failed. the file content is different." << std::endl;
						return;
					}

					std::cout << "+";
				}

				// 写入 len
				f.len = d.len;

				// 写入 md5
				f.md5 = GetDataMD5Hash(d);

				std::cout << ".";
			}

			// 更新条件变量
			++counter;
		});
		ts[i].detach();
	}

	// 等所有线程完成
	while (counter < numThreads) Sleep(1);

	std::cout << std::endl;

	// 文件写入当前工作目录
	auto outFilePath = FS::current_path() / "files_idx.json";
	ajson::save_to_file(fs, outFilePath.string().c_str());

	std::cout << "success! handled " << ff.size() << " files!" << std::endl;
	std::cin.get();
	return 0;
}
