// 扫描指定目录下所有文件，生成文件索引 json

#include "files_idx_json.h"
#include "get_md5.h"
#include <iostream>
#include <xx_file.h>
namespace FS = std::filesystem;


#include "../cpp-httplib/httplib.h"

// todo

int main(int numArgs, char** args) {

	// todo



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
	FS::path tar(args[1]);
	std::string_view url(args[2]);
	std::string_view verify;
	if (numArgs > 3) {
		verify = args[3];
	}

	// 检查目录是否存在
	if (!FS::exists(tar) && !FS::is_directory(tar)) {
		std::cout << "error: arg1 target is not a directory.\n" << std::endl;
		return -2;
	}

	// 简单检查 url 是否合法
	if (!url.starts_with("http") || !url.ends_with("/")) {
		std::cout << "error: arg2 is not a valid url.\n" << std::endl;
		return -3;
	}

	// 检查是否校验
	bool needVerify = verify == "verify";

	// 准备数据容器
	FilesIndexJson::Files fs;
	xx::Data d, d2;
	auto rootPathLen = tar.string().size() + 1;	// 用来切割相对路径

	// 写 url
	fs.baseUrl = url;

	// 开始递归遍历并生成
	for (auto&& o : FS::recursive_directory_iterator(tar)) {
		auto&& p = o.path();

		// 不是文件, 没有文件名, 0字节 就跳过
		if (!o.is_regular_file() || !p.has_filename()) continue;

		// 创建一个 File 实例，开始填充
		auto& f = fs.files.emplace_back();

		// 写入 path 并换 \\ 为 /
		f.path = p.string().substr(rootPathLen);
		for (auto& c : f.path) if (c == '\\') c = '/';

		// 读出文件所有数据( 这里比较粗暴，如果文件体积大于可用内存，那就没法搞了 )
		if (int r = xx::ReadAllBytes(p, d)) {
			std::cout << "file read error: r = " << r << std::endl;
			return -4;
		}

		// 按需校验
		if (needVerify) {
			// todo: 下载文件到 d2
			if (d != d2) {
				std::cout << "file: "<< o.path() <<" verify failed. the file content is different." << std::endl;
				return -5;
			}
		}

		// 写入 len
		f.len = d.len;

		// 写入 md5
		f.md5 = GetDataMD5Hash(d);

		// 输出一下 f 的 json
		ajson::save_to(std::cout, f);
		std::cout << std::endl << std::endl;
	}
	
	// 文件写入当前工作目录
	auto outFilePath = FS::current_path() / "files_idx.json";
	ajson::save_to_file(fs, outFilePath.string().c_str());

	std::cout << "success! handled " << fs.files.size() << " files!" << std::endl;
	std::cin.get();
	return 0;
}
