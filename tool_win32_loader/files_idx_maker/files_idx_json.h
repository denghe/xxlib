#pragma once
#include "xx_helpers.h"
#include "ajson.hpp"
#include <filesystem>

namespace FilesIndexJson {
	struct File {
		XX_SIMPLE_STRUCT_DEFAULT_CODES(File);
		// ǰ�治�� / �����·��
		std::string path;
		// �ļ�����
		uint64_t len = 0;
		// ���ݵ� md5
		std::string md5;

		// ����������������ʱ��¼�Ƿ�ͨ���� len + md5 check. ��д�� json �ļ�
		bool ok = false;
	};
}
// ���ﲻӳ�� ok
AJSON(FilesIndexJson::File, path, len, md5);

namespace FilesIndexJson {
	struct Files {
		XX_SIMPLE_STRUCT_DEFAULT_CODES(Files);
		// �������ص�ַ������� �� /
		std::string baseUrl;
		// �ļ��б�
		std::vector<File> files;
	};
}
AJSON(FilesIndexJson::Files, baseUrl, files);
