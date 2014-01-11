/*
 * Simple rom path retrieval and file/path checker. Used by emulators to simplify the repetitive logic
 *
 */


#include "rompb.h"
#include <dirent.h>
#include <algorithm>  // find
#include <iostream>
#include <fstream>


static const char *romPath_xxx = "/accounts/1000/shared/misc/roms";
static const char *romPath_nes = "/accounts/1000/shared/misc/roms/nes";
static const char *romPath_gba = "/accounts/1000/shared/misc/roms/gba";
static const char *romPath_pce = "/accounts/1000/shared/misc/roms/pce";
static const char *romPath_smd = "/accounts/1000/shared/misc/roms/smd";
static const char *romPath_gb  = "/accounts/1000/shared/misc/roms/gb";

#define DEBUG 1

//*********************************************************
//
//*********************************************************
Rompb::Rompb(rom_type_t rtype)
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
    	 extensions_vsm.push_back("nes");
    	 extensions_vsm.push_back("NES");
         break;

    case rom_gb_c:
    case rom_gbc_c:
    	 activeRomPath_m = romPath_gb;
    	 extensions_vsm.push_back("gb");
    	 extensions_vsm.push_back("gbc");
    	 extensions_vsm.push_back("bin");
    	 extensions_vsm.push_back("zip");
    	break;

    case rom_gba_c:
    	 activeRomPath_m = romPath_gba;
    	 extensions_vsm.push_back("gba");
    	 extensions_vsm.push_back("GBA");
    	 extensions_vsm.push_back("zip");
    	 extensions_vsm.push_back("bin");
         break;

    case rom_pce_c:
    	 activeRomPath_m = romPath_pce;
    	 extensions_vsm.push_back("pce");
    	 break;

    case rom_smd_c:
    	 activeRomPath_m = romPath_smd;
    	 extensions_vsm.push_back("smd");
    	 extensions_vsm.push_back("gen");
    	 extensions_vsm.push_back("bin");
    	 break;

    default:  activeRomPath_m = romPath_xxx;
              break;
  }

  cfgFilePath_m = activeRomPath_m + "/pbrom.cfg";
#ifdef DEBUG
  cout << "cfgFilePath_m " << cfgFilePath_m << endl;
#endif
  loadState();
}


bool Rompb::extensionIsValid(string ext)
{
  unsigned int i = 0;
  for(i =0; i < extensions_vsm.size(); i++)
  {
	 if( extensions_vsm[i] == ext)
	 {
		 return true;
	 }
  }

  return false;
}


bool Rompb::isADir(string dpath)
{
  struct stat sb;

  if (stat( dpath.c_str(), &sb) == -1)
  {
	perror("stat");
	return false;
  }

  switch (sb.st_mode & S_IFMT)
  {
	    case S_IFBLK:  printf("block device\n");
	                   break;

	    case S_IFCHR:  printf("character device\n");
	                   break;

	    case S_IFDIR:  printf("directory\n");
	                   return true;
	                   break;

	    case S_IFIFO:  printf("FIFO/pipe\n");               break;
	    case S_IFLNK:  printf("symlink\n");                 break;
	    case S_IFREG:  printf("regular file\n");            break;
	    case S_IFSOCK: printf("socket\n");                  break;
	    default:       printf("unknown?\n");                break;
  }

  return false;
}

//*********************************************************
//
//*********************************************************
bool Rompb::pathExists(string dpath)
{
	DIR *dp =0;

	bool is_a_directory = isADir( dpath.c_str() );

	if( !is_a_directory)
	{
	   return false;
	}

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

void Rompb::setRomPath(string dpath)
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
vector<string> Rompb::getRomList( void )
{

  DIR* dirp;
  struct dirent* direntp;

  if(activeRomPath_m == "")
  {
	activeRomPath_m = "./";
  }

  activeRomList_vsm.clear();

  if( !pathExists( activeRomPath_m) )
  {
	cout << "\nrompb.cpp->getRomList() directory " << activeRomPath_m << " not found.\n";

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

   	    string tmp = direntp->d_name;

		if( strcmp( direntp->d_name, ".") == 0)
		{
	      continue;
		}

		if( strcmp( direntp->d_name,"..") == 0)
		{
		  continue;
		}
        //cout << tmp;
		string extension = tmp.substr(tmp.find_last_of(".") +1);
        if( extensionIsValid( extension ) == true)
		{
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
 activeRomIndex_m = 0;
 //std::sort( activeRomList_vsm.begin(), activeRomList_vsm.end() );
 sortRoms();
 // showRoms();
 return activeRomList_vsm;
}


void Rompb::showRoms(void)
{
  vector<string>::iterator it = activeRomList_vsm.begin();
  for (it ; it != activeRomList_vsm.end(); ++it)
	 std::cout << "ROM: " << *it << endl;
}


void Rompb::sortRoms(void) {
        int swap;
        string temp;

        do {
                swap = 0;
                for (unsigned int count = 0; count < activeRomList_vsm.size() - 1;
                                count++) {
                        if (activeRomList_vsm.at(count) > activeRomList_vsm.at(count + 1)) {
                                temp = activeRomList_vsm.at(count);
                                activeRomList_vsm.at(count) = activeRomList_vsm.at(count + 1);
                                activeRomList_vsm.at(count + 1) = temp;
                                swap = 1;
                        }
                }
        } while (swap != 0);
}


//*********************************************************
//
//*********************************************************
const char *Rompb::getRomNext(void)
{
    fprintf(stderr,"getRomNext: %d\n", romType_m);
    static char romName[128];

    memset(romName,0,128);

	if( activeRomList_vsm.size() == 0)
	{
	   fprintf(stderr,"getRomNext: error no games in vecList\n");
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

//*********************************************************
//
//*********************************************************
const char *Rompb::getRomPrev(void)
{
    fprintf(stderr,"getRomPrev: %d\n", romType_m);
    static char romName[128];

    memset(romName,0,128);

	if( activeRomList_vsm.size() == 0)
	{
	   fprintf(stderr,"getRomPrev: error no games in vecList\n");
	   return "";
	}

    if(--activeRomIndex_m < 1)
    	activeRomIndex_m = 0;

    if( activeRomList_vsm.size() == 1)
    	activeRomIndex_m = 0;

    activeRom_m = activeRomList_vsm[activeRomIndex_m];
    string baseDir = activeRomPath_m + "/" + activeRomList_vsm[activeRomIndex_m];

    sprintf(romName,"%s",baseDir.c_str());

    fprintf(stderr,"getRomPrev: %d/%d '%s'\n", activeRomIndex_m, activeRomList_vsm.size(), baseDir.c_str() );
    return romName;
}

string Rompb::findRom(const char *srch)
{
  vector<string>::iterator it = activeRomList_vsm.begin();
  int searchLen = strlen(srch);
  unsigned int i = 0;
  for (i = 0; i < activeRomList_vsm.size(); i++)
  {
	 if ( activeRomList_vsm[i].compare(0,searchLen, string(srch)) == 0)
	 {
		 setRomIndex(i-1);
		 return activeRomList_vsm[i];
	 }
  }

  return "";
}



  /*

  it = std::find( activeRomList_vsm.begin(), activeRomList_vsm.end(),  string(srch) );

  if( it == activeRomList_vsm.end() ) {
	  cout << "no matching file name found for '"  << srch << "'" << endl;
	  return "";
  } else {
	  cout << "found: " << *it << endl;
      return *it;
  }
  */


void Rompb::setActiveRomBad()
{
  badRomList_vim.push_back( activeRomIndex_m );
}

bool Rompb::isBadRom()
{
  vector<unsigned int>::iterator i;
  for( i = badRomList_vim.begin(); i != badRomList_vim.end(); ++i)
  {
    if( activeRomIndex_m == *i )
    	return true;
  }
  return false;
}

string Rompb::getInfoStr()
{
  return activeRom_m;
}


//*******************************
//
//*******************************
void Rompb::updateRomList(void)
{
  (void) getRomList();
}


void   Rompb::setRomIndex(unsigned int idx)
{
   if( idx <= activeRomList_vsm.size() )
	  activeRomIndex_m = idx;

   if( activeRomList_vsm.size() )
     activeRom_m = activeRomList_vsm[activeRomIndex_m];
   cout << "index now " << activeRomIndex_m << endl;
}

void Rompb::saveState(void)
{
	  unsigned int i = 0;
	  ofstream cfgFile;
	  cfgFile.open (cfgFilePath_m.c_str());
	  cfgFile << activeRomIndex_m << "\n";
	  cfgFile << activeRom_m << "\n";
	  for( i = 0; i < activeRomList_vsm.size(); i++)
	  {
		 cfgFile << activeRomList_vsm[i] << "\n";
	  }
	  cfgFile.close();
}

void Rompb::loadState(void)
{
     ifstream cfgFile;
     string line;

     cfgFile.open(cfgFilePath_m.c_str());

     // 3
     // spaceinvaders.gba
     // blah1.gba
     // ...

     if (cfgFile.is_open())
      {

    	getline(cfgFile, line);
    	//  activeRomIndex_m
    	getline(cfgFile, line);
    	//  activeRom_m

        while ( cfgFile.good() )
        {
          getline ( cfgFile, line);
          cout << line << endl;
          // activeRomList_vsm.push_back(line);
        }
        cfgFile.close();
      }

}
