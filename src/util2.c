/*
  $NiH: util2.c,v 1.1.2.3 2005/07/31 11:37:08 dillo Exp $

  util.c -- utility functions needed only by ckmame itself
  Copyright (C) 1999-2005 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include <sys/stat.h>
#include <errno.h>

#include "dir.h"
#include "error.h"
#include "file_by_hash.h"
#include "funcs.h"
#include "globals.h"
#include "hashes.h"
#include "util.h"
#include "xmalloc.h"

#define MAXROMPATH 128
#define DEFAULT_ROMDIR "."

char *needed_dir = "needed";	/* XXX: proper value */
char *rompath[MAXROMPATH] = { NULL };
static int rompath_init = 0;

map_t *disk_file_map = NULL;
map_t *extra_file_map = NULL;
map_t *needed_map = NULL;
delete_list_t *needed_delete_list = NULL;



int
ensure_dir(const char *name, int strip_fname)
{
    const char *p;
    char *dir;
    struct stat st;
    int ret;

    if (strip_fname) {
	p = strrchr(name, '/');
	if (p == NULL)
	    dir = xstrdup(".");
	else {
	    dir = xmalloc(p-name+1);
	    strncpy(dir, name, p-name);
	    dir[p-name] = 0;
	}
	name = dir;
    }

    ret = 0;
    if (stat(name, &st) < 0) {
	if (mkdir(name, 0777) < 0) {
	    myerror(ERRDEF, "mkdir `%s' failed: %s",
		    name, strerror(errno));
	    ret = -1;
	}
    }
    else if (!(st.st_mode & S_IFDIR)) {
	myerror(ERRDEF, "`%s' is not a directory", name);
	ret = -1;
    }

    if (strip_fname)
	free(dir);

    return ret;
}		    



void
ensure_extra_file_map(void)
{
    if (extra_file_map != NULL)
	return;
    
    extra_file_map = map_new();

    /* XXX: fill in */
}



void
ensure_needed_map(void)
{
    dir_t *dir;
    char b[8192];
    archive_t *a;

    if (needed_map != NULL)
	return;
    
    needed_map = map_new();
    needed_delete_list = delete_list_new();

    if ((dir=dir_open(needed_dir)) == NULL)
	return;

    while (dir_next(dir, b, sizeof(b)) != DIR_EOD) {
	/* XXX: handle error */

	if ((a=archive_new(b, TYPE_FULL_PATH, 0)) != NULL) {
	    enter_archive_in_map(needed_map, a);
	    archive_free(a);
	}
    }

    dir_close(dir);
}



void
enter_archive_in_map(map_t *map, const archive_t *a)
{
    int i;

    for (i=0; i<archive_num_files(a); i++)
	map_add(map, file_by_hash_default_hashtype(TYPE_ROM),
		rom_hashes(archive_file(a, i)),
		file_by_hash_new(archive_name(a), i));
}



char *
findfile(const char *name, filetype_t what)
{
    int i;
    char *fn;
    struct stat st;

    if (what == TYPE_FULL_PATH) {
	if (stat(name, &st) == 0)
	    return xstrdup(name);
	else
	    return NULL;
    }

    for (i=0; rompath[i]; i++) {
	fn = make_file_name(what, i, name);
	if (stat(fn, &st) == 0)
	    return fn;
	if (what == TYPE_DISK) {
	    fn[strlen(fn)-4] = '\0';
	    if (stat(fn, &st) == 0)
		return fn;
	}
	free(fn);
    }
    
    return NULL;
}



void
init_rompath(void)
{
    int i, after;
    char *s, *e;

    if (rompath_init)
	return;

    /* skipping components placed via command line options */
    for (i=0; rompath[i]; i++)
	;

    if ((e = getenv("ROMPATH"))) {
	s = xstrdup(e);

	after = 0;
	if (s[0] == ':')
	    rompath[i++] = DEFAULT_ROMDIR;
	else if (s[strlen(s)-1] == ':')
	    after = 1;
	
	for (e=strtok(s, ":"); e; e=strtok(NULL, ":"))
	    rompath[i++] = e;

	if (after)
	    rompath[i++] = DEFAULT_ROMDIR;
    }
    else
	rompath[i++] = DEFAULT_ROMDIR;

    rompath[i] = NULL;

    rompath_init = 1;
}



char *
make_file_name(filetype_t ft, int idx, const char *name)
{
    char *fn, *dir, *ext;
    
    if (rompath_init == 0)
	init_rompath();

    if (ft == TYPE_SAMPLE)
	dir = "samples";
    else
	dir = "roms";

    if (ft == TYPE_DISK)
	ext = "chd";
    else
	ext = "zip";

    fn = xmalloc(strlen(rompath[idx])+strlen(dir)+strlen(name)+7);
    
    sprintf(fn, "%s/%s/%s.%s", rompath[idx], dir, name, ext);

    return fn;
}



char *
make_needed_name(const rom_t *r)
{
    struct stat st;
    int i;
    char *s, crc[HASHES_SIZE_CRC*2+1];

    /* <needed_dir>/<crc>-nnn.zip */

    hash_to_string(crc, HASHES_TYPE_CRC, rom_hashes(r));
    
    s = xmalloc(strlen(needed_dir) + 18);
		
    for (i=0; i<1000; i++) {
	sprintf(s, "%s/%s-%03d.zip", needed_dir, crc, i);

	if (stat(s, &st) == -1 && errno == ENOENT)
	    return s;
    }

    free(s);

    /* XXX: better error handling */
    return NULL;
}



void
print_extra_files(const parray_t *files)
{
    int i;

    if (parray_length(files) == 0)
	return;

    printf("Extra files found:\n");
    
    for (i=0; i<parray_length(files); i++)
	printf("%s\n", (char *)parray_get(files, i));
}



struct zip *
my_zip_open(const char *name, int flags)
{
    struct zip *z;
    char errbuf[80];
    int err;

    z = zip_open(name, flags, &err);
    if (z == NULL)
	myerror(ERRDEF, "error %s zip archive `%s': %s", name,
		(flags & ZIP_CREATE ? "creating" : "opening"),
		zip_error_to_str(errbuf, sizeof(errbuf), err, errno));

    return z;
}
