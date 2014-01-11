#ifndef _ROM_H_
#define _ROM_H_

/* Rom is a class that provides file path setting and rom selection, iteration, cycling
 * It saves repeating the same type of code on multiple emulators.
 */
using namespace std;
#include <vector>
#include <string>
#include <cstdio>



class Rompb {

public:

typedef enum {
		rom_nes_c = 0,
		rom_gb_c,
		rom_gbc_c,
		rom_gba_c,
		rom_pce_c,
		rom_smd_c
}rom_type_t;

Rompb();

Rompb(rom_type_t rtype);

bool           pathExists(string dpath);
bool           isADir(string dpath);

vector<string> getRomList(void);
const char    *getRomNext(void);  // increment up the list
const char    *getRomPrev(void);  // decrement down the list
void           updateRomList(void);
void           sort(void);

void           setRomPath(string path);
string         getRomPath()       { return activeRomPath_m; };
string         getActiveRomName() { return activeRom_m; };
void           setActiveRomBad(void);
string         getInfoStr();
int            romCount() { return activeRomList_vsm.size(); };
bool           isBadRom();
void           setRomIndex(unsigned int idx);
bool           extensionIsValid(string ext);
void           saveState(void);
void           loadState(void);
string         findRom(const char *srch);
void           showRoms(void);
void           sortRoms(void);


private:

 string                  activeRomPath_m;
 string                  cfgFilePath_m;
 vector<string>          activeRomList_vsm;
 vector<string>          extensions_vsm;
 vector<unsigned int>    badRomList_vim;
 string                  activeRom_m;
 rom_type_t              romType_m;
 unsigned int            activeRomIndex_m;
};




#endif /* _ROM_H_ */
