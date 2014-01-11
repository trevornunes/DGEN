// DGen/SDL 1.17
// by Joe Groff <joe@pknet.com>
// Read LICENSE for copyright etc., but if you've seen one BSDish license,
// you've seen them all ;)

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <bps/dialog.h>
#include <bps/bps.h>
#include <bps/event.h>
#include "utils/xstring.h"
#include <pthread.h>
#include <dirent.h>

#define IS_MAIN_CPP
#include "system.h"
#include "md.h"
#include "pd.h"
#include "pd-defs.h"
#include "rc.h"
#include "rc-vars.h"

#ifdef __BEOS__
#include <OS.h>
#endif

using namespace std;
#ifdef __QNXNTO__
#include "rompb.h"
#include <sched.h>
char g_runningFile_str[64];

void md_shutdown(md& megad);
int md_startup(md& megad);
void md_load(md& megad);
void md_save(md& megad);
void ram_load(md& megad);
void ram_save(md& megad);

#endif
// Defined in ras.cpp, and set to true if the Genesis palette's changed.
extern int pal_dirty;
int load_new_game_g = 0;  // trigger new ROM load.
int switch_mode_g = 0;    // reset and toggle between NTSC,PAL
char new_game_path_g[128];

int slot = 0;  // see md_save and related
unsigned long oldclk, newclk, startclk, fpsclk; // used in main and playbook_load ...
int start_slot = -1;
int c = 0, pal_mode = 0, running = 1, usec = 0;
unsigned long frames = 0, frames_old = 0, fps = 0;
char *patches = NULL;
char rom[256];
FILE *debug_log = NULL;
unsigned int samples;
unsigned int hz = 60;
//char region = '\0';

char region = ('u' & ~(0x20));

int gameIndex = 0;
char romfilename[256];
// Do a demo frame, if active
enum demo_status {
	DEMO_OFF, DEMO_RECORD, DEMO_PLAY
};

Rompb romp = Rompb(Rompb::rom_smd_c);

void rombrowser_setup(void) {
	(void) mkdir("/accounts/1000/shared/misc/dgen",
			S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

	(void) mkdir("/accounts/1000/shared/misc/roms",
			S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

	(void) mkdir("/accounts/1000/shared/misc/roms/smd",
			S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

	(void) mkdir("data/saves", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

	romp.getRomList();
	if (romp.romCount() == 0) {
		fprintf(stderr,"no roms found on setup.\n");
	}
	if (romp.romCount() == 0) {
		romp.setRomPath("app/native");
		romp.getRomList();
	}
}

const char *rombrowser_next() {
	return romp.getRomNext();
}

const char *rombrowser_get_rom_name() {
	return romp.getActiveRomName().c_str();
}

void rombrowser_update() {
	romp.getRomList();
}

void rombrowser_setpath(const char *newdir) {
	if (newdir) {
		romp.setRomPath(newdir);
	}
}

int rombrowser_get_num_roms() {
	return (romp.romCount());
}

const char *rombrowser_find(const char *srch) {
	fprintf(stderr,"rombrowser_find '%s' returns %s\n", srch, romp.findRom(srch).c_str());

	return romp.findRom(srch).c_str();
}

/******************************
 *
 ******************************/
int md_load_new_rom(md& megad, char *rom) {

	if (!rom)
		return -1;

	printf("md_load_new_rom %s\n", rom);
	fflush(stdout);

	// suspend CPU , save ...
	md_shutdown(megad);
	md_startup(megad);

	// Load the requested ROM
	if (megad.load(rom)) {
		fprintf(stderr, "main: failed to load ROM '%s'\n", rom);
		return -1;
	}
	// Set untouched pads
	megad.pad[0] = megad.pad[1] = 0xF303F;
#ifdef WITH_JOYSTICK
	if (dgen_joystick)
		megad.init_joysticks(dgen_joystick1_dev, dgen_joystick2_dev);
#endif

	/*
	 // Load patches, if given
	 if(patches)
	 {
	 printf("main: Using patch codes %s\n", patches);
	 megad.patch(patches, NULL, NULL, NULL);
	 }
	 */

	// Fix checksum
	megad.fix_rom_checksum();
	// Reset
	megad.reset();

	// Load up save RAM
	ram_load(megad);

	// If autoload is on, load save state 0
	if (dgen_autoload) {
		slot = 0;
		md_load(megad);
	}

	// If -s option was given, load the requested slot
	if (start_slot >= 0) {
		slot = start_slot;
		md_load(megad);
	}

	// Start the timing refs
	startclk = pd_usecs();
	oldclk = startclk;
	fpsclk = startclk;

	//  if(dgen_sound) pd_sound_start();

	if (strcmp("E", megad.cart_head.countries) == 0)
		fprintf(stderr,"cart-region: Europe\n");

	if (strcmp("U", megad.cart_head.countries) == 0)
		fprintf(stderr,"cart-region: USA\n");

	if (strcmp("J", megad.cart_head.countries) == 0)
		fprintf(stderr,"cart-region: Japan\n");

	if (strcmp("F", megad.cart_head.countries) == 0)
		fprintf(stderr,"cart-region: F\n");

	if (strcmp("JU", megad.cart_head.countries) == 0)
		fprintf(stderr,"cart-region: JU\n");

	if (strcmp("UUE", megad.cart_head.countries) == 0)
		fprintf(stderr,"cart-region: UUE\n");

	if (strcmp("UU", megad.cart_head.countries) == 0)
		fprintf(stderr,"cart-region: UU\n");

	if (strcmp("JUE", megad.cart_head.countries) == 0)
		fprintf(stderr,"cart-region: UU\n");

//  pd_show_carthead(megad);
	return 1;
}

int md_startup(md& megad) {
	// Start up the sound chips.
	if (YM2612Init(1, (((pal_mode) ? PAL_MCLK : NTSC_MCLK) / 7),
	dgen_soundrate, NULL, NULL))
		goto ym2612_fail;
	if (SN76496_init(0, (((pal_mode) ? PAL_MCLK : NTSC_MCLK) / 15),
	dgen_soundrate, 16)) {
		YM2612Shutdown();
		ym2612_fail: fprintf(stderr,
		"main: Couldn't start sound chipset emulators!\n");
		return 1;
	}

	/*
	 // Initialize the platform-dependent stuff.
	 if (!pd_graphics_init(dgen_sound, pal_mode, hz))
	 {
	 fprintf(stderr, "main: Couldn't initialize graphics!\n");
	 return 1;
	 }
	 if(dgen_sound)
	 {
	 if (dgen_soundsegs < 0)
	 dgen_soundsegs = 0;
	 samples = (dgen_soundsegs * (dgen_soundrate / hz));
	 dgen_sound = pd_sound_init(dgen_soundrate, samples);
	 }
	 */
	return 0;
}

void md_shutdown(md& megad) {
	fprintf(stderr,"md_shutdown\n");

	ram_save(megad);
	if(dgen_autosave)
	{
		slot = 0;
		md_save(megad);
	}

	megad.unplug();

	//  pd_quit();

	YM2612Shutdown();
}

static inline void do_demo(md& megad, FILE* demo, enum demo_status* status) {
	uint32_t pad[2];

	switch (*status) {
	case DEMO_OFF:
		break;
	case DEMO_RECORD:
		pad[0] = h2be32(megad.pad[0]);
		pad[1] = h2be32(megad.pad[1]);
		fwrite(&pad, sizeof(pad), 1, demo);
		break;
	case DEMO_PLAY:
		if (fread(&pad, sizeof(pad), 1, demo) == 1) {
			megad.pad[0] = be2h32(pad[0]);
			megad.pad[1] = be2h32(pad[1]);
		} else {
			if (feof(demo))
				pd_message("Demo finished.");
			else
				pd_message("Demo finished (read error).");
			*status = DEMO_OFF;
		}
		break;
	}
}

// Temporary garbage can string :)
static char temp[65536] = "";

// Show help and exit with code 2
static void help() {
	printf(
			"DGen/SDL v\"VER\"\n"
					"Usage: dgen [options] romname\n\n"
					"Where options are:\n"
					"    -v              Print version number and exit.\n"
					"    -r RCFILE       Read in the file RCFILE after parsing\n"
					"                    $HOME/.dgen/dgenrc.\n"
					"    -n USEC         Causes DGen to sleep USEC microseconds per frame, to\n"
					"                    be nice to other processes.\n"
					"    -p CODE,CODE... Takes a comma-delimited list of Game Genie (ABCD-EFGH)\n"
					"                    or Hex (123456:ABCD) codes to patch the ROM with.\n"
					"    -R (J|U|E)      Force emulator region. This does not affect -P.\n"
					"    -P              Use PAL mode (50Hz) instead of normal NTSC (60Hz).\n"
					"    -H HZ           Use a custom frame rate.\n"
					"    -d DEMONAME     Record a demo of the game you are playing.\n"
					"    -D DEMONAME     Play back a previously recorded demo.\n"
					"    -s SLOT         Load the saved state from the given slot at startup.\n"
#ifdef WITH_JOYSTICK
			"    -j              Use joystick if detected.\n"
#endif
			);
	// Display platform-specific options
	pd_help();
	exit(2);
}

// Save/load states
// It is externed from your implementation to change the current slot
// (I know this is a hack :)
void md_save(md& megad) {
	FILE *save;
	char file[256];

#ifndef __QNXNTO__
	if (((size_t) snprintf(file, sizeof(file), "%s.gs%d", megad.romname, slot)
			>= sizeof(file))
			|| ((save = dgen_fopen("/accounts/1000/shared/misc/dgen/saves",
					file, DGEN_WRITE)) == NULL)) {
		snprintf(temp, sizeof(temp), "Couldn't save state to slot %d!", slot);
		pd_message(temp);
		return;
	}
#else
    snprintf(file, sizeof(file), "%s/%s.gs%d"
    		 ,"/accounts/1000/shared/misc/dgen/saves", megad.romname, slot);
    fprintf(stderr,"save-state: %s\n",file);
	save = fopen(file, "wb");
    if(!save) {
    	pd_message("Couldn't save state to slot");
    	perror("file error");
	    return;
    }

#endif

	megad.export_gst(save);
	fclose(save);
	snprintf(temp, sizeof(temp), "Saved state to slot %d.", slot);
	pd_message(temp);
}

void md_load(md& megad) {
	FILE *load;
	char file[256];

#ifndef __QNXNTO__
	if (((size_t) snprintf(file, sizeof(file), "%s.gs%d", megad.romname, slot)
			>= sizeof(file))
			|| ((load = dgen_fopen("/accounts/1000/shared/misc/dgen/saves",
					file, DGEN_READ)) == NULL)) {
		snprintf(temp, sizeof(temp), "Couldn't load state from slot %d!", slot);
		pd_message(temp);
		return;
	}

#else
	   snprintf(file, sizeof(file), "%s/%s.gs%d"
	    		 ,"/accounts/1000/shared/misc/dgen/saves", megad.romname, slot);
	   fprintf(stderr,"load-state: %s\n",file);
	   load = fopen(file, "rb");
	   if(!load) {
		   perror("md_load");
		   pd_message("state load failed");
		   return;
	   }

#endif

	megad.import_gst(load);
	fclose(load);
	snprintf(temp, sizeof(temp), "Loaded state from slot %d.", slot);
	pd_message(temp);
}

// Load/save states from file
void ram_save(md& megad) {
	mkdir("/accounts/1000/shared/misc/dgen/saves", 0777);
	char saves[256] = "/accounts/1000/shared/misc/dgen/saves/";
	strcat(saves, romfilename);

	fprintf(stderr,"ram_save: '%s'\n", saves);
	FILE *save = fopen(saves, "w+b");

	int ret;

	if (!megad.has_save_ram())
		return;

	if (save == NULL)
		goto fail;
	ret = megad.put_save_ram(save);
	fclose(save);
	if (ret == 0)
		return;
	fail: fprintf(stderr, "Couldn't save battery RAM to `%s'\n", megad.romname);
}

void ram_load(md& megad) {
	fprintf(stderr,"ram_load save files \n");
	char saves[256] = "/accounts/1000/shared/misc/dgen/saves/";
	strcat(saves, romfilename);

	fprintf(stderr,"ram_load: '%s \n", saves);

	FILE *load = fopen( saves,"r");
	int ret;

	if (!megad.has_save_ram())
	return;

	if (load == NULL)
	goto fail;
	ret = megad.get_save_ram(load);
	fclose(load);
	if (ret == 0)
	return;

	fail:
	fprintf(stderr, "Couldn't load battery RAM from `%s'\n",
			megad.romname);
}

int md_create(md& megad, int palmode) {
	pal_mode = palmode ? 1 : 0;
	hz = pal_mode ? 50 : 60;
	fprintf(stderr,"create megad object switching to %s \n", pal_mode ? "pal" :"ntsc");

	if (region == ('e' & ~(0x20)))  // if region is EUROPE
		if (!pal_mode)                   // if switch to NTSC then ...
			region = ('u' & ~(0x20));  // set US region.

	if (region == ('u' & ~(0x20)))     // if region is US
		if (pal_mode)                    // if switch to PAL then ...
			region = ('e' & ~(0x20));  // set EUROPE region.

	///restart sound chips...

	YM2612Shutdown();
	if (YM2612Init(1, (((pal_mode) ? PAL_MCLK : NTSC_MCLK) / 7),
	dgen_soundrate, NULL, NULL)) {
		fprintf(stderr,
		"main: Couldn't start sound YM chip...\n");
	}

	if (SN76496_init(0, (((pal_mode) ? PAL_MCLK : NTSC_MCLK) / 15),
	dgen_soundrate, 16)) {
		fprintf(stderr,
		"main: Couldn't start sound SN76496 chip ...\n");
	}
	pd_sound_reset(pal_mode);

	megad.recreate(pal_mode, region);

	if (!megad.okay()) {
		fprintf(stderr, "main: Megadrive init failed!\n");
		return 1;
	} else {
		fprintf(stderr,"main: %s initialized.\n", pal_mode ? "megadrive" : "genesis");
	}

	// Set untouched pads
	megad.pad[0] = megad.pad[1] = 0xF303F;
#ifdef WITH_JOYSTICK
	if (dgen_joystick)
		megad.init_joysticks(dgen_joystick1_dev, dgen_joystick2_dev);
#endif

	megad.reset();  // reset

	pd_message(pal_mode ? "    reset to PAL" : "    reset to NTSC");

	// Start the timing refs
	startclk = pd_usecs();
	oldclk = startclk;
	fpsclk = startclk;

	return 0;
}

void shutdown_quit(md& megad) {
	ram_save(megad);
	if (dgen_autosave) {
		slot = 0;
		md_save(megad);
	}
	megad.unplug();
	pd_quit();
	YM2612Shutdown();
}

int main(int argc, char *argv[]) {
	rombrowser_setup();

	FILE *file = NULL;
	enum demo_status demo_status = DEMO_OFF;

	// Parse the RC file
	if ((file = dgen_fopen_rc(DGEN_READ)) != NULL) {
		parse_rc(file, DGEN_RC);
		fclose(file);
		file = NULL;
		pd_rc();
	} else if (errno== ENOENT) {
		if ((file = dgen_fopen_rc(DGEN_APPEND)) != NULL) {
			fprintf(file,
			"# DGen \" VER \" configuration file.\n"
			"# See dgenrc(5) for more information.\n");
			fclose(file);
			file = NULL;
		}
	}
	else
	fprintf(stderr, "rc: %s: %s\n", DGEN_RC, strerror(errno));

	// Check all our options
	snprintf(temp, sizeof(temp), "%s%s", "s:hvr:n:p:R:PH:jd:D:", pd_options);
	while ((c = getopt(argc, argv, temp)) != EOF) {
		switch (c) {
		case 'v':
			// Show version and exit
			printf("DGen/SDL version \"VER\" \n");
			return 0;
		case 'r':
			// Parse another RC file or stdin
			if ((strcmp(optarg, "-") == 0)
					|| ((file = dgen_fopen(NULL, optarg,
							(DGEN_READ | DGEN_CURRENT))) != NULL)) {
				if (file == NULL)
					parse_rc(stdin, "(stdin)");
					else {
						parse_rc(file, optarg);
						fclose(file);
						file = NULL;
					}
					pd_rc();
				}
				else
				fprintf(stderr, "rc: %s: %s\n", optarg, strerror(errno));
				break;
				case 'n':
				// Sleep for n microseconds
							dgen_nice = atoi(optarg);
							break;
							case 'p':
							// Game Genie patches
							patches = optarg;
							break;
							case 'R':
							// Region selection
							if (strlen(optarg) != 1)
							goto bad_region;
							switch (optarg[0] | 0x20) {
								case 'u':
								case 'j':
								case 'e':
								break;
								default:
								bad_region:
								fprintf(stderr, "main: invalid region `%s'.\n",
										optarg);
								return EXIT_FAILURE;
							}
							region = (optarg[0] & ~(0x20));
							break;
							case 'P':
							// PAL mode
							hz = 50;
							pal_mode = 1;
							break;
							case 'H':
							// Custom frame rate
							hz = atoi(optarg);
							if ((hz <= 0) || (hz > 1000)) {
								fprintf(stderr, "main: invalid frame rate (%d).\n",
										hz);
								hz = (pal_mode ? 50 : 60);
							}
							break;
#ifdef WITH_JOYSTICK
							case 'j':
							// Phil's joystick code
							dgen_joystick = 1;
							break;
#endif
							case 'd':
							// Record demo
							if(file)
							{
								fprintf(stderr,"main: Can't record and play at the same time!\n");
								break;
							}
							if(!(file = dgen_fopen("demos", optarg, DGEN_WRITE)))
							{
								fprintf(stderr, "main: Can't record demo file %s!\n", optarg);
								break;
							}
							demo_status = DEMO_RECORD;
							break;
							case 'D':
							// Play demo
							if(file)
							{
								fprintf(stderr,"main: Can't record and play at the same time!\n");
								break;
							}
							if(!(file = dgen_fopen("demos", optarg, (DGEN_READ | DGEN_CURRENT))))
							{
								fprintf(stderr, "main: Can't play demo file %s!\n", optarg);
								break;
							}
							demo_status = DEMO_PLAY;
							break;
							case '?': // Bad option!
							case 'h':// A cry for help :)
							help();
							break;
							case 's':
							// Pick a savestate to autoload
							start_slot = atoi(optarg);
							break;
							default:
							// Pass it on to platform-dependent stuff
							pd_option(c, optarg);
							break;
						}
					}

#ifdef __BEOS__
					// BeOS snooze() sleeps in milliseconds, not microseconds
					dgen_nice /= 1000;
#endif

					// There should be a romname after all those options. If not, show help and
					// exit.
					/*
					 if(optind >= argc)
					 help();
					 */

#ifdef __QNXNTO__
	struct sched_param sparam;
	sched_getparam(0, &sparam);
	sparam.sched_priority += 1;
	sched_setscheduler(0, SCHED_RR, &sparam);
#endif

	// Start up the sound chips.
	if (YM2612Init(1, (((pal_mode) ? PAL_MCLK : NTSC_MCLK) / 7),
	dgen_soundrate, NULL, NULL))
		goto ym2612_fail;
	if (SN76496_init(0, (((pal_mode) ? PAL_MCLK : NTSC_MCLK) / 15),
	dgen_soundrate, 16)) {
		YM2612Shutdown();
		ym2612_fail: fprintf(stderr,
		"main: Couldn't start sound chipset emulators!\n");
		return 1;
	}

	// Initialize the platform-dependent stuff.
	if (!pd_graphics_init(dgen_sound, pal_mode, hz)) {
		fprintf(stderr, "main: Couldn't initialize graphics!\n");
		fflush(stderr);
		sleep(1);
		return 1;
	}

	if (dgen_sound) {
		if (dgen_soundsegs < 0)
			dgen_soundsegs = 0;
		samples = (dgen_soundsegs * (dgen_soundrate / hz));
		dgen_sound = pd_sound_init(dgen_soundrate, samples);
	}
	/*
	 rom = argv[optind];
	 */

	if (romp.romCount()) {
		sprintf(rom, rombrowser_next()); // full path to rom file
		memset(&g_runningFile_str[0], 0, 63); // rom file name for display on screen.
		sprintf(&g_runningFile_str[0], rombrowser_get_rom_name());
	} else {
		fprintf(stderr,"no roms found...\n");
	}

		// Create the megadrive object
	md megad(pal_mode, region);

	if (!megad.okay()) {
		fprintf(stderr, "main: Megadrive init failed!\n");
		return 1;
	} else {
		fprintf(stderr,"main: %s initialized.\n", pal_mode ? "megadrive" :"genesis");
    }

		// load a default rom if none found in roms dir ...
	int romNotLoaded = megad.load(rom);
	if (romNotLoaded) {
		sprintf(rom, "app/native/demo_badapple.bin");
		romNotLoaded = megad.load(rom);
	}

	// Set untouched pads
	megad.pad[0] = megad.pad[1] = 0xF303F;
#ifdef WITH_JOYSTICK
	if (dgen_joystick)
		megad.init_joysticks(dgen_joystick1_dev, dgen_joystick2_dev);
#endif
	// Load patches, if given
	if (patches) {
		printf("main: Using patch codes %s\n", patches);
		megad.patch(patches, NULL, NULL, NULL);
	}
	// Fix checksum
	megad.fix_rom_checksum();
	// Reset
	megad.reset();

	// Load up save RAM
	ram_load(megad);
	// If autoload is on, load save state 0
	if (dgen_autoload) {
		slot = 0;
		md_load(megad);
	}
	// If -s option was given, load the requested slot
	if (start_slot >= 0) {
		slot = start_slot;
		md_load(megad);
	}

	// Start the timing refs
	startclk = pd_usecs();
	oldclk = startclk;
	fpsclk = startclk;

	// Start audio
	if (dgen_sound) {
		pd_sound_start();
		pd_sound_reset(pal_mode);
	}

	// Show cartridge header
	if (!romNotLoaded) {
		if (dgen_show_carthead)
			pd_show_carthead(megad);
	}

	// Go around, and around, and around, and around... ;)
	fprintf(stderr,"launch!\n");
	fflush(stdout);

	while (running) {

		if (switch_mode_g) {
			if (pal_mode)
				md_create(megad, 0); // delete megad object, recreate with  NTSC/PAL toggling.
			else
				md_create(megad, 1);

			if (-1 == md_load_new_rom(megad, rom)) {
				load_new_game_g = 1;
			}

			switch_mode_g = 0;

			/*
			 load_new_game_g = 0;
			 strcpy(rom,  rombrowser_get_rom_name() );
			 if( -1 == md_load_new_rom(megad,rom) ) {
			 fprintf(stderr,"loading current rom failed, attempt new rom load ... \n");
			 load_new_game_g = 1;
			 }
			 */
		}

		if (load_new_game_g) {
			fprintf(stderr,"load new ROM ...\n");
			// bb10 Dialog here ...
			if( load_new_game_g == 1)
			strcpy(rom, rombrowser_next() );
			else
			strcpy(rom,rombrowser_next() );

			load_new_game_g = 0;

			if( -1 == md_load_new_rom(megad,rom) ) {
				rombrowser_update();  // re-read ROM dir
				sleep(3);// wait a few seconds ...
				printf("md_load_new_rom: failed to locate ROM file\n");
				pd_message("         no roms found, add some...");
				pd_graphics_update();
				load_new_game_g = 1;
				continue;
			}

			pd_message( megad.cart_head.domestic_name );
			pd_graphics_update();
			// md_shutdown()
			// md_startup();
			// playbook_load()
		}

		const unsigned int usec_frame = (1000000 / hz);
		unsigned long tmp;
		int frames_todo;

		newclk = pd_usecs();

		if (pd_stopped()) {
			fprintf(stderr,"pd_stopped\n");
			// Fix FPS count.
			tmp = (newclk - oldclk);
			startclk += tmp;
			fpsclk += tmp;
			oldclk = newclk;
		}

		// Update FPS count.
		tmp = ((newclk - fpsclk) & 0x3fffff);
		if (tmp >= 1000000) {
			fpsclk = newclk;
			if (frames_old > frames)
				fps = (frames_old - frames);
			else
				fps = (frames - frames_old);
			frames_old = frames;
			//fprintf(stderr,"FPS %lu \n",fps);
		}

		if (dgen_frameskip == 0)
			goto do_not_skip;

		// Measure how many frames to do this round.
		usec += ((newclk - oldclk) & 0x3fffff); // no more than 4 secs
		frames_todo = (usec / usec_frame);
		usec %= usec_frame;
		oldclk = newclk;

		if (frames_todo == 0) {
			// No frame to do yet, relax the CPU until next one.
			tmp = (usec_frame - usec);
			if (tmp > 1000) {
				// Never sleep for longer than the 50Hz value
				// so events are checked often enough.
				if (tmp > (1000000 / hz))
					tmp = (1000000 / hz);
				tmp -= 1000;
				usleep(tmp);
			}
		} else {
			// Draw frames.
			while (frames_todo != 1) {
				// do_demo(megad, file, &demo_status);
				if (dgen_sound) {
					// Skip this frame, keep sound going.
					megad.one_frame(NULL, NULL, &sndi);
					pd_sound_write();
				} else
					megad.one_frame(NULL, NULL, NULL);
				--frames_todo;
			}
			--frames_todo;
			do_not_skip:
			//do_demo(megad, file, &demo_status);
			if (dgen_sound) {
				megad.one_frame(&mdscr, mdpal, &sndi);
				pd_sound_write();
			} else
				megad.one_frame(&mdscr, mdpal, NULL);
			if ((mdpal) && (pal_dirty)) {
				pd_graphics_palette_update();
				pal_dirty = 0;
			}
			pd_graphics_update();
			++frames;
		}

		running = pd_handle_events(megad);

		if (dgen_nice) {
#ifdef __BEOS__
			snooze(dgen_nice);
#else
			usleep(dgen_nice);
#endif
		}
	}

	// Print fps
	fpsclk = ((pd_usecs() - startclk) / 1000000);
	if (fpsclk == 0)
		fpsclk = 1;
	printf("%lu frames per second (average %lu, optimal %d)\n", fps,
			(frames / fpsclk), hz);

	fprintf(stderr,"shutdown...\n");

	if (file)
		fclose(file);
	shutdown_quit(megad);

	// Come back anytime :)
	return 0;
}
