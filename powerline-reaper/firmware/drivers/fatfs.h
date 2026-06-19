/*
 * fatfs.h — FatFs glue interface (simplified types to avoid pulling in ff.h)
 *
 * Author: jayis1
 * License: GPL-2.0
 */

#ifndef FATFS_H
#define FATFS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct { uint8_t opaque[64]; } FATFS_FYTE;
typedef struct { uint8_t opaque[64]; } FIL;
typedef uint32_t UINT;

typedef BYTE BYTE_t;
typedef BYTE BYTE;
typedef BYTE DWORD_t;
typedef uint32_t DWORD;
typedef BYTE DSTATUS;
typedef enum { RES_OK=0, RES_ERROR, RES_WRPRT, RES_NOTRDY, RES_PARERR } DRESULT;

#define FA_READ       0x01
#define FA_WRITE      0x02
#define FA_CREATE_ALWAYS 0x08
#define FA_OPEN_ALWAYS    0x10

typedef FATFS_FYTE FYTEFS;

int fatfs_mount(FYTEFS *fs);
int fatfs_open(FIL *fp, const char *path, uint8_t mode);
int fatfs_write(FIL *fp, const void *buf, uint32_t len, UINT *bw);
int fatfs_close(FIL *fp);
int fatfs_sync(FIL *fp);
int fatfs_format(FYTEFS *fs);

#ifdef __cplusplus
}
#endif

#endif /* FATFS_H */