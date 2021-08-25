#pragma once
#include "ajson.hpp"
#include "xx_helpers.h"
#include <filesystem>

namespace FilesIndexJson {
	struct File {
		XX_SIMPLE_STRUCT_DEFAULT_CODES(File);
		std::string path;
		uint64_t len = 0;
		std::string md5;
	};
}
AJSON(FilesIndexJson::File, path, len, md5);

namespace FilesIndexJson {
	struct Files {
		XX_SIMPLE_STRUCT_DEFAULT_CODES(Files);
		std::string baseUrl;
		std::vector<File> files;

		// 用以查找 prefix 打头的首个出现的 File
		File const* FirstFile_PathStartWith(std::string_view prefix) {
			for (auto const& f : files) {
				if (f.path.starts_with(prefix)) return &f;
			}
			return nullptr;
		}

		static Files LoadFromFile(std::string const& path);
	};
}
AJSON(FilesIndexJson::Files, baseUrl, files);

namespace FilesIndexJson {
	Files Files::LoadFromFile(std::string const& path) {
		Files rtv;
		ajson::load_from_file(rtv, path.c_str());
		return rtv;
	}
}
