// DGen/SDL v1.15+

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "md.h"
#include "decode.h"

// These are my (Joe's) functions added to the md class.

// This takes a comma or whitespace-separated list of Game Genie and/or hex
// codes to patch the ROM with.
int md::patch(const char *list, unsigned int *errors,
	      unsigned int *applied, unsigned int *reverted)
{
  static const char delims[] = " \t\n,";
  char *worklist, *tok;
  struct patch p;
  int ret = 0;

  if (errors != NULL)
    *errors = 0;
  if (applied != NULL)
    *applied = 0;
  if (reverted != NULL)
    *reverted = 0;
  // Copy the given list to a working list so we can strtok it
  worklist = (char*)malloc(strlen(list)+1);
  if (worklist == NULL)
    return -1;
  strcpy(worklist, list);

  for(tok = strtok(worklist, delims); tok; tok = strtok(NULL, delims))
    {
      struct patch_elem *elem = patch_elem;
      struct patch_elem *prev = NULL;
      uint8_t *dest = rom;
      size_t mask = ~(size_t)0;
      int rev = 0;

      // If it's empty, toss it
      if(*tok == '\0') continue;
      // Decode it
      decode(tok, &p);
      // Discard it if it was bad code
      if (((signed)p.addr == -1) || (p.addr >= (size_t)(romlen - 1))) {
		if ((p.addr < 0xff0000) || (p.addr >= 0xffffff)) {
			printf("Bad patch \"%s\"\n", tok);
			if (errors != NULL)
				++(*errors);
			ret = -1;
			continue;
		}
		// This is a RAM patch.
		dest = ram;
		mask = 0xffff;
      }
      // Put it into dest (remember byteswapping)
      while (elem != NULL) {
	if (elem->addr == p.addr) {
	  // Revert a previous patch.
	  p.data = elem->data;
	  if (prev != NULL)
	    prev->next = elem->next;
	  else
	    patch_elem = NULL;
	  free(elem);
	  rev = 1;
	  break;
	}
	prev = elem;
	elem = elem->next;
      }
      if (rev) {
        printf("Reverting patch \"%s\" -> %06X\n", tok, p.addr);
	if (reverted != NULL)
	  ++(*reverted);
      }
      else {
	printf("Patch \"%s\" -> %06X:%04X\n", tok, p.addr, p.data);
	if (applied != NULL)
	  ++(*applied);
	if ((elem = (struct patch_elem *)malloc(sizeof(*elem))) != NULL) {
	  elem->next = patch_elem;
	  elem->addr = p.addr;
	  elem->data = ((dest[((p.addr + 1) & mask)] << 8) |
			(dest[(p.addr & mask)]));
	  patch_elem = elem;
	}
      }
      dest[(p.addr & mask)] = (uint8_t)(p.data & 0xff);
      dest[((p.addr + 1) & mask)] = (uint8_t)(p.data >> 8);
    }
  // Done!
  free(worklist);
  return ret;
}

// Get/put saveram from/to FILE*'s
int md::get_save_ram(FILE *from)
{
	// Pretty simple, just read the saveram raw
	// Return 0 on success
	return !fread((void*)saveram, save_len, 1, from);
}

int md::put_save_ram(FILE *into)
{
	// Just the opposite of the above :)
	// Return 0 on success
	return !fwrite((void*)saveram, save_len, 1, into);
}

// Dave: This is my code, but I thought it belonged here
// Joe: Thanks Dave! No problem ;)
static unsigned short calculate_checksum(unsigned char *rom,int len)
{
  unsigned short checksum=0;
  int i;
  for (i=512;i<=(len-2);i+=2)
  {
    checksum+=(rom[i+1]<<8);
    checksum+=rom[i+0];
  }
  return checksum;
}

void md::fix_rom_checksum()
{
  unsigned short cs; cs=calculate_checksum(rom,romlen);
  if (romlen>=0x190) { rom[0x18f]=cs>>8; rom[0x18e]=cs&255; }
}

