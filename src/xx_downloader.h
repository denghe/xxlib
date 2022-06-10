#pragma once

#include <httplib.h>
#include <xx_data.h>
#include <atomic>

namespace xx {
	// 如果要支持 https 则需要包含 openssl 并设置某宏启用 httplib 里的相应功能

	// 自带线程的下载器. 由于不写文件，故可能不适合下载大于内存空间的文件。
	struct Downloader {
		Downloader() = default;
		Downloader(Downloader const&) = delete;
		Downloader& operator=(Downloader const&) = delete;
		~Downloader() { Close(); }

		// 后台下载线程
		std::thread t;
		// 下载器核心组件
		std::shared_ptr<httplib::Client> cli;
		// 基础 URL，下载相对路径, 相对路径的前缀( 从基础 URL 切割来 )
		std::string baseUrl, path, pathPrefix;
		// 下载的数据
		xx::Data data;
		// flags
		// 线程正在执行
		std::atomic<bool> running = false;
		// 已停止下载
		std::atomic<bool> stoped = true;
		// 已取消下载
		std::atomic<bool> canceled = false;
		// 已下载完
		std::atomic<bool> finished = false;
		// 正在下载的文件的 当前长，总长
		std::atomic<size_t> len, total;

		// 设置 baseUrl。示例: http://xxx.xxx 后面没有 / 没有别的 path. 如果带有额外 path, 则将存入 pathPrefix
		// 非阻塞
		void SetBaseUrl(std::string_view const& baseUrl) {
			assert(!running);
			assert(baseUrl.starts_with("http://") || baseUrl.starts_with("https://"));
			assert(baseUrl.size() > 9);
			if (auto idx = baseUrl.find('/', 9); idx != std::string::npos) {
				this->baseUrl = baseUrl.substr(0, idx);
				pathPrefix = baseUrl.substr(idx);
			}
			else {
				this->baseUrl = baseUrl;
				pathPrefix = "/";
			}
		}

		// 开始下载。path 示例：/xxx/abc.png 以 / 打头
		// 最终下载 URL = baseUrl + pathPrefix + path
		// 正在下载的时候不可调用. 用 Busy 来检查是否正在下载
		// 下载的数据 位于 data 成员
		// 非阻塞
		void Download(std::string const& path) {
			assert(stoped);
			this->path = pathPrefix + path;
			data.Clear();
			len = 0;
			total = 0;
			canceled = false;
			stoped = false;
			finished = false;

			if (!running) {
				running = true;
				t = std::thread([this] { this->Process(); });
			}
		}

		// 读取当前下载进度 pair: len, total. 返回是否正在下载( 是否下载完成要判断 finished )
		// 注意：首个 total 可能为 0
		// 非阻塞
		bool TryGetProgress(std::pair<size_t, size_t>& lenTotal) const {
			lenTotal.first = len;
			lenTotal.second = total;
			return !stoped;
		}

		// 返回是否正在下载
		// 非阻塞
		bool Busy() const { return !stoped; }

		// 返回是否已停止下载
		// 非阻塞
		bool Stoped() const { return stoped; }

		// 返回是否下载 "完整" ( 不是完成 )
		// 非阻塞
		bool Finished() const { return finished; }

		// 取消下载，掐线，销毁所有( 析构并不会自动调用这个, 以避免杀进程出错 )
		void Close() {
			if (t.joinable()) {
				canceled = true;
				running = false;
				t.join();
			}

			cli.reset();
			baseUrl.shrink_to_fit();
			path.shrink_to_fit();
			data.Clear(true);
			running = false;
			stoped = true;
			canceled = false;
			len = 0;
			total = 0;
		}

		// 后台线程代码
		void Process() {
			// 一直执行
			while (true) {

				// 等待下载指令
				while (stoped) {
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
					[this](const char* buf, size_t len) {
						data.WriteBuf(buf, len);
						return true;
					}, [this](uint64_t len, uint64_t total)->bool {
						this->len = len;
						this->total = total;
						return !canceled;
					}
					);

				// 如果没出问题就标记下载成功
				finished = (bool)res;

				// 标记下载结束
				stoped = true;
			}
		}
	};
}

/*
* example:

Downloader d;
d.SetBaseUrl("http://192.168.1.200");

while (true) {
	d.Download("/www/runtime/libogg.dll");
	{
		std::pair<size_t, size_t> curr, bak;
		while (!d.TryGetProgress(curr)) {
			if (bak != curr) {
				bak = curr;
				std::cout << "downloading: " << curr.first << " / " << curr.second << std::endl;
			}
		}

		std::cout << (d ? "finished" : "error/canceled") << std::endl;
	}
}

*/