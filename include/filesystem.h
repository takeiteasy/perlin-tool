//
//  filesystem.h
//  sokol
//
//  Created by George Watson on 23/02/2023.
//

#ifndef filesystem_h
#define filesystem_h
#include "platform.h"
#include "vector.h"
#include <stdlib.h>
#include <string.h>
#if defined(PLATFORM_WINDOWS)
#include <io.h>
#define F_OK 0
#define access _access
#define PATH_SEPERATOR "\\"
#else
#include <dirent.h>
#include <unistd.h>
#define PATH_SEPERATOR "/"
#endif

const char* FileExt(const char *path);
const char** FindFiles(const char *ext);

#endif /* filesystem_h */