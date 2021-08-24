#pragma once
#include "ajson.hpp"

namespace FilesIndexJson {
	struct File {
		std::string path;
		uint64_t len = 0;
		std::string md5;
	};
}
AJSON(FilesIndexJson::File, path, len, md5);

namespace FilesIndexJson {
	struct Files {
		std::string baseUrl;
		std::vector<File> files;

		// ���Բ��� prefix ��ͷ���׸����ֵ� File
		File const* FirstFile_PathStartWith(std::string_view prefix) {
			for (auto const& f : files) {
				if (f.path.starts_with(prefix)) return &f;
			}
			return nullptr;
		}
	};
}
AJSON(FilesIndexJson::Files, baseUrl, files);
