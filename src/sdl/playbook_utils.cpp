/*
 * playbook_utils.cpp
 *
 *  Created on: Jan 10, 2012
 *      Author: tnunes
 */


#include "playbook_utils.h"
#include "dirent.h"


static const char *romPath_xxx = "/accounts/1000/shared/misc/roms";
static const char *romPath_nes = "/accounts/1000/shared/misc/roms/nes";
static const char *romPath_gba = "/accounts/1000/shared/misc/roms/gba";
static const char *romPath_pce = "/accounts/1000/shared/misc/roms/pce";
static const char *romPath_smd = "/accounts/1000/shared/misc/roms/smd";



//*********************************************************
//
//*********************************************************
PlaybookRom::PlaybookRom(rom_type_t rtype)
{
  activeRom_m = "";
  activeRomPath_m = "";
  activeRomList_vsm.clear();
  romType_m = rtype;
  activeRomIndex_m =0;

  switch(romType_m)
  {
    case rom_nes_c:
    	 activeRomPath_m = romPath_nes;
         break;

    case rom_gba_c:
    	 activeRomPath_m = romPath_gba;
         break;

    case rom_pce_c:
    	 activeRomPath_m = romPath_pce;
    	 break;

    case rom_smd_c:
    	 activeRomPath_m = romPath_smd;
    	 break;

    default:  activeRomPath_m = romPath_xxx;
              break;
  }

  // root path is now set ...

}



//*********************************************************
//
//*********************************************************
bool PlaybookRom::pathExists(string dpath)
{
	DIR *dp =0;
	if( (dp=opendir(dpath.c_str())) == NULL) {
		closedir(dp);
		return false;
	}
	 else
	{
		closedir(dp);
		return true;
	}
}

void PlaybookRom::setRomPath(string dpath)
{
 if( pathExists(dpath) == true)
 {
     activeRomPath_m = dpath;
     getRomList();
 }
}

//*********************************************************
//
//*********************************************************
vector<string> PlaybookRom::getRomList( void )
{

	DIR* dirp;
	struct dirent* direntp;

	if(activeRomPath_m == "")
	{
        activeRomList_vsm.clear();
		return activeRomList_vsm;
	}

  dirp = opendir( activeRomPath_m.c_str() );
  if( dirp != NULL )
  {
	 for(;;)
	 {
		direntp = readdir( dirp );
		if( direntp == NULL )
		  break;

		// fprintf(stderr,"FILE: '%s' \n", direntp->d_name);
   	     string tmp = direntp->d_name;

		  if( strcmp( direntp->d_name, ".") == 0)
		  {
			 continue;
		  }

		  if( strcmp( direntp->d_name,"..") == 0)
			  continue;


		  // TODO: At this point we should check extensions based on
		  // active emulator e.g. PCE and invoke a method to check
		  // romCheckExtension()  which internally works out what is supported.

		  if( (tmp.substr(tmp.find_last_of(".") + 1) == "nes") ||  // toupper this sucka!
			  (tmp.substr(tmp.find_last_of(".") + 1) == "NES") ||
			  (tmp.substr(tmp.find_last_of(".") + 1) == "gba") ||
			  (tmp.substr(tmp.find_last_of(".") + 1) == "gb")  ||
			  (tmp.substr(tmp.find_last_of(".") + 1) == "GB")  ||
			  (tmp.substr(tmp.find_last_of(".") + 1) == "gbc") ||
			  (tmp.substr(tmp.find_last_of(".") + 1) == "GBC") ||
			  (tmp.substr(tmp.find_last_of(".") + 1) == "GBA") ||
			  (tmp.substr(tmp.find_last_of(".") + 1) == "pce") ||
			  (tmp.substr(tmp.find_last_of(".") + 1) == "PCE") ||
			  (tmp.substr(tmp.find_last_of(".") + 1) == "zip") ||
			  (tmp.substr(tmp.find_last_of(".") + 1) == "ZIP") ||
			  (tmp.substr(tmp.find_last_of(".") + 1) == "smd") ||
			  (tmp.substr(tmp.find_last_of(".") + 1) == "SMD") ||
			  (tmp.substr(tmp.find_last_of(".") + 1) == "GEN") ||
			  (tmp.substr(tmp.find_last_of(".") + 1) == "gen") ||
			  (tmp.substr(tmp.find_last_of(".") + 1) == "bin") )
		  {
	          fprintf(stderr,"ROM: %s\n", direntp->d_name);
			  activeRomList_vsm.push_back(direntp->d_name);
		  }
	 }
  }
  else
  {
	fprintf(stderr,"dirp is NULL ...\n");
  }

 // sort list here .

 fprintf(stderr,"number of files %d\n", activeRomList_vsm.size() );
 return activeRomList_vsm;
}





//*********************************************************
//
//*********************************************************
const char *PlaybookRom::getRomNext(void)
{
    int status = 0;
    fprintf(stderr,"AutoLoadRom: %d\n", romType_m);
    static char romName[128];

    memset(romName,0,128);

	if( activeRomList_vsm.size() == 0)
	{
	   fprintf(stderr,"AutoLoadRom: error no games in vecList\n");
	   return "";
	}

    if(++activeRomIndex_m >= activeRomList_vsm.size())
    	activeRomIndex_m = 0;

    if( activeRomList_vsm.size() == 1)
    	activeRomIndex_m = 0;

    if( activeRomIndex_m == activeRomList_vsm.size())
    	activeRomIndex_m = 0;

    activeRom_m = activeRomList_vsm[activeRomIndex_m];
    string baseDir = activeRomPath_m + "/" + activeRomList_vsm[activeRomIndex_m];

    sprintf(romName,"%s",baseDir.c_str());

    fprintf(stderr,"loading: %d/%d '%s'\n", activeRomIndex_m, activeRomList_vsm.size(), baseDir.c_str() );
    return romName;
}



//*******************************
//
//*******************************
void PlaybookRom::updateRomList(void)
{
  (void) getRomList();
}




