#pragma once
# ifdef  __cplusplus
extern "C" {
# endif
#include "md5.h"
# ifdef  __cplusplus
}
# endif
#include <xx_data.h>
#include <string>

// 随便抄一个. 不支持 > 2g 的文件( 除非改进 Update 部分 )
std::string GetDataMD5Hash(const xx::Data& data) {
	if (!data.len || data.len > std::numeric_limits<long>::max()) return {};

	MD5_CTX state;
	uint8_t digest[16];
	char hexOutput[(16 << 1) + 1] = { 0 };

	MD5_Init(&state);
	MD5_Update(&state, data.buf, (int)data.len);
	MD5_Final(digest, &state);

	for (int di = 0; di < 16; ++di) {
		sprintf_s(hexOutput + di * 2, 3, "%02x", digest[di]);
	}

	return hexOutput;
}
