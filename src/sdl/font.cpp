// DGen/SDL v1.29
// by Joe Groff
// How's my programming? E-mail <joe@pknet.com>

/* DGen's font renderer.
 * I hope it's pretty well detached from the DGen core, so you can use it in
 * any other SDL app you like. */

#include <stddef.h>
#include <stdio.h>
#include "font.h"	/* The interface functions */

extern const short *dgen_font_8x13[0x80];
extern const short *dgen_font_16x26[0x80];
extern const short *dgen_font_7x5[0x80];

const struct dgen_font {
	unsigned int w;
	unsigned int h;
	const short *(*data)[0x80];
} dgen_font[] = {
	{ 16, 26, &dgen_font_16x26 },
	{ 8, 13, &dgen_font_8x13 },
	{ 7, 5, &dgen_font_7x5 },
	{ 0, 0, NULL }
};

size_t font_text(uint8_t *buf, unsigned int width, unsigned int height,
		 unsigned int bytes_per_pixel, unsigned int pitch,
		 const char *msg, size_t len)
{
	const struct dgen_font *font = dgen_font;
	uint8_t *p_max;
	size_t r;
	unsigned int x;

	if (len == 0)
		return 0;
	while ((font->data != NULL) && (font->h > height)) {
		++font;
		continue;
	}
	if (font->data == NULL) {
		printf("info: %.*s\n", (unsigned int)len, msg);
		return len;
	}
	p_max = (buf + (pitch * height));
	for (x = 0, r = 0;
	     ((msg[r] != '\0') && (r != len) && ((x + font->w) <= width));
	     ++r, x += font->w) {
		const short *glyph = (*font->data)[(msg[r] & 0x7f)];
		uint8_t *p = (buf + (x * bytes_per_pixel));
		unsigned int n = 0;
		short g;

		if (glyph == NULL)
			continue;
		while ((g = *glyph) != -1) {
			unsigned int i;

			p += (((n += g) / font->w) * pitch);
			n %= font->w;
			for (i = 0; (i < bytes_per_pixel); ++i) {
				uint8_t *tmp = &p[((n * bytes_per_pixel) + i)];

				if (tmp < p_max)
					*tmp = 0xff;
			}
			++glyph;
		}
	}
	return r;
}
