// DGen/SDL v1.23+
#ifndef RC_H_
#define RC_H_

#include <stdio.h>

// Define the different craptv types
#define NUM_CTV 4 // Include CTV_OFF
#define NUM_SCALING 2
#define CTV_OFF       0
#define CTV_BLUR      1
#define CTV_SCANLINE  2
#define CTV_INTERLACE 3

// Define OR masks for key modifiers
#define KEYSYM_MOD_ALT		0x40000000
#define KEYSYM_MOD_SHIFT	0x20000000
#define KEYSYM_MOD_CTRL		0x10000000
#define KEYSYM_MOD_META		0x08000000

// All the CTV engine names, in string form for the RC and message bar
extern const char *ctv_names[];

// Scaling algorithms names
extern const char *scaling_names[];

// Provide a prototype to the parse_rc function in rc.cpp
extern void parse_rc(FILE *file, const char *name);

#endif // RC_H_
