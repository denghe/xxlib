// 扫描指定目录下所有文件，生成文件索引 json

#include <iostream>
#include <xx_threadpool.h>
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

	// 起个线程池搞事情
	{
		// 线程池共享上下文
		struct Ctx {
			XX_SIMPLE_STRUCT_DEFAULT_CODES(Ctx);
			// 下载器
			xx::Downloader dl;
			// 临时读文件容器
			xx::Data d;
		};

		// 起个线程池
		xx::ThreadPool2<Ctx, 8> tp;

		// 初始化上下文
		for (auto& c : tp.envs) {
			c.dl.SetBaseUrl(baseUrl);
		}

		// 往线程池压任务, 等结束. 中途发生错误, 利用 f.len == 0 来表达, 错误文本在 md5
		for (auto& f : ff) {

			// 拷贝捕获 f 的指针到线程函数。f 不可直接& ( 每次 for 变化 并且出 for 就没了 )
			tp.Add([&, f = &f](Ctx& c) {

				// 计算文件完整路径
				auto p = rootPath / f->path;

				// 读出文件所有数据( 这里比较粗暴，如果文件体积大于可用内存，那就没法搞了 )
				if (int r = xx::ReadAllBytes(p, c.d)) {
					f->md5 = std::string("file read error: r = ") + std::to_string(r);
					std::cout << "#";
					return;
				}

				// 按需校验
				if (needVerify) {

					// 下载并等待下完
					c.dl.Download(f->path);
					while (c.dl.Busy()) Sleep(1);

					// 异常: 下载出错
					if (!c.dl.Finished()) {
						f->md5 = std::string("url: ") + (fs.baseUrl + f->path) + " download error";
						std::cout << "!";
						return;
					}

					// 异常: 内容对不上
					if (c.d != c.dl.data) {
						f->md5 = std::string("file: ") + p.string() + " verify failed. the file content is different";
						std::cout << "*";
						return;
					}

					// 随便输出点啥 显得进程没死
					std::cout << ",";
				}

				// 写入 len
				f->len = c.d.len;

				// 写入 md5
				f->md5 = GetDataMD5Hash(c.d);

				// 标记为通过
				f->ok = true;

				// 随便输出点啥 显得进程没死
				std::cout << ".";
			});
		}
	}
	// 执行到此处时，线程池已完结

	// 空一行
	std::cout << std::endl;

	// 如果出错, 打印 & 退出
	bool ok = true;
	for (auto& f : ff) {
		// 警告下 0 长文件( 不属于出错 )
		if (!f.len) {
			std::cout << f.path << std::endl << "warning!!! this file's len == 0" << std::endl << std::endl;
		}
		// 出错了
		if (!f.ok) {
			ok = false;
			std::cout << f.path << std::endl << f.md5 << std::endl << std::endl;
		}
	}
	if (!ok) {
		std::cout << "failed! data not save!!! press any to continue..." << std::endl;
		std::cin.get();
		return 1;
	}

	// 文件写入当前工作目录
	auto outFilePath = FS::current_path() / "files_idx.json";
	ajson::save_to_file(fs, outFilePath.string().c_str());

	// 打印成功，退出
	std::cout << "success! handled " << ff.size() << " files! data saved to " << outFilePath << "!!! press any to continue..." << std::endl;
	std::cin.get();
	return 0;
}
