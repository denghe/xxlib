#pragma once

#include <httplib.h>
#include <xx_data.h>
#include <atomic>

namespace xx {
	// ���Ҫ֧�� https ����Ҫ���� openssl ������ĳ������ httplib �����Ӧ����

	// �Դ��̵߳�������. ���ڲ�д�ļ����ʿ��ܲ��ʺ����ش����ڴ�ռ���ļ���
	struct Downloader {
		Downloader() = default;
		Downloader(Downloader const&) = delete;
		Downloader& operator=(Downloader const&) = delete;
		~Downloader() { Close(); }

		// ��̨�����߳�
		std::thread t;
		// �������������
		std::shared_ptr<httplib::Client> cli;
		// ���� URL���������·��, ���·����ǰ׺( �ӻ��� URL �и��� )
		std::string baseUrl, path, pathPrefix;
		// ���ص�����
		xx::Data data;
		// flags
		// �߳�����ִ��
		std::atomic<bool> running = false;
		// ��ֹͣ����
		std::atomic<bool> stoped = true;
		// ��ȡ������
		std::atomic<bool> canceled = false;
		// ��������
		std::atomic<bool> finished = false;
		// �������ص��ļ��� ��ǰ�����ܳ�
		std::atomic<size_t> len, total;

		// ���� baseUrl��ʾ��: http://xxx.xxx ����û�� / û�б�� path. ������ж��� path, �򽫴��� pathPrefix
		// ������
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

		// ��ʼ���ء�path ʾ����/xxx/abc.png �� / ��ͷ
		// �������� URL = baseUrl + pathPrefix + path
		// �������ص�ʱ�򲻿ɵ���. �� Busy ������Ƿ���������
		// ���ص����� λ�� data ��Ա
		// ������
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

		// ��ȡ��ǰ���ؽ��� pair: len, total. �����Ƿ���������( �Ƿ��������Ҫ�ж� finished )
		// ע�⣺�׸� total ����Ϊ 0
		// ������
		bool TryGetProgress(std::pair<size_t, size_t>& lenTotal) const {
			lenTotal.first = len;
			lenTotal.second = total;
			return !stoped;
		}

		// �����Ƿ���������
		// ������
		bool Busy() const { return !stoped; }

		// �����Ƿ���ֹͣ����
		// ������
		bool Stoped() const { return stoped; }

		// �����Ƿ����� "����" ( ������� )
		// ������
		bool Finished() const { return finished; }

		// ȡ�����أ����ߣ���������( �����������Զ��������, �Ա���ɱ���̳��� )
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

		// ��̨�̴߳���
		void Process() {
			// һֱִ��
			while (true) {

				// �ȴ�����ָ��
				while (stoped) {
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
					[this](const char* buf, size_t len) {
						data.WriteBuf(buf, len);
						return true;
					}, [this](uint64_t len, uint64_t total)->bool {
						this->len = len;
						this->total = total;
						return !canceled;
					}
					);

				// ���û������ͱ�����سɹ�
				finished = (bool)res;

				// ������ؽ���
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