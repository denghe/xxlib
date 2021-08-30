#pragma once
#include "xx_helpers.h"
#include "ajson.hpp"
#include <filesystem>

namespace FilesIndexJson {
	struct File {
		XX_SIMPLE_STRUCT_DEFAULT_CODES(File);
		// 前面不带 / 的相对路径
		std::string path;
		// 文件长度
		uint64_t len = 0;
		// 内容的 md5
		std::string md5;

		// 辅助变量，用于临时记录是否通过了 len + md5 check. 不写入 json 文件
		bool ok = false;
	};
}
// 这里不映射 ok
AJSON(FilesIndexJson::File, path, len, md5);

namespace FilesIndexJson {
	struct Files {
		XX_SIMPLE_STRUCT_DEFAULT_CODES(Files);
		// 基础下载地址，最后面 带 /
		std::string baseUrl;
		// 文件列表
		std::vector<File> files;
	};
}
AJSON(FilesIndexJson::Files, baseUrl, files);
