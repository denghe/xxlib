// 扫描指定目录下所有文件，生成文件索引 json

#include "files_idx_json.h"
#include "get_md5.h"
#include <iostream>
#include <xx_file.h>
namespace FS = std::filesystem;


#include "../cpp-httplib/httplib.h"
#include <atomic>

// 自带线程的下载器
struct Downloader {
	Downloader() = default;
	Downloader(Downloader const&) = delete;
	Downloader& operator=(Downloader const&) = delete;

	~Downloader() {
		Dispose();
	}

	std::thread t;
	std::shared_ptr<httplib::Client> cli;
	std::string baseUrl, path, ctx;
	std::atomic<bool> running = false, finished = true, canceled = false;
	std::atomic<size_t> len, total;

	// 设置 baseUrl。示例: http://xxx.xxx 后面没有 / 没有别的 path
	void SetBaseUrl(std::string_view const& baseUrl) {
		assert(!running);
		this->baseUrl = baseUrl;
	}

	// 开始下载。path 示例：/xxx/abc.png 以 / 打头，和 baseUrl 拼接形成完整下载链接
	// 正在下载的时候不可调用
	void Download(std::string_view const& path) {
		assert(finished);
		this->path = path;
		ctx.clear();
		len = 0;
		total = 0;
		canceled = false;
		finished = false;
		if (!running) {
			running = true;
			t = std::thread([this] { this->Process(); });
		}
	}

	// 取消下载
	void Cancel() {
		canceled = true;
	}

	// 等待下完( 不常用，通常自己每帧扫 finished 判断当前文件是否已下载完，并从 ctx 拿内容 )
	void WaitFinish() {
		while (!finished) Sleep(1);
	}

	// 读取当前下载进度. 正在下载就返回 true
	bool TryGetProgress(std::pair<size_t, size_t>& lenTotal) {
		if (finished) return false;
		lenTotal.first = len;
		lenTotal.second = total;
		return true;
	}

	// 停止下载，掐线，销毁所有
	void Dispose() {
		canceled = true;
		running = false;
		t.join();

		cli.reset();
		baseUrl.shrink_to_fit();
		path.shrink_to_fit();
		ctx.shrink_to_fit();
		running = false;
		finished = true;
		canceled = false;
		len = 0;
		total = 0;
	}

	// 后台线程代码
	void Process() {
		// 一直执行
		while (true) {

			// 等待下载指令
			while (finished) {
				if (!running) return;
				Sleep(1);
			}

			// 没有连就连线
			if (!cli) {
				cli = std::make_shared<httplib::Client>(baseUrl);
				cli->set_keep_alive(true);
			}

			// 开始下载
			auto res = cli->Get(path.c_str(),
				[this](const char* data, size_t data_length) {
					ctx.append(data, data_length);
					return true;
				}, [this](uint64_t len, uint64_t total)->bool {
					this->len = len;
					this->total = total;
					return !canceled;
				}
				);

			// todo: 判断 res. 如果异常就 reset cli

			finished = true;
		}
	}
};


int main(int numArgs, char** args) {

	{
		Downloader d;
		d.SetBaseUrl("http://192.168.1.200");

		d.Download("/www/runtime/libogg.dll");
		{
			std::pair<size_t, size_t> curr, bak;
			while (d.TryGetProgress(curr)) {
				if (bak != curr) {
					bak = curr;
					std::cout << "downloading: " << curr.first << " / " << curr.second << std::endl;
				}
			}
			std::cout << "finished" << std::endl;
		}
		d.Download("/www/runtime/libogg.dll");
		{
			std::pair<size_t, size_t> curr, bak;
			while (d.TryGetProgress(curr)) {
				if (bak != curr) {
					bak = curr;
					std::cout << "downloading: " << curr.first << " / " << curr.second << std::endl;
				}
			}
			std::cout << "finished" << std::endl;
		}
	}
	return 0;


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
				std::cout << "file: " << o.path() << " verify failed. the file content is different." << std::endl;
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
