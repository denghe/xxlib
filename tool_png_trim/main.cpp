/*

功能：扫描当前工作目录下 *.png, 并保持中心点不变，进行 上下，左右 对称 trim

*/
#define _CRT_SECURE_NO_WARNINGS
#include <filesystem>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

int main() {
	// 遍历当前工作目录下 *.png
	auto&& cp = std::filesystem::current_path();
	std::cout << "working dir: " << cp.string() << std::endl;

	for (auto&& entry : std::filesystem::recursive_directory_iterator(cp)) {
		if (!entry.is_regular_file()) continue;
		auto&& p = entry.path();
		if (p.extension() != ".png") continue;

		// 加载
		int w, h, comp;
		auto d = stbi_load(p.string().c_str(), &w, &h, &comp, 0);

		// 因为是 上下，左右 对称 trim, 所以只需要存储 左上角裁切范围值即可
		int tx = 0, ty = 0;

		// 边界查找: 左右
		for (int x = 0; x < w / 2; ++x) {
			for (int y = 0; y < h; ++y) {
				auto a1 = d[(y * w + x) * 4 + 3];
				auto a2 = d[(y * w + (w - x - 1)) * 4 + 3];
				if (a1 || a2) {
					tx = x;
					goto LabStep1;
				}
			}
		}

	LabStep1:
		// 边界查找: 上下
		for (int y = 0; y < h / 2; ++y) {
			for (int x = tx; x < w - tx; ++x) {
				auto a1 = d[(y * w + x) * 4 + 3];
				auto a2 = d[((h - y - 1) * w + x) * 4 + 3];
				if (a1 || a2) {
					ty = y;
					goto LabStep2;
				}
			}
		}

	LabStep2:
		// 存盘, 根据 tx ty 算范围 和 strip 值. 如果 tx, ty 为 0 就不做任何更改了
		if (tx || ty) {
			stbi_write_png(p.string().c_str(), w - tx * 2, h - ty * 2, comp, d + (ty * w + tx) * 4, w * 4);
			std::cout << "cuted file: " << p.filename() << std::endl;
		}

		// 卸载
		stbi_image_free(d);
	}

	std::cout << "finished! press any key to continue..." << std::endl;
	std::cin.get();

	return 0;
}
