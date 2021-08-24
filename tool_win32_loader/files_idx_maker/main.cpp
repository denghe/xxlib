// ɨ��ָ��Ŀ¼�������ļ��������ļ����� json

#include "files_idx_json.h"
#include "get_md5.h"
#include <iostream>
#include <xx_file.h>
namespace FS = std::filesystem;


#include "../cpp-httplib/httplib.h"

// todo

int main(int numArgs, char** args) {

	// todo



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
				std::cout << "file: "<< o.path() <<" verify failed. the file content is different." << std::endl;
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
