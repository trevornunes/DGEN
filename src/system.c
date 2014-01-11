#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#ifndef __MINGW32__
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <unistd.h>
#else
#include <io.h>
#include <shlobj.h>
#endif
#ifdef WITH_LIBARCHIVE
#include <archive.h>
#endif
#include "system.h"

#ifdef __MINGW32__
#define mkdir(a, b) mkdir(a)
#endif

static const char *fopen_mode(unsigned int mode)
{
	const char *fmode;

	if (mode & DGEN_APPEND)
		fmode = "ab";
	else if (mode & DGEN_WRITE)
		fmode = "w+b";
	else if (mode & DGEN_READ)
		fmode = "rb";
	else
		fmode = NULL;
	return fmode;
}

/*
  Open a file relative to user's DGen directory and create the directory
  hierarchy if necessary, unless the file name is already relative to
  something or found in the current directory if mode contains DGEN_CURRENT.
*/

FILE *dgen_fopen(const char *dir, const char *file, unsigned int mode)
{
	size_t size;
	size_t space;
	const char *fmode = fopen_mode(mode);


#ifndef __MINGW32__
	struct passwd *pwd = getpwuid(geteuid());
#endif
	char path[PATH_MAX];

	if ((file == NULL) || (file[0] == '\0') || (fmode == NULL))
	{
		fprintf(stderr,"dgen_fopen: error opening file ... \n");
		goto error;
	}
		/*
	  Try to open the file in the current directory if DGEN_CURRENT
	  is specified.
	*/
	if (mode & DGEN_CURRENT) {
		FILE *fd = fopen(file, fmode);

		if (fd != NULL)
		{
			return fd;
		}
	}
#ifndef __MINGW32__
	if ((pwd == NULL) || (pwd->pw_dir == NULL))
	{
	    fprintf(stderr,"path error ..\n");
		goto error;
	}
	strncpy(path, pwd->pw_dir, sizeof(path));
#else /* __MINGW32__ */
	if (SHGetFolderPath(NULL, (CSIDL_APPDATA | CSIDL_FLAG_CREATE),
			    0, 0, path) != S_OK)
		goto error;
#endif /* __MINGW32__ */
	path[(sizeof(path) - 1)] = '\0';
	size = strlen(path);
	space = (sizeof(path) - size);
	size += (size_t)snprintf(&path[size], space,
				 DGEN_DIRSEP DGEN_BASEDIR DGEN_DIRSEP);
	if (size >= sizeof(path))
		goto error;
	space = (sizeof(path) - size);

	if (mode & (DGEN_WRITE | DGEN_APPEND))
		mkdir(path, 0777);
	if (dir != NULL) {
		size += (size_t)snprintf(&path[size], space,
					 "%s" DGEN_DIRSEP, dir);
		if (size >= sizeof(path))
			goto error;
		space = (sizeof(path) - size);
		if (mode & (DGEN_WRITE | DGEN_APPEND))
			mkdir(path, 0777);
	}
	size += (size_t)snprintf(&path[size], space, "%s", file);
	if (size >= sizeof(path))
		goto error;
	file = path;
	fprintf(stderr,"dgen_fopen: file='%s' mode='%s'\n", file, fmode);
	return fopen(file, fmode);
error:
	errno = EACCES;
	return NULL;
}

/*
  Return the base name in path, like basename() but without allocating anything
  nor modifying the argument.
*/

char *dgen_basename(char *path)
{
	char *tmp;

	while ((tmp = strpbrk(path, DGEN_DIRSEP DGEN_DIRSEP_ALT)) != NULL)
		path = (tmp + 1);
	fprintf(stderr,"dgen_basename: %s\n",path);
	return path;
}

#define CHUNK_SIZE BUFSIZ

struct chunk {
	size_t size;
	struct chunk *next;
	struct chunk *prev;
	uint8_t data[];
};

/*
  Unload pointer returned by load().
*/

void unload(uint8_t *data)
{
	struct chunk *chunk = ((struct chunk *)data - 1);

	assert(chunk->next == chunk);
	assert(chunk->prev == chunk);
	free(chunk);
}

#ifdef HAVE_FTELLO
#define FTELL(f) ftello(f)
#define FSEEK(f, o, w) fseeko((f), (o), (w))
#define FOFFT off_t
#else
#define FTELL(f) ftell(f)
#define FSEEK(f, o, w) fseek((f), (o), (w))
#define FOFFT long
#endif

/*
  Return the file size from the current file offset.
*/

static size_t load_size(FILE *file)
{
	FOFFT old = FTELL(file);
	FOFFT pos;
	size_t ret = 0;

	if ((old == (FOFFT)-1) ||
	    (FSEEK(file, 0, SEEK_END) == -1))
		return 0;
	if (((pos = FTELL(file)) != (FOFFT)-1) && (pos >= old))
		ret = (size_t)(pos - old);
	FSEEK(file, old, SEEK_SET);
	return ret;
}

/*
  Allocate a buffer and stuff the file inside using transparent decompression
  if libarchive is available. If file_size is non-NULL, store the final size
  there. If max_size is nonzero, refuse to load anything bigger.
  In case the returned value is NULL, errno should contain the error.
*/

uint8_t *load(size_t *file_size, FILE *file, size_t max_size)
{
	size_t pos;
	size_t size = 0;
	struct chunk *chunk;
	struct chunk head = { 0, &head, &head };
	size_t chunk_size = load_size(file);
	int error = 0;
#ifdef WITH_LIBARCHIVE
	struct archive *archive;
	struct archive_entry *archive_entry;

	archive = archive_read_new();
	if (archive == NULL) {
		error = ENOMEM;
		goto error;
	}


	archive_read_support_compression_all(archive);
	archive_read_support_format_raw(archive);
	if ((archive_read_open_FILE(archive, file) != ARCHIVE_OK) ||
	    (archive_read_next_header(archive,
				      &archive_entry) != ARCHIVE_OK)) {
		error = EIO;
		goto error;
	}
#endif
	if (chunk_size == 0)
		chunk_size = CHUNK_SIZE;
	else if ((max_size != 0) && (chunk_size > max_size))
		chunk_size = max_size;
	while (1) {
		pos = 0;
		chunk = malloc(sizeof(*chunk) + chunk_size);
		if (chunk == NULL) {
			error = errno;
			goto error;
		}
		chunk->size = chunk_size;
		chunk->next = &head;
		chunk->prev = head.prev;
		chunk->prev->next = chunk;
		head.prev = chunk;
		do {
			size_t i;
#ifdef WITH_LIBARCHIVE
			ssize_t j;

			j = archive_read_data(archive, &chunk->data[pos],
					      (chunk->size - pos));
			/*
			  Don't bother with ARCHIVE_WARN and ARCHIVE_RETRY,
			  consider any negative value an error.
			*/
			if (j < 0) {
				error = EIO;
				goto error;
			}
			i = (size_t)j;
#else
			i = fread(&chunk->data[pos], 1, (chunk->size - pos),
				  file);
#endif
			if (i == 0) {
				chunk->size = pos;
#ifndef WITH_LIBARCHIVE
				if (ferror(file)) {
					error = EIO;
					goto error;
				}
				assert(feof(file));
#endif
				goto process;
			}
			pos += i;
			size += i;
			if ((max_size != 0) && (size > max_size)) {
				error = EFBIG;
				goto error;
			}
		}
		while (pos != chunk->size);
		chunk_size = CHUNK_SIZE;
	}
process:
#ifdef WITH_LIBARCHIVE
	archive_read_finish(archive);
#endif
	chunk = realloc(head.next, (sizeof(*chunk) + size));
	if (chunk == NULL) {
		error = errno;
		goto error;
	}
	chunk->next->prev = chunk;
	head.next = chunk;
	pos = chunk->size;
	chunk->size = size;
	chunk = chunk->next;
	while (chunk != &head) {
		struct chunk *next = chunk->next;

		memcpy(&head.next->data[pos], chunk->data, chunk->size);
		pos += chunk->size;
		chunk->next->prev = chunk->prev;
		chunk->prev->next = chunk->next;
		free(chunk);
		chunk = next;
	}
	chunk = head.next;
	chunk->prev = chunk;
	chunk->next = chunk;
	if (file_size != NULL)
		*file_size = chunk->size;
	return chunk->data;
error:
#ifdef WITH_LIBARCHIVE
	if (archive != NULL)
		archive_read_finish(archive);
#endif
	chunk = head.next;
	while (chunk != &head) {
		struct chunk *next = chunk->next;

		free(chunk);
		chunk = next;
	}
	errno = error;
	return NULL;
}
