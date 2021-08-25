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
	FS::path tar(args[1]);
	std::string_view baseUrl(args[2]);
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
	if (!baseUrl.starts_with("http") || !baseUrl.ends_with("/")) {
		std::cout << "error: arg2 is not a valid url.\n" << std::endl;
		return -3;
	}

	// ����Ƿ�У��
	bool needVerify = verify == "verify";

	// ׼���������� ������ɶ��
	FilesIndexJson::Files fs;
	xx::Data d;
	auto rootPathLen = tar.string().size() + 1;	// �����и����·��
	xx::Downloader dl;
	dl.SetBaseUrl(baseUrl);

	// д url
	fs.baseUrl = baseUrl;

	// ��ʼ�ݹ����������
	for (auto&& o : FS::recursive_directory_iterator(tar)) {
		auto&& p = o.path();

		// �����ļ�, û���ļ���, 0�ֽ� ������
		if (!o.is_regular_file() || !p.has_filename()) continue;

		// �����ʼ������ļ���
		std::cout << "file: " << p << std::endl;

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
			std::cout << "begin download " << (fs.baseUrl + f.path) << std::endl;
			dl.Download(f.path);
			std::pair<size_t, size_t> curr, bak;
			while (!dl.TryGetProgress(curr)) {
				if (bak != curr) {
					bak = curr;
					std::cout << "downloading... " << curr.first << " / " << curr.second << std::endl;
				}
			}

			if (!dl) {
				std::cout << "url: " << (fs.baseUrl + f.path) << " download error." << std::endl;
				return -5;
			}

			if (d != dl.data) {
				std::cout << "file: " << o.path() << " verify failed. the file content is different." << std::endl;
				return -6;
			}

			std::cout << "verify success!" << std::endl;
		}

		// д�� len
		f.len = d.len;

		// д�� md5
		f.md5 = GetDataMD5Hash(d);

		// ���һ�� f �� json
		std::cout << "json: ";
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
