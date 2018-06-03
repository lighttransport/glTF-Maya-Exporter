#pragma once
#ifndef _KIL_COPY_TEXTURE_FILE_H_
#define _KIL_COPY_TEXTURE_FILE_H_

#include <string>

namespace kil
{
	bool CopyTextureFile(const std::string& src_path, const std::string& dst_path);
}

#endif
