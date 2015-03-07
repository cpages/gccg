#ifndef COMPAT_H
#define COMPAT_H

#include <stdio.h>

struct dirent
{
  char d_name[FILENAME_MAX+1];
};

typedef struct DIR DIR;

DIR * opendir ( const char * dirname );
struct dirent * readdir ( DIR * dir );
int closedir ( DIR * dir );
void rewinddir ( DIR * dir );

// following POSIX function are deprecated, using ISO C++ conformant names
#undef unlink
#define unlink _unlink

void win32_display_error(const char* caption, const char* message);

unsigned int win32_get_clipboard_text(char* buffer, unsigned int buffen_len);

#endif // COMPAT_H

