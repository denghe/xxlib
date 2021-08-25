// ɨ��ָ��Ŀ¼�������ļ��������ļ����� json

#include "files_idx_json.h"
#include "get_md5.h"
#include <iostream>
#include <xx_file.h>
namespace FS = std::filesystem;


#include "../cpp-httplib/httplib.h"
#include <atomic>

// �Դ��̵߳�������
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

	// ���� baseUrl��ʾ��: http://xxx.xxx ����û�� / û�б�� path
	void SetBaseUrl(std::string_view const& baseUrl) {
		assert(!running);
		this->baseUrl = baseUrl;
	}

	// ��ʼ���ء�path ʾ����/xxx/abc.png �� / ��ͷ���� baseUrl ƴ���γ�������������
	// �������ص�ʱ�򲻿ɵ���
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

	// ȡ������
	void Cancel() {
		canceled = true;
	}

	// �ȴ�����( �����ã�ͨ���Լ�ÿ֡ɨ finished �жϵ�ǰ�ļ��Ƿ��������꣬���� ctx ������ )
	void WaitFinish() {
		while (!finished) Sleep(1);
	}

	// ��ȡ��ǰ���ؽ���. �������ؾͷ��� true
	bool TryGetProgress(std::pair<size_t, size_t>& lenTotal) {
		if (finished) return false;
		lenTotal.first = len;
		lenTotal.second = total;
		return true;
	}

	// ֹͣ���أ����ߣ���������
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

	// ��̨�̴߳���
	void Process() {
		// һֱִ��
		while (true) {

			// �ȴ�����ָ��
			while (finished) {
				if (!running) return;
				Sleep(1);
			}

			// û����������
			if (!cli) {
				cli = std::make_shared<httplib::Client>(baseUrl);
				cli->set_keep_alive(true);
			}

			// ��ʼ����
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

			// todo: �ж� res. ����쳣�� reset cli

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


	// �����������
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

	// ����ת��
	FS::path tar(args[1]);
	std::string_view url(args[2]);
	std::string_view verify;
	if (numArgs > 3) {
		verify = args[3];
	}

	// ���Ŀ¼�Ƿ����
	if (!FS::exists(tar) && !FS::is_directory(tar)) {
		std::cout << "error: arg1 target is not a directory.\n" << std::endl;
		return -2;
	}

	// �򵥼�� url �Ƿ�Ϸ�
	if (!url.starts_with("http") || !url.ends_with("/")) {
		std::cout << "error: arg2 is not a valid url.\n" << std::endl;
		return -3;
	}

	// ����Ƿ�У��
	bool needVerify = verify == "verify";

	// ׼����������
	FilesIndexJson::Files fs;
	xx::Data d, d2;
	auto rootPathLen = tar.string().size() + 1;	// �����и����·��

	// д url
	fs.baseUrl = url;

	// ��ʼ�ݹ����������
	for (auto&& o : FS::recursive_directory_iterator(tar)) {
		auto&& p = o.path();

		// �����ļ�, û���ļ���, 0�ֽ� ������
		if (!o.is_regular_file() || !p.has_filename()) continue;

		// ����һ�� File ʵ������ʼ���
		auto& f = fs.files.emplace_back();

		// д�� path ���� \\ Ϊ /
		f.path = p.string().substr(rootPathLen);
		for (auto& c : f.path) if (c == '\\') c = '/';

		// �����ļ���������( ����Ƚϴֱ�������ļ�������ڿ����ڴ棬�Ǿ�û������ )
		if (int r = xx::ReadAllBytes(p, d)) {
			std::cout << "file read error: r = " << r << std::endl;
			return -4;
		}

		// ����У��
		if (needVerify) {
			// todo: �����ļ��� d2
			if (d != d2) {
				std::cout << "file: " << o.path() << " verify failed. the file content is different." << std::endl;
				return -5;
			}
		}

		// д�� len
		f.len = d.len;

		// д�� md5
		f.md5 = GetDataMD5Hash(d);

		// ���һ�� f �� json
		ajson::save_to(std::cout, f);
		std::cout << std::endl << std::endl;
	}

	// �ļ�д�뵱ǰ����Ŀ¼
	auto outFilePath = FS::current_path() / "files_idx.json";
	ajson::save_to_file(fs, outFilePath.string().c_str());

	std::cout << "success! handled " << fs.files.size() << " files!" << std::endl;
	std::cin.get();
	return 0;
}
