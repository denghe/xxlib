#pragma once
#include "xx_helpers.h"
#include "ajson.hpp"
#include <filesystem>

namespace FilesIndexJson {
	struct File {
		XX_SIMPLE_STRUCT_DEFAULT_CODES(File);
		std::string path;
		uint64_t len = 0;
		std::string md5;
		bool ok = false;	// 辅助变量，不计入 json 文件
	};
}
AJSON(FilesIndexJson::File, path, len, md5);

namespace FilesIndexJson {
	struct Files {
		XX_SIMPLE_STRUCT_DEFAULT_CODES(Files);
		std::string baseUrl;
		std::vector<File> files;
	};
}
AJSON(FilesIndexJson::Files, baseUrl, files);
