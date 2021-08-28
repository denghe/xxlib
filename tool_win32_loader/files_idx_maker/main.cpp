// ɨ��ָ��Ŀ¼�������ļ��������ļ����� json

#include <iostream>
#include <xx_threadpool.h>
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

	// ����̳߳ظ�����
	{
		// �̳߳ع���������
		struct Ctx {
			XX_SIMPLE_STRUCT_DEFAULT_CODES(Ctx);
			// ������
			xx::Downloader dl;
			// ��ʱ���ļ�����
			xx::Data d;
		};

		// ����̳߳�
		xx::ThreadPool2<Ctx, 8> tp;

		// ��ʼ��������
		for (auto& c : tp.envs) {
			c.dl.SetBaseUrl(baseUrl);
		}

		// ���̳߳�ѹ����, �Ƚ���. ��;��������, ���� f.len == 0 �����, �����ı��� md5
		for (auto& f : ff) {

			// �������� f ��ָ�뵽�̺߳�����f ����ֱ��& ( ÿ�� for �仯 ���ҳ� for ��û�� )
			tp.Add([&, f = &f](Ctx& c) {

				// �����ļ�����·��
				auto p = rootPath / f->path;

				// �����ļ���������( ����Ƚϴֱ�������ļ�������ڿ����ڴ棬�Ǿ�û������ )
				if (int r = xx::ReadAllBytes(p, c.d)) {
					f->md5 = std::string("file read error: r = ") + std::to_string(r);
					std::cout << "#";
					return;
				}

				// ����У��
				if (needVerify) {

					// ���ز��ȴ�����
					c.dl.Download(f->path);
					while (c.dl.Busy()) Sleep(1);

					// �쳣: ���س���
					if (!c.dl.Finished()) {
						f->md5 = std::string("url: ") + (fs.baseUrl + f->path) + " download error";
						std::cout << "!";
						return;
					}

					// �쳣: ���ݶԲ���
					if (c.d != c.dl.data) {
						f->md5 = std::string("file: ") + p.string() + " verify failed. the file content is different";
						std::cout << "*";
						return;
					}

					// ��������ɶ �Եý���û��
					std::cout << ",";
				}

				// д�� len
				f->len = c.d.len;

				// д�� md5
				f->md5 = GetDataMD5Hash(c.d);

				// ���Ϊͨ��
				f->ok = true;

				// ��������ɶ �Եý���û��
				std::cout << ".";
			});
		}
	}
	// ִ�е��˴�ʱ���̳߳������

	// ��һ��
	std::cout << std::endl;

	// �������, ��ӡ & �˳�
	bool ok = true;
	for (auto& f : ff) {
		// ������ 0 ���ļ�( �����ڳ��� )
		if (!f.len) {
			std::cout << f.path << std::endl << "warning!!! this file's len == 0" << std::endl << std::endl;
		}
		// ������
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

	// �ļ�д�뵱ǰ����Ŀ¼
	auto outFilePath = FS::current_path() / "files_idx.json";
	ajson::save_to_file(fs, outFilePath.string().c_str());

	// ��ӡ�ɹ����˳�
	std::cout << "success! handled " << ff.size() << " files! data saved to " << outFilePath << "!!! press any to continue..." << std::endl;
	std::cin.get();
	return 0;
}
