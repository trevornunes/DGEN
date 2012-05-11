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
#include "utils\xstring.h"
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

char g_runningFile_str[64];

static vector<string> vsList;

void md_shutdown(md& megad);
int md_startup(md& megad);
void md_load(md& megad);
void md_save(md& megad);
void ram_load(md& megad);
void ram_save(md& megad);

#endif
// Defined in ras.cpp, and set to true if the Genesis palette's changed.
extern int pal_dirty;
int load_new_game_g = 0;

int slot = 0;  // see md_save and related
unsigned long oldclk, newclk, startclk, fpsclk;  // used in main and playbook_load ...
int start_slot = -1;
int c = 0, pal_mode = 0, running = 1, usec = 0;
unsigned long frames = 0, frames_old = 0, fps = 0;
char *patches = NULL;
char rom[256];
FILE *debug_log = NULL;
unsigned int samples;
unsigned int hz = 60;
char region = '\0';

int gameIndex = 0;
char romfilename[256];
// Do a demo frame, if active
enum demo_status {
	DEMO_OFF,
	DEMO_RECORD,
	DEMO_PLAY
};

void playbook_setup(void)
{

  mkdir("/accounts/1000/shared/misc/dgen",0777);
  chmod("/accounts/1000/shared/misc/dgen",0777);
  perror("dgen 777 ");

  mkdir("/accounts/1000/shared/misc/roms/smd",0777);
  chmod("/accounts/1000/shared/misc/roms/smd",0777);
  perror("smd 777");

// update rom list here? ROMLIST
  fprintf(stderr,"playbook_setup: done\n");
}

vector<string> GetRomDirListing( const char *dpath )

{
 //vector<string> vsList;

#ifdef __PLAYBOOK__
	DIR* dirp;
	struct dirent* direntp;

#endif

if(!dpath)
{
    fprintf(stderr,"dpath is null.\n");
	return vsList;
}

#ifdef __PLAYBOOK__

  dirp = opendir( "/accounts/1000/shared/misc/roms/smd/" );
  if( dirp != NULL )
  {
	 for(;;)
	 {
		direntp = readdir( dirp );
		if( direntp == NULL )
		  break;

		 fprintf(stderr,"FILE: '%s' \n", direntp->d_name);
		// FCEUI_DispMessage(direntp->d_name,0);
		  string tmp = direntp->d_name;

		  if( strcmp( direntp->d_name, ".") == 0)
		  {
			 continue;
		  }

		  if( strcmp( direntp->d_name,"..") == 0)
			  continue;

		  if( (tmp.substr(tmp.find_last_of(".") + 1) == "smd") ||
			  (tmp.substr(tmp.find_last_of(".") + 1) == "SMD") ||
			  (tmp.substr(tmp.find_last_of(".") + 1) == "bin") ||
			  (tmp.substr(tmp.find_last_of(".") + 1) == "BIN"));
		  {
	         // fprintf(stderr,"ROM: %s\n", direntp->d_name);
			  vsList.push_back(direntp->d_name);
		  }
	 }
 }
  else
  {
	fprintf(stderr,"dirp is NULL ...\n");
  }

#endif
 fprintf(stderr,"number of files %d\n", vsList.size() );

 if (vsList.size() == 0) {
	 dialog_instance_t alert_dialog = 0;
	 	 dialog_request_events(0);    //0 indicates that all events are requested
	 	 if (dialog_create_alert(&alert_dialog) != BPS_SUCCESS) {
	 		 fprintf(stderr, "Failed to create alert dialog.");
	      //return EXIT_FAILURE;
	 	 }
	 	 dialog_set_title_text(alert_dialog, "GBColor-PB Error Report");
	 	 if (dialog_set_alert_html_message_text(alert_dialog, "<u><b>ERROR:</b> You do not have any ROMS!<br></u><b>Add smd/bin ROMS to:</b> <br><u>\"shared/misc/roms/smd\"</u><br>using either <i>WiFi sharing</i> or <i>Desktop Software.</i>")
	 			 != BPS_SUCCESS) {
	 		 fprintf(stderr, "Failed to set alert dialog message text.");
	 		 dialog_destroy(alert_dialog);
	 		 alert_dialog = 0;
	        //return EXIT_FAILURE;
	 	 }

	 	 char* cancel_button_context = "Cancelled";

	 	 if (dialog_add_button(alert_dialog, "OK", true, 0, true)
	 			 != BPS_SUCCESS) {
	 		 fprintf(stderr, "Failed to add button to alert dialog.");
	 		 dialog_destroy(alert_dialog);
	 		 alert_dialog = 0;
	        //return EXIT_FAILURE;
	 	 }

	 	 if (dialog_show(alert_dialog) != BPS_SUCCESS) {
	 		 fprintf(stderr, "Failed to show alert dialog.");
	 		 dialog_destroy(alert_dialog);
	 		 alert_dialog = 0;
	 		 //return EXIT_FAILURE;
	 	 }

	 	 while (1) {
	 		 bps_event_t *event = NULL;
	 		 bps_get_event(&event, -1);    // -1 means that the function waits
	                                    // for an event before returning

	 		 if (event) {
	 			 if (bps_event_get_domain(event) == dialog_get_domain()) {

	 				 int selectedIndex =
	 						 dialog_event_get_selected_index(event);
	 				 const char* label =
	 						 dialog_event_get_selected_label(event);
	 				 const char* context =
	 						 dialog_event_get_selected_context(event);

	 				 exit(-1);
	 			 }
	 		 }
	 	 }

	 	 if (alert_dialog) {
	 		 dialog_destroy(alert_dialog);
	 	 }
	  }

 return vsList;
}



#ifdef __PLAYBOOK__
static pthread_mutex_t loader_mutex = PTHREAD_MUTEX_INITIALIZER;
static vector<string> vecList;
vector<string> sortedvecList;
#endif

//
//
//


vector<string> sortAlpha(vector<string> sortThis)
{
     int swap;
     string temp;

     do
     {
         swap = 0;
         for (int count = 0; count < sortThis.size() - 1; count++)
         {
             if (sortThis.at(count) > sortThis.at(count + 1))
             {
                   temp = sortThis.at(count);
                   sortThis.at(count) = sortThis.at(count + 1);
                   sortThis.at(count + 1) = temp;
                   swap = 1;
             }
         }
     }while (swap != 0);

     return sortThis;
}


void UpdateRomList(void)
{
  vecList = GetRomDirListing("/accounts/1000/shared/misc/roms/smd");
  sortedvecList = sortAlpha(vecList);
}

/******************************
 *
 ******************************/
int md_load_new_rom(md& megad, char *rom)
{

	if(!rom)
		return -1;

	// suspend CPU , save ...
    md_shutdown(megad);
    md_startup(megad);

	// Load the requested ROM
	  if(megad.load(rom))
	    {
	      fprintf(stderr, "main: Couldn't load ROM file %s!\n", rom);
	      return -1;
	    }
	  // Set untouched pads
	  megad.pad[0] = megad.pad[1] = 0xF303F;
	#ifdef WITH_JOYSTICK
	  if(dgen_joystick)
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
	  if(dgen_autoload)
	    {
	      slot = 0;
	      md_load(megad);
	    }

	  // If -s option was given, load the requested slot
	  if(start_slot >= 0)
	    {
	      slot = start_slot;
	      md_load(megad);
	    }

	  // Start the timing refs
	  startclk = pd_usecs();
	  oldclk = startclk;
	  fpsclk = startclk;

	//  if(dgen_sound) pd_sound_start();

//  pd_show_carthead(megad);

  return 1;
}



int md_startup(md& megad)
{
	// Start up the sound chips.
	if (YM2612Init(1,
		       (((pal_mode) ? PAL_MCLK : NTSC_MCLK) / 7),
		       dgen_soundrate, NULL, NULL))
		goto ym2612_fail;
	if (SN76496_init(0,
			 (((pal_mode) ? PAL_MCLK : NTSC_MCLK) / 15),
			 dgen_soundrate, 16)) {
		YM2612Shutdown();
	ym2612_fail:
		fprintf(stderr,
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

void md_shutdown(md& megad)
{
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



static inline void do_demo(md& megad, FILE* demo, enum demo_status* status)
{
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
		}
		else {
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
static void help()
{
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
void md_save(md& megad)
{
	FILE *save;
	char file[256];

	if (((size_t)snprintf(file,
			      sizeof(file),
			      "%s.gs%d",
			      megad.romname,
			      slot) >= sizeof(file)) ||
	    ((save = dgen_fopen("saves", file, DGEN_WRITE)) == NULL)) {
		snprintf(temp, sizeof(temp),
			 "Couldn't save state to slot %d!", slot);
		pd_message(temp);
		return;
	}
	megad.export_gst(save);
	fclose(save);
	snprintf(temp, sizeof(temp), "Saved state to slot %d.", slot);
	pd_message(temp);
}

void md_load(md& megad)
{
	FILE *load;
	char file[256];

	if (((size_t)snprintf(file,
			      sizeof(file),
			      "%s.gs%d",
			      megad.romname,
			      slot) >= sizeof(file)) ||
	    ((load = dgen_fopen("saves", file, DGEN_READ)) == NULL)) {
		snprintf(temp, sizeof(temp),
			 "Couldn't load state from slot %d!", slot);
		pd_message(temp);
		return;
	}
	megad.import_gst(load);
	fclose(load);
	snprintf(temp, sizeof(temp), "Loaded state from slot %d.", slot);
	pd_message(temp);
}

// Load/save states from file
void ram_save(md& megad)
{
	mkdir("/accounts/1000/shared/misc/dgen/saves", 0777);
	char saves[256] = "/accounts/1000/shared/misc/dgen/saves/";
	strcat(saves, romfilename);

	FILE *save = fopen( saves,"w+b");
	int ret;

	if (!megad.has_save_ram())
		return;

	if (save == NULL)
		goto fail;
	ret = megad.put_save_ram(save);
	fclose(save);
	if (ret == 0)
		return;
fail:
	fprintf(stderr, "Couldn't save battery RAM to `%s'\n", megad.romname);
}

void ram_load(md& megad)
{

	char saves[256] = "/accounts/1000/shared/misc/dgen/saves/";
	strcat(saves, romfilename);

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

int main(int argc, char *argv[])
{
	playbook_setup();
	bps_initialize();
	dialog_request_events(0);
	UpdateRomList();
  FILE *file = NULL;
  enum demo_status demo_status = DEMO_OFF;


  playbook_setup();

	// Parse the RC file
	if ((file = dgen_fopen_rc(DGEN_READ)) != NULL) {
		parse_rc(file, DGEN_RC);
		fclose(file);
		file = NULL;
		pd_rc();
	}
	else if (errno == ENOENT) {
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
  while((c = getopt(argc, argv, temp)) != EOF)
    {
      switch(c)
	{
	case 'v':
	  // Show version and exit
	  printf("DGen/SDL version \"VER\" \n");
	  return 0;
	case 'r':
	  // Parse another RC file or stdin
	  if ((strcmp(optarg, "-") == 0) ||
	      ((file = dgen_fopen(NULL, optarg,
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
	case 'h': // A cry for help :)
	  help();
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

	// Start up the sound chips.
	if (YM2612Init(1,
		       (((pal_mode) ? PAL_MCLK : NTSC_MCLK) / 7),
		       dgen_soundrate, NULL, NULL))
		goto ym2612_fail;
	if (SN76496_init(0,
			 (((pal_mode) ? PAL_MCLK : NTSC_MCLK) / 15),
			 dgen_soundrate, 16)) {
		YM2612Shutdown();
	ym2612_fail:
		fprintf(stderr,
			"main: Couldn't start sound chipset emulators!\n");
		return 1;
	}

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
/*
  rom = argv[optind];
*/


  int status = 0;


      pthread_mutex_lock(&loader_mutex);
      const char ** list = 0;
      int count = 0;
      list = (const char**)malloc(sortedvecList.size()*sizeof(char*));



      for(;;){
      	if(count >= sortedvecList.size()) break;
      	fprintf(stderr, "%d \n", count);
      	list[count] = sortedvecList[count].c_str();
      	count++;
      }

      // ROM selector
         dialog_instance_t dialog = 0;
         int i, rc;
         bps_event_t *event;
         int domain = 0;
         const char * label;

         dialog_create_popuplist(&dialog);
         dialog_set_popuplist_items(dialog, list, sortedvecList.size());

             char* cancel_button_context = "Canceled";
             char* okay_button_context = "Okay";
             dialog_add_button(dialog, DIALOG_CANCEL_LABEL, true, cancel_button_context, true);
             dialog_add_button(dialog, DIALOG_OK_LABEL, true, okay_button_context, true);
             dialog_set_popuplist_multiselect(dialog, false);
             dialog_show(dialog);

             while(1){
                 bps_get_event(&event, -1);

                 if (event) {
                     domain = bps_event_get_domain(event);
                     if (domain == dialog_get_domain()) {
                         int *response[1];
                         int num;

                         label = dialog_event_get_selected_label(event);

                         if(strcmp(label, DIALOG_OK_LABEL) == 0){
                             dialog_event_get_popuplist_selected_indices(event, (int**)response, &num);
                             if(num != 0){
                                 //*response[0] is the index that was selected
                                 printf("%s", list[*response[0]]);fflush(stdout);
                                 strcpy(romfilename, list[*response[0]]);
                             }
                             bps_free(response[0]);
                         } else {
                             printf("User has canceled ISO dialog.");
                             return false;
                         }
                         break;
                     }
                 }
             }


      //test end

  	fprintf(stderr,"AutoLoadRom\n");
      string baseDir ="/accounts/1000/shared/misc/roms/smd/";

      memset(&g_runningFile_str[0],0,64);
      sprintf(&g_runningFile_str[0], romfilename);

      baseDir = baseDir + romfilename;
      fprintf(stderr,"loading: %d/%d '%s'\n",gameIndex + 1, sortedvecList.size(), baseDir.c_str() );






     pthread_mutex_unlock(&loader_mutex);  // -lpthread normally would be added, it's already in PB runtime.

     free(list);
     fprintf(stderr,"still loading?\n");
    strcpy(rom, baseDir.c_str());
    fprintf(stderr,"still loading?\n");
    fprintf(stderr,"main: rom=%s \n",rom);

  // Create the megadrive object
  md megad(pal_mode, region);
  if(!megad.okay())
    {
      fprintf(stderr, "main: Megadrive init failed!\n");
      return 1;
    }





  // Load the requested ROM
  if(megad.load(rom))
    {
      fprintf(stderr, "main: Couldn't load ROM file %s!\n", rom);
      return 1;
    }
  // Set untouched pads
  megad.pad[0] = megad.pad[1] = 0xF303F;
#ifdef WITH_JOYSTICK
  if(dgen_joystick)
    megad.init_joysticks(dgen_joystick1_dev, dgen_joystick2_dev);
#endif
  // Load patches, if given
  if(patches)
    {
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
  if(dgen_autoload)
    {
      slot = 0;
      md_load(megad);
    }
  // If -s option was given, load the requested slot
  if(start_slot >= 0)
    {
      slot = start_slot;
      md_load(megad);
    }

  // Start the timing refs
  startclk = pd_usecs();
  oldclk = startclk;
  fpsclk = startclk;

  // Start audio
  if(dgen_sound) pd_sound_start();

  // Show cartridge header
  if(dgen_show_carthead) pd_show_carthead(megad);

	// Go around, and around, and around, and around... ;)
	while (running) {

		if(load_new_game_g)
        {
		  load_new_game_g = 0;

		  int status = 0;


		      pthread_mutex_lock(&loader_mutex);
		      const char ** list = 0;
		      int count = 0;
		      list = (const char**)malloc(sortedvecList.size()*sizeof(char*));



		      for(;;){
		      	if(count >= sortedvecList.size()) break;
		      	fprintf(stderr, "%d \n", count);
		      	list[count] = sortedvecList[count].c_str();
		      	count++;
		      }

		      // ROM selector
		         dialog_instance_t dialog = 0;
		         int i, rc;
		         bps_event_t *event;
		         int domain = 0;
		         const char * label;

		         dialog_create_popuplist(&dialog);
		         dialog_set_popuplist_items(dialog, list, sortedvecList.size());

		             char* cancel_button_context = "Canceled";
		             char* okay_button_context = "Okay";
		             dialog_add_button(dialog, DIALOG_CANCEL_LABEL, true, cancel_button_context, true);
		             dialog_add_button(dialog, DIALOG_OK_LABEL, true, okay_button_context, true);
		             dialog_set_popuplist_multiselect(dialog, false);
		             dialog_show(dialog);

		             while(1){
		                 bps_get_event(&event, -1);

		                 if (event) {
		                     domain = bps_event_get_domain(event);
		                     if (domain == dialog_get_domain()) {
		                         int *response[1];
		                         int num;

		                         label = dialog_event_get_selected_label(event);

		                         if(strcmp(label, DIALOG_OK_LABEL) == 0){
		                             dialog_event_get_popuplist_selected_indices(event, (int**)response, &num);
		                             if(num != 0){
		                                 //*response[0] is the index that was selected
		                                 printf("%s", list[*response[0]]);fflush(stdout);
		                                 strcpy(romfilename, list[*response[0]]);
		                             }
		                             bps_free(response[0]);
		                         } else {
		                             printf("User has canceled ISO dialog.");
		                             return false;
		                         }
		                         break;
		                     }
		                 }
		             }


		      //test end

		  	fprintf(stderr,"AutoLoadRom2\n");
		      string baseDir ="/accounts/1000/shared/misc/roms/smd/";

		      memset(&g_runningFile_str[0],0,64);
		      sprintf(&g_runningFile_str[0], romfilename);

		      baseDir = baseDir + romfilename;
		      fprintf(stderr,"loading: %d/%d '%s'\n",gameIndex + 1, sortedvecList.size(), baseDir.c_str() );

		      strcpy(rom, baseDir.c_str());




		     pthread_mutex_unlock(&loader_mutex);  // -lpthread normally would be added, it's already in PB runtime.

		     free(list);


		   md_load_new_rom(megad,rom);
		  // md_shutdown()
	      // md_startup();
		  // playbook_load()
        }

		const unsigned int usec_frame = (1000000 / hz);
		unsigned long tmp;
		int frames_todo;

		newclk = pd_usecs();

		if (pd_stopped()) {
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
				if (tmp > (1000000 / 50))
					tmp = (1000000 / 50);
				tmp -= 1000;
#ifdef __BEOS__
				snooze(tmp / 1000);
#else
				usleep(tmp);
#endif
			}
		}
		else {
			// Draw frames.
			while (frames_todo != 1) {
				do_demo(megad, file, &demo_status);
				if (dgen_sound) {
					// Skip this frame, keep sound going.
					megad.one_frame(NULL, NULL, &sndi);
					pd_sound_write();
				}
				else
					megad.one_frame(NULL, NULL, NULL);
				--frames_todo;
			}
			--frames_todo;
		do_not_skip:
			do_demo(megad, file, &demo_status);
			if (dgen_sound) {
				megad.one_frame(&mdscr, mdpal, &sndi);
				pd_sound_write();
			}
			else
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
	printf("%lu frames per second (average %lu, optimal %d)\n",
	       fps, (frames / fpsclk), hz);

  // Cleanup
  if(file) fclose(file);
  ram_save(megad);
  if(dgen_autosave) { slot = 0; md_save(megad); }
  megad.unplug();
  pd_quit();
  YM2612Shutdown();

  // Come back anytime :)
  return 0;
}
