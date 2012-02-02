#ifndef __FONT_H__
#define __FONT_H__

#include <stddef.h>
#include <stdint.h>

extern size_t font_text(uint8_t *buf, unsigned int width, unsigned int height,
			unsigned int bytes_per_pixel, unsigned int pitch,
			const char *msg, size_t len);

#endif /* __FONT_H__ */
