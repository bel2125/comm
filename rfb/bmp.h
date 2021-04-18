#ifndef BMP_HEADER_INCLUDED
#define BMP_HEADER_INCLUDED

#include <stdint.h>
extern int writebmp(const char *filename,
                    uint32_t width,
                    uint32_t height,
                    uint32_t *pixels);

#endif
