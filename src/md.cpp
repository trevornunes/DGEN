// DGen/SDL v1.17+
// Megadrive C++ module

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_MEMCPY_H
#include "memcpy.h"
#endif
#include "md.h"
#include "system.h"
#include "romload.h"
#include "rc-vars.h"

#define WITH_CZ80 1
#define WITH_MUSA 1

extern FILE *debug_log;

#ifdef WITH_STAR
extern "C" unsigned star_readbyte(unsigned a, unsigned d);
extern "C" unsigned star_readword(unsigned a, unsigned d);
extern "C" unsigned star_writebyte(unsigned a, unsigned d);
extern "C" unsigned star_writeword(unsigned a, unsigned d);

int md::memory_map()
{
  int i=0,j=0;
  int rommax=romlen;

  if (rommax>0xa00000) rommax=0xa00000;
  if (rommax<0) rommax=0;

// FETCH: Set up 2 or 3 FETCH sections
  i=0;
  if (rommax>0)
  { fetch[i].lowaddr=0x000000; fetch[i].highaddr=rommax-1; fetch[i].offset=(unsigned)rom-0x000000; i++; }
  fetch[i].lowaddr=0xff0000; fetch[i].highaddr=0xffffff; fetch[i].offset=(unsigned)ram-  0xff0000; i++;
// Testing
  fetch[i].lowaddr=0xffff0000; fetch[i].highaddr=0xffffffff; fetch[i].offset=(unsigned)ram-0xffff0000; i++;
// Testing 2
  fetch[i].lowaddr=0xff000000; fetch[i].highaddr=0xff000000+rommax-1; fetch[i].offset=(unsigned)rom-0xff000000; i++;
  fetch[i].lowaddr=fetch[i].highaddr=0xffffffff; fetch[i].offset=0; i++;

  if (debug_log!=NULL)
    fprintf (debug_log,"StarScream memory_map has %d fetch sections\n",i);

  i=0; j=0;

#if 0
// Simple version ***************
  readbyte[i].lowaddr=   readword[i].lowaddr=
  writebyte[j].lowaddr=  writeword[j].lowaddr=   0;
  readbyte[i].highaddr=  readword[i].highaddr=
  writebyte[j].highaddr= writeword[j].highaddr=  0xffffffff;

  readbyte[i].memorycall=(void *)star_readbyte;
  readword[i].memorycall=(void *)star_readword;
  writebyte[j].memorycall=(void *)star_writebyte;
  writeword[j].memorycall=(void *)star_writeword;

  readbyte[i].userdata=  readword[i].userdata=
  writebyte[j].userdata= writeword[j].userdata=  NULL;
  i++; j++;
// Simple version end ***************

#else
// Faster version ***************
// IO: Set up 3/4 read sections, and 2/3 write sections
  if (rommax>0)
  {
// Cartridge save RAM memory
    if(save_len) {
      readbyte[i].lowaddr=    readword[i].lowaddr=
      writebyte[j].lowaddr=   writeword[j].lowaddr=   save_start;
      readbyte[i].highaddr=   readword[i].highaddr=
      writebyte[j].highaddr=  writeword[j].highaddr=  save_start+save_len-1;
      readbyte[i].memorycall = star_readbyte;
      readword[j].memorycall = star_readword;
      writebyte[i].memorycall = star_writebyte;
      writeword[j].memorycall = star_writeword;
      readbyte[i].userdata=   readword[i].userdata=
      writebyte[j].userdata=  writeword[j].userdata=  NULL;
      i++; j++;
    }
// Cartridge ROM memory (read only)
    readbyte[i].lowaddr=   readword[i].lowaddr=   0x000000;
    readbyte[i].highaddr=  readword[i].highaddr=  rommax-1;
    readbyte[i].memorycall=readword[i].memorycall=NULL;
    readbyte[i].userdata=  readword[i].userdata=  rom;
    i++;
// misc memory (e.g. aoo and coo) through star_rw
    readbyte[i].lowaddr=   readword[i].lowaddr=
    writebyte[j].lowaddr=  writeword[j].lowaddr=   rommax;
  }
  else
    readbyte[i].lowaddr=   readword[i].lowaddr=
    writebyte[j].lowaddr=  writeword[j].lowaddr=   0;

  readbyte[i].highaddr=  readword[i].highaddr=
  writebyte[j].highaddr= writeword[j].highaddr=  0xfeffff;

  readbyte[i].memorycall = star_readbyte;
  readword[i].memorycall = star_readword;
  writebyte[j].memorycall = star_writebyte;
  writeword[j].memorycall = star_writeword;

  readbyte[i].userdata=  readword[i].userdata=
  writebyte[j].userdata= writeword[j].userdata=  NULL;
  i++; j++;

// scratch RAM memory
  readbyte[i].lowaddr =   readword[i].lowaddr =
  writebyte[j].lowaddr =  writeword[j].lowaddr =   0xff0000;
  readbyte[i].highaddr=   readword[i].highaddr=
  writebyte[j].highaddr=  writeword[j].highaddr=   0xffffff;
  readbyte[i].memorycall= readword[i].memorycall=
  writebyte[j].memorycall=writeword[j].memorycall= NULL;
  readbyte[i].userdata=  readword[i].userdata =
  writebyte[j].userdata= writeword[j].userdata =   ram;
  i++; j++;
// Faster version end ***************
#endif

// The end
   readbyte[i].lowaddr  =   readword[i].lowaddr  =
  writebyte[j].lowaddr  =  writeword[j].lowaddr  =
   readbyte[i].highaddr =   readword[i].highaddr =
  writebyte[j].highaddr =  writeword[j].highaddr = 0xffffffff;

	readbyte[i].memorycall = 0;
	readword[i].memorycall = 0;
	writebyte[j].memorycall = 0;
	writeword[j].memorycall = 0;
	readbyte[i].userdata = 0;
	readword[i].userdata = 0;
	writebyte[j].userdata = 0;
	writeword[j].userdata = 0;

  i++; j++;

  if (debug_log!=NULL)
    fprintf (debug_log,"StarScream memory_map has %d read sections and %d write sections\n",i,j);

  return 0;
}
#endif

int md::reset()
{
	fprintf(stderr,"md::reset\n");

#ifdef WITH_STAR
	md_set_star(1);
	s68000reset();
	md_set_star(0);
#endif
#ifdef WITH_MUSA
	md_set_musa(1);
	m68k_pulse_reset();
	md_set_musa(0);
#endif
  if (debug_log) fprintf (debug_log,"reset()\n");

    coo_waiting=coo_cmd=aoo3_toggle=aoo5_toggle=aoo3_six=aoo5_six
    =aoo3_six_timeout=aoo5_six_timeout
    =coo4=coo5=0;
  pad[0]=pad[1]=0xf303f; // Untouched pad
  memset(pad_com, 0, sizeof(pad_com));

  // Reset FM registers
  fm_reset();
  dac_init();

  memset(&odo, 0, sizeof(odo));
  ras = 0;

  z80_st_running = 0;
  m68k_st_running = 0;
  z80_reset();
  z80_st_busreq = 1;
  z80_st_reset = 1;
  return 0;
}

#ifdef WITH_MZ80

extern "C" UINT8 mz80_read(UINT32 a, struct MemoryReadByte *unused);
extern "C" void mz80_write(UINT32 a, UINT8 d, struct MemoryWriteByte *unused);
extern "C" UINT16 mz80_ioread(UINT16 a, struct z80PortRead *unused);
extern "C" void mz80_iowrite(UINT16 a, UINT8 d, struct z80PortWrite *unused);

static struct MemoryReadByte mem_read[] = {
	{ 0x0000, 0xffff, mz80_read, NULL },
	{ (UINT32)-1, (UINT32)-1, NULL, NULL }
};

static struct MemoryWriteByte mem_write[] = {
	{ 0x0000, 0xffff, mz80_write, NULL },
	{ (UINT32)-1, (UINT32)-1, NULL, NULL }
};

static struct z80PortRead io_read[] = {
	{ 0x00, 0xff, mz80_ioread, NULL },
	{ (UINT16)-1, (UINT16)-1, NULL, NULL }
};

static struct z80PortWrite io_write[] = {
	{ 0x00, 0xff, mz80_iowrite, NULL },
	{ (UINT16)-1, (UINT16)-1, NULL, NULL }
};

#endif // WITH_MZ80

#ifdef WITH_CZ80

extern "C" uint8_t cz80_memread(void *ctx, uint16_t a);
extern "C" void cz80_memwrite(void *ctx, uint16_t a, uint8_t d);
extern "C" uint8_t cz80_ioread(void *ctx, uint16_t a);
extern "C" void cz80_iowrite(void *ctx, uint16_t a, uint8_t d);

#endif // WITH_CZ80

void md::z80_init()
{
    fprintf(stderr,"md::z80 \n");

#ifdef WITH_MZ80
	md_set_mz80(1);
	mz80init();
	mz80reset();
	// Erase local context with global context.
	mz80GetContext(&z80);
	// Configure callbacks in local context.
	z80.z80Base = z80ram;
	z80.z80MemRead = mem_read;
	z80.z80MemWrite = mem_write;
	z80.z80IoRead = io_read;
	z80.z80IoWrite = io_write;
	// Erase global context with the above.
	mz80SetContext(&z80);
	md_set_mz80(0);
#endif
#ifdef WITH_CZ80
	Cz80_Set_Ctx(&cz80, this);
	Cz80_Set_Fetch(&cz80, 0x0000, 0xffff, (void *)z80ram);
	Cz80_Set_ReadB(&cz80, cz80_memread);
	Cz80_Set_WriteB(&cz80, cz80_memwrite);
	Cz80_Set_INPort(&cz80, cz80_ioread);
	Cz80_Set_OUTPort(&cz80, cz80_iowrite);
	Cz80_Reset(&cz80);
#endif
	z80_st_busreq = 1;
	z80_st_reset = 0;
	z80_bank68k = 0xff8000;
}

void md::z80_reset()
{
	z80_bank68k = 0xff8000;
#ifdef WITH_MZ80
	md_set_mz80(1);
	mz80reset();
	md_set_mz80(0);
#endif
#ifdef WITH_CZ80
	Cz80_Reset(&cz80);
#endif
}

md::md(bool pal, char region):
#ifdef WITH_MUSA
	md_musa_ref(0), md_musa_prev(0),
#endif
#ifdef WITH_STAR
	md_star_ref(0), md_star_prev(0),
#endif
#ifdef WITH_MZ80
	md_mz80_ref(0), md_mz80_prev(0),
#endif
	pal(pal), vdp(*this), region(region)
{
	unsigned int hc;

	if (pal) {
		mclk = PAL_MCLK;
		lines = PAL_LINES;
		vhz = PAL_HZ;
	}
	else {
		mclk = NTSC_MCLK;
		lines = NTSC_LINES;
		vhz = NTSC_HZ;
	}
	clk0 = (mclk / 15);
	clk1 = (mclk / 7);
	// Initialize horizontal counter table (Gens style)
	for (hc = 0; (hc < 512); ++hc) {
		// H32
		hc_table[hc][0] = (((hc * 170) / M68K_CYCLES_PER_LINE) - 0x18);
		// H40
		hc_table[hc][1] = (((hc * 205) / M68K_CYCLES_PER_LINE) - 0x1c);
	}

  romlen=0;
  mem=rom=ram=z80ram=saveram=NULL;
  save_start=save_len=save_prot=save_active=0;

  fm_reset();

  memset(&m68k_state, 0, sizeof(m68k_state));
  memset(&z80_state, 0, sizeof(z80_state));

#ifdef WITH_MUSA
	ctx_musa = calloc(1, m68k_context_size());
	if (ctx_musa == NULL)
		return;
#endif

#ifdef WITH_STAR
  fetch=NULL;
  readbyte=readword=writebyte=writeword=NULL;
  memset(&cpu,0,sizeof(cpu));
#endif

#ifdef WITH_MZ80
  memset(&z80,0,sizeof(z80));
#endif
#ifdef WITH_CZ80
  Cz80_Init(&cz80);
#endif

  memset(&cart_head, 0, sizeof(cart_head));

  memset(romname, 0, sizeof(romname));

  ok=0;

  //  Format of pad is: __SA____ UDLRBC__

  rom=mem=ram=z80ram=NULL;
  mem=(unsigned char *)malloc(0x20000);
	if (mem == NULL)
		goto cleanup;
  memset(mem,0,0x20000);
  ram=   mem+0x00000;
  z80ram=mem+0x10000;

  romlen=0;

#ifdef WITH_STAR
	md_set_star(1);
	if (s68000init() != 0) {
		md_set_star(0);
		printf ("s68000init failed!\n");
		goto cleanup;
	}
	md_set_star(0);

// Dave: Rich said doing point star stuff is done after s68000init
// in Asgard68000, so just in case...
  fetch= new STARSCREAM_PROGRAMREGION[6]; if (!fetch)     return;
  readbyte= new STARSCREAM_DATAREGION[5]; if (!readbyte)  return;
  readword= new STARSCREAM_DATAREGION[5]; if (!readword)  return;
  writebyte=new STARSCREAM_DATAREGION[5]; if (!writebyte) return;
  writeword=new STARSCREAM_DATAREGION[5]; if (!writeword) return;
  memory_map();

  // point star stuff
  cpu.s_fetch     = cpu.u_fetch     =     fetch;
  cpu.s_readbyte  = cpu.u_readbyte  =  readbyte;
  cpu.s_readword  = cpu.u_readword  =  readword;
  cpu.s_writebyte = cpu.u_writebyte = writebyte;
  cpu.s_writeword = cpu.u_writeword = writeword;

	md_set_star(1);
	s68000reset();
	md_set_star(0);
#endif

	// M68K: 0 = none, 1 = StarScream, 2 = Musashi
	switch (dgen_emu_m68k) {
#ifdef WITH_STAR
	case 1:
		cpu_emu = CPU_EMU_STAR;
		break;
#endif
#ifdef WITH_MUSA
	case 2:
		cpu_emu = CPU_EMU_MUSA;
		break;
#endif
	default:
		cpu_emu = CPU_EMU_NONE;
		break;
	}
	// Z80: 0 = none, 1 = CZ80, 2 = MZ80
	switch (dgen_emu_z80) {
#ifdef WITH_MZ80
	case 1:
		z80_core = Z80_CORE_MZ80;
		break;
#endif
#ifdef WITH_CZ80
	case 2:
		z80_core = Z80_CORE_CZ80;
		break;
#endif
	default:
		z80_core = Z80_CORE_NONE;
		break;
	}

#ifdef WITH_MUSA
	md_set_musa(1);
	m68k_pulse_reset();
	md_set_musa(0);
#endif

  z80_init();

  reset(); // reset megadrive

	patch_elem = NULL;

  ok=1;

	return;
cleanup:
#ifdef WITH_MUSA
	free(ctx_musa);
#endif
	free(mem);
	memset(this, 0, sizeof(*this));
}


md::~md()
{
  if (rom!=NULL) unplug();

  free(mem);
  rom=mem=ram=z80ram=NULL;

#ifdef WITH_MUSA
	free(ctx_musa);
#endif
#ifdef WITH_STAR
  if (fetch)     delete[] fetch;
  if (readbyte)  delete[] readbyte;
  if (readword)  delete[] readword;
  if (writebyte) delete[] writebyte;
  if (writeword) delete[] writeword;
#endif

	while (patch_elem != NULL) {
		struct patch_elem *next = patch_elem->next;

		free(patch_elem);
		patch_elem = next;
	}

  ok=0;
}

// Byteswaps memory
int byteswap_memory(unsigned char *start,int len)
{ int i; unsigned char tmp;
  for (i=0;i<len;i+=2)
  { tmp=start[i+0]; start[i+0]=start[i+1]; start[i+1]=tmp; }
  return 0;
}

int md::plug_in(unsigned char *cart,int len)
{
  // Plug in the cartridge specified by the uchar *
  // NB - The megadrive will free() it if unplug() is called, or it exits
  // So it must be a single piece of malloced data
  if (cart==NULL) return 1; if (len<=0) return 1;
  byteswap_memory(cart,len); // for starscream
  romlen=len;
  rom=cart;
  // Get saveram start, length (remember byteswapping)
  // First check magic, if there is saveram
  if(rom[0x1b1] == 'R' && rom[0x1b0] == 'A')
    {
      save_start = rom[0x1b5] << 24 | rom[0x1b4] << 16 |
                   rom[0x1b7] << 8  | rom[0x1b6];
      save_len = rom[0x1b9] << 24 | rom[0x1b8] << 16 |
                 rom[0x1bb] << 8  | rom[0x1ba];
      // Make sure start is even, end is odd, for alignment
// A ROM that I came across had the start and end bytes of
// the save ram the same and wouldn't work.  Fix this as seen
// fit, I know it could probably use some work. [PKH]
      if(save_start != save_len) {
        if(save_start & 1) --save_start;
        if(!(save_len & 1)) ++save_len;
        save_len -= (save_start - 1);
        saveram = (unsigned char*)malloc(save_len);
	// If save RAM does not overlap main ROM, set it active by default since
	// a few games can't manage to properly switch it on/off.
	if(save_start >= (unsigned int)romlen)
	  save_active = 1;
      }
      else {
        save_start = save_len = 0;
        saveram = NULL;
      }
    }
  else
    {
      save_start = save_len = 0;
      saveram = NULL;
    }
#ifdef WITH_STAR
  memory_map(); // Update memory map to include this cartridge
#endif
  reset(); // Reset megadrive
  return 0;
}

int md::unplug()
{
  if (rom==NULL) return 1; if (romlen<=0) return 1;
  unload_rom(rom);
  free(saveram);
  romlen = save_start = save_len = 0;
#ifdef WITH_STAR
  memory_map(); // Update memory map to include no rom
#endif
  memset(romname, 0, sizeof(romname));
  reset();

  return 0;
}

int md::load(char *name)
{
	uint8_t *temp;
	size_t size;
	char *b_name;

	  if(!name)
		  return 1;
    b_name = name;

	if ((name == NULL) ||
	    ((b_name = dgen_basename(name)) == NULL))
		return 1;

	fprintf(stderr,"md::load %s\n", name);
	temp = load_rom(&size, name);

	if (temp == NULL) {
		fprintf(stderr,"md::load load_rom() failed\n");
		return 1;
	}

	// Register name
	romname[0] = '\0';
	if ((b_name[0] != '\0')) {
		unsigned int i;

		snprintf(romname, sizeof(romname), "%s", b_name);
		for (i = 0; (romname[i] != '\0'); ++i)
			if (romname[i] == '.') {
				memset(&(romname[i]), 0,
				       (sizeof(romname) - i));
				break;
			}
	}
	if (romname[0] == '\0')
		snprintf(romname, sizeof(romname), "%s", "unknown");

  // Fill the header with ROM info (god this is ugly)
  memcpy((void*)cart_head.system_name,  (void*)(temp + 0x100), 0x10);
  memcpy((void*)cart_head.copyright,    (void*)(temp + 0x110), 0x10);
  memcpy((void*)cart_head.domestic_name,(void*)(temp + 0x120), 0x30);
  memcpy((void*)cart_head.overseas_name,(void*)(temp + 0x150), 0x30);
  memcpy((void*)cart_head.product_no,   (void*)(temp + 0x180), 0x0e);
  cart_head.checksum = temp[0x18e]<<8 | temp[0x18f]; // ugly, but endian-neutral
  memcpy((void*)cart_head.control_data, (void*)(temp + 0x190), 0x10);
  cart_head.rom_start  = temp[0x1a0]<<24 | temp[0x1a1]<<16 | temp[0x1a2]<<8 | temp[0x1a3];
  cart_head.rom_end    = temp[0x1a4]<<24 | temp[0x1a5]<<16 | temp[0x1a6]<<8 | temp[0x1a7];
  cart_head.ram_start  = temp[0x1a8]<<24 | temp[0x1a9]<<16 | temp[0x1aa]<<8 | temp[0x1ab];
  cart_head.ram_end    = temp[0x1ac]<<24 | temp[0x1ad]<<16 | temp[0x1ae]<<8 | temp[0x1af];
  cart_head.save_magic = temp[0x1b0]<<8 | temp[0x1b1];
  cart_head.save_flags = temp[0x1b2]<<8 | temp[0x1b3];
  cart_head.save_start = temp[0x1b4]<<24 | temp[0x1b5]<<16 | temp[0x1b6]<<8 | temp[0x1b7];
  cart_head.save_end   = temp[0x1b8]<<24 | temp[0x1b9]<<16 | temp[0x1ba]<<8 | temp[0x1bb];
  memcpy((void*)cart_head.memo,       (void*)(temp + 0x1c8), 0x28);
  memcpy((void*)cart_head.countries,  (void*)(temp + 0x1f0), 0x10);

	// Plug it into the memory map
	plug_in(temp, size); // md then deallocates it when it's done

	return 0;
}

void md::cycle_z80()
{
	z80_state_dump();
	z80_core = (enum z80_core)((z80_core + 1) % Z80_CORE_TOTAL);
	z80_state_restore();
}

void md::cycle_cpu()
{
	m68k_state_dump();
	cpu_emu = (enum cpu_emu)((cpu_emu + 1) % CPU_EMU_TOTAL);
	m68k_state_restore();
}

int md::z80dump()
{
  FILE *hand;
  hand = dgen_fopen(NULL, "dgz80ram", DGEN_WRITE);
  if (hand!=NULL)
  { fwrite(z80ram,1,0x10000,hand); fclose(hand); }
  return 0;
}
