/* Utility functions definitions */

#ifndef SYSTEM_H_
#define SYSTEM_H_

#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
#define SYSTEM_H_BEGIN_ extern "C" {
#define SYSTEM_H_END_ }
#else
#define SYSTEM_H_BEGIN_
#define SYSTEM_H_END_
#endif

SYSTEM_H_BEGIN_

#ifndef __MINGW32__
#define DGEN_BASEDIR "."
#define DGEN_RC "dgen.cfg"
#define DGEN_DIRSEP "/"
#define DGEN_DIRSEP_ALT ""
#else
#define DGEN_BASEDIR "/accounts/1000/shared/misc/dgen"
#define DGEN_RC "dgen.cfg"
#define DGEN_DIRSEP "\\"
#define DGEN_DIRSEP_ALT "/"
#endif

#define DGEN_READ 0x1
#define DGEN_WRITE 0x2
#define DGEN_APPEND 0x4
#define DGEN_CURRENT 0x8

#define dgen_fopen_rc(mode) dgen_fopen(NULL, DGEN_RC, (mode))
extern FILE *dgen_fopen(const char *subdir, const char *file,
			unsigned int mode);

extern char *dgen_basename(char *path);

#define le2h16(v) h2le16(v)
static inline uint16_t h2le16(uint16_t v)
{
#ifdef WORDS_BIGENDIAN
	return ((v >> 8) | (v << 8));
#else
	return v;
#endif
}

#define be2h16(v) h2be16(v)
static inline uint16_t h2be16(uint16_t v)
{
#ifdef WORDS_BIGENDIAN
	return v;
#else
	return ((v >> 8) | (v << 8));
#endif
}

#define le2h32(v) h2le32(v)
static inline uint32_t h2le32(uint32_t v)
{
#ifdef WORDS_BIGENDIAN
	return (((v & 0xff000000) >> 24) | ((v & 0x00ff0000) >>  8) |
		((v & 0x0000ff00) <<  8) | ((v & 0x000000ff) << 24));
#else
	return v;
#endif
}

#define be2h32(v) h2be32(v)
static inline uint32_t h2be32(uint32_t v)
{
#ifdef WORDS_BIGENDIAN
	return v;
#else
	return (((v & 0xff000000) >> 24) | ((v & 0x00ff0000) >>  8) |
		((v & 0x0000ff00) <<  8) | ((v & 0x000000ff) << 24));
#endif
}

typedef uint8_t uint24_t[3];

static inline uint24_t *u24cpy(uint24_t *dst, const uint24_t *src)
{
	/* memcpy() is sometimes faster. */
#ifdef U24CPY_MEMCPY
	memcpy(*dst, *src, sizeof(*dst));
#else
	(*dst)[0] = (*src)[0];
	(*dst)[1] = (*src)[1];
	(*dst)[2] = (*src)[2];
#endif
	return dst;
}

extern uint8_t *load(size_t *file_size, FILE *file, size_t max_size);
extern void unload(uint8_t *data);

SYSTEM_H_END_

#endif /* SYSTEM_H_ */
