// This parses the RC file.

#ifdef __MINGW32__
#undef __STRICT_ANSI__
#endif

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <assert.h>
#include <strings.h>

#include "rc.h"
#include "ckvp.h"
#include "pd-defs.h"
#include "md-phil.h"

// CTV names
const char *ctv_names[NUM_CTV] = { "off", "blur", "scanline", "interlace" };

// Scaling algorithms names
const char *scaling_names[NUM_SCALING] = { "default", "hqx" };

// CPU names, keep index in sync with rc-vars.h and enums in md.h
static const char *emu_z80_names[] = { "none", "mz80", "cz80", NULL };
static const char *emu_m68k_names[] = { "none", "star", "musa", NULL };

// The table of strings and the keysyms they map to.
// The order is a bit weird, since this was originally a mapping for the SVGALib
// scancodes, and I just added the SDL stuff on top of it.
struct rc_keysym {
  const char *name;
  long keysym;
} keysyms[] = {
  { "ESCAPE", PDK_ESCAPE },
  { "1", PDK_1 },
  { "2", PDK_2 },
  { "3", PDK_3 },
  { "4", PDK_4 },
  { "5", PDK_5 },
  { "6", PDK_6 },
  { "7", PDK_7 },
  { "8", PDK_8 },
  { "9", PDK_9 },
  { "0", PDK_0 },
  { "-", PDK_MINUS },
  { "=", PDK_EQUALS },
  { "+", PDK_EQUALS },
  { "BACKSPACE", PDK_BACKSPACE },
  { "TAB", PDK_TAB },
  { "Q", PDK_q },
  { "W", PDK_w },
  { "E", PDK_e },
  { "R", PDK_r },
  { "T", PDK_t },
  { "Y", PDK_y },
  { "U", PDK_u },
  { "I", PDK_i },
  { "O", PDK_o },
  { "P", PDK_p },
  { "[", PDK_LEFTBRACKET },
  { "{", PDK_LEFTBRACKET },
  { "]", PDK_RIGHTBRACKET },
  { "}", PDK_RIGHTBRACKET },
  { "RETURN", PDK_RETURN },
  { "ENTER", PDK_RETURN },
  { "A", PDK_a },
  { "S", PDK_s },
  { "D", PDK_d },
  { "F", PDK_f },
  { "G", PDK_g },
  { "H", PDK_h },
  { "J", PDK_j },
  { "K", PDK_k },
  { "L", PDK_l },
  { ";", PDK_SEMICOLON },
  { ":", PDK_SEMICOLON },
  { "'", PDK_QUOTE },
  { "\"", PDK_QUOTE },
  { "`", PDK_BACKQUOTE },
  { "~", PDK_BACKQUOTE },
  { "\\", PDK_BACKSLASH },
  { "|", PDK_BACKSLASH },
  { "Z", PDK_z },
  { "X", PDK_x },
  { "C", PDK_c },
  { "V", PDK_v },
  { "B", PDK_b },
  { "N", PDK_n },
  { "M", PDK_m },
  { ",", PDK_COMMA },
  { "<", PDK_COMMA },
  { ".", PDK_PERIOD },
  { ">", PDK_PERIOD },
  { "/", PDK_SLASH },
  { "?", PDK_SLASH },
  { "KP_MULTIPLY", PDK_KP_MULTIPLY },
  { "SPACE", PDK_SPACE },
  { "F1", PDK_F1 },
  { "F2", PDK_F2 },
  { "F3", PDK_F3 },
  { "F4", PDK_F4 },
  { "F5", PDK_F5 },
  { "F6", PDK_F6 },
  { "F7", PDK_F7 },
  { "F8", PDK_F8 },
  { "F9", PDK_F9 },
  { "F10", PDK_F10 },
  { "KP_7", PDK_KP7 },
  { "KP_HOME", PDK_KP7 },
  { "KP_8", PDK_KP8 },
  { "KP_UP", PDK_KP8 },
  { "KP_9", PDK_KP9 },
  { "KP_PAGE_UP", PDK_KP9 },
  { "KP_PAGEUP", PDK_KP9 },
  { "KP_MINUS", PDK_KP_MINUS },
  { "KP_4", PDK_KP4 },
  { "KP_LEFT", PDK_KP4 },
  { "KP_5", PDK_KP5 },
  { "KP_6", PDK_KP6 },
  { "KP_RIGHT", PDK_KP6 },
  { "KP_PLUS", PDK_KP_PLUS },
  { "KP_1", PDK_KP1 },
  { "KP_END", PDK_KP1 },
  { "KP_2", PDK_KP2 },
  { "KP_DOWN", PDK_KP2 },
  { "KP_3", PDK_KP3 },
  { "KP_PAGE_DOWN", PDK_KP3 },
  { "KP_PAGEDOWN", PDK_KP3 },
  { "KP_0", PDK_KP0 },
  { "KP_INSERT", PDK_KP0 },
  { "KP_PERIOD", PDK_KP_PERIOD },
  { "KP_DELETE", PDK_KP_PERIOD },
  { "F11", PDK_F11 },
  { "F12", PDK_F12 },
  { "KP_ENTER", PDK_KP_ENTER },
  { "KP_DIVIDE", PDK_KP_DIVIDE },
  { "HOME", PDK_HOME },
  { "UP", PDK_UP },
  { "PAGE_UP", PDK_PAGEUP },
  { "PAGEUP", PDK_PAGEUP },
  { "LEFT", PDK_LEFT },
  { "RIGHT", PDK_RIGHT },
  { "END", PDK_END },
  { "DOWN", PDK_DOWN },
  { "PAGE_DOWN", PDK_PAGEDOWN },
  { "PAGEDOWN", PDK_PAGEDOWN },
  { "INSERT", PDK_INSERT },
  { "DELETE", PDK_DELETE },
  { "NUMLOCK", PDK_NUMLOCK },
  { "NUM_LOCK", PDK_NUMLOCK },
  { "CAPSLOCK", PDK_CAPSLOCK },
  { "CAPS_LOCK", PDK_CAPSLOCK },
  { "SCROLLOCK", PDK_SCROLLOCK },
  { "SCROLL_LOCK", PDK_SCROLLOCK },
  { "LSHIFT", PDK_LSHIFT },
  { "SHIFT_L", PDK_LSHIFT },
  { "RSHIFT", PDK_RSHIFT },
  { "SHIFT_R", PDK_RSHIFT },
  { "LCTRL", PDK_LCTRL },
  { "CTRL_L", PDK_LCTRL },
  { "RCTRL", PDK_RCTRL },
  { "CTRL_R", PDK_RCTRL },
  { "LALT", PDK_LALT },
  { "ALT_L", PDK_LALT },
  { "RALT", PDK_RALT },
  { "ALT_R", PDK_RALT },
  { "LMETA", PDK_LMETA },
  { "META_L", PDK_LMETA },
  { "RMETA", PDK_RMETA },
  { "META_R", PDK_RMETA },
  { NULL, 0 } // Terminator
}; // Phew! ;)

/* Define all the external RC variables */
#include "rc-vars.h"

long js_map_button[2][16] = {
    {
	MD_A_MASK, MD_C_MASK, MD_A_MASK,
	MD_B_MASK, MD_Y_MASK, MD_Z_MASK,
	MD_X_MASK, MD_X_MASK, MD_START_MASK,
	MD_MODE_MASK, 0, 0, 0, 0, 0, 0
    },
    {
	MD_A_MASK, MD_C_MASK, MD_A_MASK,
	MD_B_MASK, MD_Y_MASK, MD_Z_MASK,
	MD_X_MASK, MD_X_MASK, MD_START_MASK,
	MD_MODE_MASK, 0, 0, 0, 0, 0, 0
    }
};

/* Parse a keysym.
 * If the string matches one of the strings in the keysym table above,
 * return the keysym, otherwise -1. */
static long keysym(const char *code)
{
  struct rc_keysym *s = keysyms;
  long r = 0;

  // Check for modifier prefixes shift-, ctrl-, alt-, meta-
  for(;;) {
    if(!strncasecmp("shift-", code, 6)) {
      r |= KEYSYM_MOD_SHIFT;
      code += 6;
      continue;
    }
    if(!strncasecmp("ctrl-", code, 5)) {
      r |= KEYSYM_MOD_CTRL;
      code += 5;
      continue;
    }
    if(!strncasecmp("alt-", code, 4)) {
      r |= KEYSYM_MOD_ALT;
      code += 4;
      continue;
    }
    if(!strncasecmp("meta-", code, 5)) {
      r |= KEYSYM_MOD_META;
      code += 5;
      continue;
    }
    break;
  }

  do {
    if(!strcasecmp(s->name, code)) return r |= s->keysym;
  } while ((++s)->name);
  /* No match */
  return -1;
}

/* Parse a boolean value.
 * If the string is "yes" or "true", return 1.
 * If the string is "no" or "false", return 0.
 * Otherwise, just return atoi(value). */
static long boolean(const char *value)
{
  if(!strcasecmp(value, "yes") || !strcasecmp(value, "true"))
    return 1;
  if(!strcasecmp(value, "no") || !strcasecmp(value, "false"))
    return 0;
  return atoi(value);
}

// Made GCC happy about unused things when we don't want a joystick.  :) [PKH]
// Cheesy hack to set joystick mappings from the RC file. [PKH]
static long jsmap(const char *value) {
  if(!strcasecmp(value, "mode"))
    snprintf((char*)value, 2, "%c", 'm');
  if(!strcasecmp(value, "start"))
    snprintf((char*)value, 2, "%c", 's');
  switch(*value) {
    case 'A':
    case 'a':
    return(MD_A_MASK);
    break;
    case 'B':
    case 'b':
    return(MD_B_MASK);
    break;
    case 'C':
    case 'c':
    return(MD_C_MASK);
    break;
    case 'X':
    case 'x':
    return(MD_X_MASK);
    break;
    case 'Y':
    case 'y':
    return(MD_Y_MASK);
    break;
    case 'Z':
    case 'z':
    return(MD_Z_MASK);
    break;
    case 'M':
    case 'm':
    return(MD_MODE_MASK);
    break;
    case 'S':
    case 's':
    return(MD_START_MASK);
    break;
    default:
    return(0);
    }
}

/* Parse the CTV type. As new CTV filters get submitted expect this to grow ;)
 * Current values are:
 *  off      - No CTV
 *  blur     - blur bitmap (from DirectX DGen), by Dave <dave@dtmnt.com>
 *  scanline - attenuates every other line, looks cool! by Phillip K. Hornung <redx@pknet.com>
 */
static long ctv(const char *value)
{
  for(int i = 0; i < NUM_CTV; ++i)
    if(!strcasecmp(value, ctv_names[i])) return i;
  return -1;
}

static long scaling(const char *value)
{
	int i;

	for (i = 0; (i < NUM_SCALING); ++i)
		if (!strcasecmp(value, scaling_names[i]))
			return i;
	return -1;
}

/* Parse CPU types */
static long emu_z80(const char *value)
{
	unsigned int i;

	for (i = 0; (emu_z80_names[i] != NULL); ++i)
		if (!strcasecmp(value, emu_z80_names[i]))
			return i;
	return -1;
}

static long emu_m68k(const char *value)
{
	unsigned int i;

	for (i = 0; (emu_m68k_names[i] != NULL); ++i)
		if (!strcasecmp(value, emu_m68k_names[i]))
			return i;
	return -1;
}

static long number(const char *value)
{
  return atoi(value);
}

/* This is a table of all the RC options, the variables they affect, and the
 * functions to parse their values. */
struct rc_field {
  const char *fieldname;
  long (*parser)(const char*);
  long *variable;
} rc_fields[] = {
  { "key_pad1_up", keysym, &pad1_up },
  { "key_pad1_down", keysym, &pad1_down },
  { "key_pad1_left", keysym, &pad1_left },
  { "key_pad1_right", keysym, &pad1_right },
  { "key_pad1_a", keysym, &pad1_a },
  { "key_pad1_b", keysym, &pad1_b },
  { "key_pad1_c", keysym, &pad1_c },
  { "key_pad1_x", keysym, &pad1_x },
  { "key_pad1_y", keysym, &pad1_y },
  { "key_pad1_z", keysym, &pad1_z },
  { "key_pad1_mode", keysym, &pad1_mode },
  { "key_pad1_start", keysym, &pad1_start },
  { "key_pad2_up", keysym, &pad2_up },
  { "key_pad2_down", keysym, &pad2_down },
  { "key_pad2_left", keysym, &pad2_left },
  { "key_pad2_right", keysym, &pad2_right },
  { "key_pad2_a", keysym, &pad2_a },
  { "key_pad2_b", keysym, &pad2_b },
  { "key_pad2_c", keysym, &pad2_c },
  { "key_pad2_x", keysym, &pad2_x },
  { "key_pad2_y", keysym, &pad2_y },
  { "key_pad2_z", keysym, &pad2_z },
  { "key_pad2_mode", keysym, &pad2_mode },
  { "key_pad2_start", keysym, &pad2_start },
  { "key_fix_checksum", keysym, &dgen_fix_checksum },
  { "key_quit", keysym, &dgen_quit },
  { "key_craptv_toggle", keysym, &dgen_craptv_toggle },
  { "key_scaling_toggle", keysym, &dgen_scaling_toggle },
  { "key_screenshot", keysym, &dgen_screenshot },
  { "key_reset", keysym, &dgen_reset },
  { "key_slot_0", keysym, &dgen_slot_0 },
  { "key_slot_1", keysym, &dgen_slot_1 },
  { "key_slot_2", keysym, &dgen_slot_2 },
  { "key_slot_3", keysym, &dgen_slot_3 },
  { "key_slot_4", keysym, &dgen_slot_4 },
  { "key_slot_5", keysym, &dgen_slot_5 },
  { "key_slot_6", keysym, &dgen_slot_6 },
  { "key_slot_7", keysym, &dgen_slot_7 },
  { "key_slot_8", keysym, &dgen_slot_8 },
  { "key_slot_9", keysym, &dgen_slot_9 },
  { "key_save", keysym, &dgen_save },
  { "key_load", keysym, &dgen_load },
  { "key_z80_toggle", keysym, &dgen_z80_toggle },
  { "key_cpu_toggle", keysym, &dgen_cpu_toggle },
  { "key_stop", keysym, &dgen_stop },
  { "key_game_genie", keysym, &dgen_game_genie },
  { "key_fullscreen_toggle", keysym, &dgen_fullscreen_toggle },
  { "bool_autoload", boolean, &dgen_autoload },
  { "bool_autosave", boolean, &dgen_autosave },
  { "bool_frameskip", boolean, &dgen_frameskip },
  { "bool_show_carthead", boolean, &dgen_show_carthead },
  { "ctv_craptv_startup", ctv, &dgen_craptv },
  { "scaling_startup", scaling, &dgen_scaling },
  { "emu_z80_startup", emu_z80, &dgen_emu_z80 },
  { "emu_m68k_startup", emu_m68k, &dgen_emu_m68k },
  { "bool_sound", boolean, &dgen_sound },
  { "int_soundrate", number, &dgen_soundrate },
  { "int_soundsegs", number, &dgen_soundsegs },
  { "int_soundsamples", number, &dgen_soundsamples },
  { "int_nice", number, &dgen_nice },
  { "bool_fullscreen", boolean, &dgen_fullscreen },
  { "int_info_height", number, &dgen_info_height },
  { "int_scale", number, &dgen_scale },
  { "int_depth", number, &dgen_depth },
  { "bool_opengl", boolean, &dgen_opengl },
  { "bool_opengl_aspect", boolean, &dgen_opengl_aspect },
  { "int_opengl_width", number, &dgen_opengl_width },
  { "int_opengl_height", number, &dgen_opengl_height },
  { "bool_opengl_linear", boolean, &dgen_opengl_linear },
  { "bool_opengl_32bit", boolean, &dgen_opengl_32bit },
  { "bool_opengl_swap", boolean, &dgen_opengl_swap },
  { "bool_opengl_square", boolean, &dgen_opengl_square },
  { "bool_joystick", boolean, &dgen_joystick },
  { "int_joystick1_dev", number, &dgen_joystick1_dev },
  { "int_joystick2_dev", number, &dgen_joystick2_dev },
  { "joypad1_b0", jsmap, &js_map_button[0][0] },
  { "joypad1_b1", jsmap, &js_map_button[0][1] },
  { "joypad1_b2", jsmap, &js_map_button[0][2] },
  { "joypad1_b3", jsmap, &js_map_button[0][3] },
  { "joypad1_b4", jsmap, &js_map_button[0][4] },
  { "joypad1_b5", jsmap, &js_map_button[0][5] },
  { "joypad1_b6", jsmap, &js_map_button[0][6] },
  { "joypad1_b7", jsmap, &js_map_button[0][7] },
  { "joypad1_b8", jsmap, &js_map_button[0][8] },
  { "joypad1_b9", jsmap, &js_map_button[0][9] },
  { "joypad1_b10", jsmap, &js_map_button[0][10] },
  { "joypad1_b11", jsmap, &js_map_button[0][11] },
  { "joypad1_b12", jsmap, &js_map_button[0][12] },
  { "joypad1_b13", jsmap, &js_map_button[0][13] },
  { "joypad1_b14", jsmap, &js_map_button[0][14] },
  { "joypad1_b15", jsmap, &js_map_button[0][15] },
  { "joypad2_b0", jsmap, &js_map_button[1][0] },
  { "joypad2_b1", jsmap, &js_map_button[1][1] },
  { "joypad2_b2", jsmap, &js_map_button[1][2] },
  { "joypad2_b3", jsmap, &js_map_button[1][3] },
  { "joypad2_b4", jsmap, &js_map_button[1][4] },
  { "joypad2_b5", jsmap, &js_map_button[1][5] },
  { "joypad2_b6", jsmap, &js_map_button[1][6] },
  { "joypad2_b7", jsmap, &js_map_button[1][7] },
  { "joypad2_b8", jsmap, &js_map_button[1][8] },
  { "joypad2_b9", jsmap, &js_map_button[1][9] },
  { "joypad2_b10", jsmap, &js_map_button[1][10] },
  { "joypad2_b11", jsmap, &js_map_button[1][11] },
  { "joypad2_b12", jsmap, &js_map_button[1][12] },
  { "joypad2_b13", jsmap, &js_map_button[1][13] },
  { "joypad2_b14", jsmap, &js_map_button[1][14] },
  { "joypad2_b15", jsmap, &js_map_button[1][15] },
  { NULL, NULL, NULL } // Terminator
};

/* Replace unprintable characters */
static char *strclean(char *s)
{
	size_t i;

	for (i = 0; (s[i] != '\0'); ++i)
		if (!isprint(s[i]))
			s[i] = '?';
	return s;
}

/* Parse the rc file */
void parse_rc(FILE *file, const char *name)
{
	struct rc_field *rc_field = NULL;
	long potential;
	int overflow = 0;
	size_t len;
	size_t parse;
	ckvp_t ckvp = CKVP_INIT;
	char buf[1024];

	if ((file == NULL) || (name == NULL))
		return;
read:
	len = fread(buf, 1, sizeof(buf), file);
	/* Check for read errors first */
	if ((len == 0) && (ferror(file))) {
		fprintf(stderr, "rc: %s: %s\n", name, strerror(errno));
		return;
	}
	/* The goal is to make an extra pass with len == 0 when feof(file) */
	parse = 0;
parse:
	parse += ckvp_parse(&ckvp, (len - parse), &(buf[parse]));
	switch (ckvp.state) {
	case CKVP_NONE:
		/* Nothing to do */
		break;
	case CKVP_OUT_FULL:
		/*
		  Buffer is full, field is probably too large. We don't want
		  to report it more than once, so just store a flag for now.
		*/
		overflow = 1;
		break;
	case CKVP_OUT_KEY:
		/* Got a key */
		if (overflow) {
			fprintf(stderr, "rc: %s:%u:%u: key field too large\n",
				name, ckvp.line, ckvp.column);
			rc_field = NULL;
			overflow = 0;
			break;
		}
		/* Find the related rc_field in rc_fields */
		assert(ckvp.out_size < sizeof(ckvp.out));
		ckvp.out[(ckvp.out_size)] = '\0';
		for (rc_field = rc_fields; (rc_field->fieldname != NULL);
		     ++rc_field)
			if (!strcasecmp(rc_field->fieldname, ckvp.out))
				goto key_over;
		fprintf(stderr, "rc: %s:%u:%u: unknown key `%s'\n",
			name, ckvp.line, ckvp.column, strclean(ckvp.out));
	key_over:
		break;
	case CKVP_OUT_VALUE:
		/* Got a value */
		if (overflow) {
			fprintf(stderr, "rc: %s:%u:%u: value field too large\n",
				name, ckvp.line, ckvp.column);
			overflow = 0;
			break;
		}
		assert(ckvp.out_size < sizeof(ckvp.out));
		ckvp.out[(ckvp.out_size)] = '\0';
		if ((rc_field == NULL) || (rc_field->fieldname == NULL))
			break;
		potential = rc_field->parser(ckvp.out);
		/* If we got a bad value, discard and warn user */
		if ((rc_field->parser != number) && (potential == -1))
			fprintf(stderr,
				"rc: %s:%u:%u: invalid value for key"
				" `%s': `%s'\n",
				name, ckvp.line, ckvp.column,
				rc_field->fieldname, strclean(ckvp.out));
		else
			*(rc_field->variable) = potential;
		break;
	case CKVP_ERROR:
	default:
		fprintf(stderr, "rc: %s:%u:%u: syntax error, aborting\n",
			name, ckvp.line, ckvp.column);
		return;
	}
	/* Not done with the current buffer? */
	if (parse != len)
		goto parse;
	/* If len != 0, try to read once again */
	if (len != 0)
		goto read;
}
