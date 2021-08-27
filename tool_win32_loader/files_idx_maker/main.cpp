// ɨ��ָ��Ŀ¼�������ļ��������ļ����� json

#include <iostream>
#include "files_idx_json.h"
#include "xx_downloader.h"
#include "get_md5.h"
#include <xx_file.h>
namespace FS = std::filesystem;

int main(int numArgs, char** args) {

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
	FS::path rootPath(args[1]);
	std::string_view baseUrl(args[2]);
	std::string_view verify;
	if (numArgs > 3) {
		verify = args[3];
	}

	// ���Ŀ¼�Ƿ����
	if (!FS::exists(rootPath) && !FS::is_directory(rootPath)) {
		std::cout << "error: arg1 target is not a directory.\n" << std::endl;
		return -2;
	}

	// �򵥼�� url �Ƿ�Ϸ�
	if (!baseUrl.starts_with("http") || !baseUrl.ends_with("/")) {
		std::cout << "error: arg2 is not a valid url.\n" << std::endl;
		return -3;
	}

	// ����Ƿ�У��
	bool needVerify = verify == "verify";

	// ׼���������� ������ɶ��
	FilesIndexJson::Files fs;
	// д url
	fs.baseUrl = baseUrl;
	// �����и����·��
	auto rootPathLen = rootPath.string().size() + 1;

	// for easy code
	auto& ff = fs.files;

	// ��ʼ�ݹ����������
	for (auto&& o : FS::recursive_directory_iterator(rootPath)) {
		auto&& p = o.path();

		// �����ļ�, û���ļ���, 0�ֽ� ������
		if (!o.is_regular_file() || !p.has_filename()) continue;

		// ����һ�� File ʵ������ʼ���
		auto& f = ff.emplace_back();

		// д�� path ���� \\ Ϊ /
		f.path = p.string().substr(rootPathLen);
		for (auto& c : f.path) if (c == '\\') c = '/';

	}

	// ����У���߳���
	static const size_t numThreads = 8;

	// ��ʱ���ļ�����
	std::array<xx::Data, numThreads> ds;

	// ������
	std::array<xx::Downloader, numThreads> dls;

	// ��ʼ������������ url
	for (auto& dl : dls) dl.SetBaseUrl(baseUrl);

	// �̳߳�
	std::array<std::thread, numThreads> ts;

	// �ȴ������߳��깤����������
	std::atomic<int> counter;

	// �����߳��±�, ����ÿ���̶߳�λ���Լ�������������
	for (size_t i = 0; i < numThreads; ++i) {

		// �� i ���߳�ʹ���±� i �� ds & dls
		ts[i] = std::thread([&, i = i] {
			auto& d = ds[i];
			auto& dl = dls[i];

			// ÿ�߳���ʼ�±�������Ƹ���ɨ��
			for (size_t j = i; j < ff.size(); j += numThreads) {
				auto& f = ff[j];
				auto p = rootPath / f.path;

				// �����ļ���������( ����Ƚϴֱ�������ļ�������ڿ����ڴ棬�Ǿ�û������ )
				if (int r = xx::ReadAllBytes(p, d)) {
					std::cout << "file read error: r = " << r << std::endl;
					return;
				}

				// ����У��
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

				// д�� len
				f.len = d.len;

				// д�� md5
				f.md5 = GetDataMD5Hash(d);

				std::cout << ".";
			}

			// ������������
			++counter;
		});
		ts[i].detach();
	}

	// �������߳����
	while (counter < numThreads) Sleep(1);

	std::cout << std::endl;

	// �ļ�д�뵱ǰ����Ŀ¼
	auto outFilePath = FS::current_path() / "files_idx.json";
	ajson::save_to_file(fs, outFilePath.string().c_str());

	std::cout << "success! handled " << ff.size() << " files!" << std::endl;
	std::cin.get();
	return 0;
}
