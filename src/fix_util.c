/*
  $NiH: fix_util.c,v 1.3 2006/05/01 21:09:11 dillo Exp $

  util.c -- utility functions needed only by ckmame itself
  Copyright (C) 1999-2006 Dieter Baron and Thomas Klausner

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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "error.h"
#include "funcs.h"
#include "globals.h"
#include "xmalloc.h"



#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif



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
	    myerror(ERRSTR, "mkdir `%s' failed", name);
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






char *
make_garbage_name(const char *name)
{
    const char *s;
    char *t;

    if ((s=strrchr(name, '/')) == NULL)
	s = name;
    else
	s++;

    t = (char *)xmalloc(strlen(name)+strlen("garbage/")+1);

    sprintf(t, "%.*sgarbage/%s", (int)(s-name), name, s);

    return t;
}



char *
make_unique_name(const char *ext, const char *fmt, ...)
{
    char ret[MAXPATHLEN];
    int i, len;
    struct stat st;
    va_list ap;

    va_start(ap, fmt);
    len = vsnprintf(ret, sizeof(ret), fmt, ap);
    va_end(ap);

    /* already used space, "-XXX.", extension, 0 */
    if (len+5+strlen(ext)+1 > sizeof(ret)) {
	return NULL;
    }

    for (i=0; i<1000; i++) {
	sprintf(ret+len, "-%03d.%s", i, ext);

	if (stat(ret, &st) == -1 && errno == ENOENT)
	    return xstrdup(ret);
    }
    
    return NULL;
}



char *
make_needed_name(const rom_t *r)
{
    char crc[HASHES_SIZE_CRC*2+1];

    /* <needed_dir>/<crc>-nnn.zip */

    hash_to_string(crc, HASHES_TYPE_CRC, rom_hashes(r));

    return make_unique_name("zip", "%s/%s", needed_dir, crc);
}



char *
make_needed_name_disk(const disk_t *d)
{
    char md5[HASHES_SIZE_MD5*2+1];

    /* <needed_dir>/<md5>-nnn.zip */

    hash_to_string(md5, HASHES_TYPE_MD5, rom_hashes(d));

    return make_unique_name("chd", "%s/%s", needed_dir, md5);
}



int
move_image_to_garbage(const char *fname)
{
    char *to_name;
    int ret;

    to_name = make_garbage_name(fname);
    ensure_dir(to_name, 1);
    ret = rename_or_move(fname, to_name);
    free(to_name);

    return ret;
}



int
my_remove(const char *name)
{
    if (remove(name) != 0) {
	seterrinfo(name, NULL);
	myerror(ERRFILESTR, "cannot remove");
	return -1;
    }

    return 0;
}



int
rename_or_move(const char *old, const char *new)
{
    if (rename(old, new) < 0) {
	/* XXX: try move */
	seterrinfo(old, NULL);
	myerror(ERRFILESTR, "cannot rename to `%s'", new);
	return -1;
    }

    return 0;
}



void
remove_empty_archive(const char *name)
{
    int idx;

    if (fix_options & FIX_PRINT)
	printf("%s: remove empty archive\n", name);
    if (superfluous) {
	idx = parray_index(superfluous, name, strcmp);
	/* "needed" zip archives are not in list */
	if (idx >= 0)
	    parray_delete(superfluous, idx, free);
    }
}



void
remove_from_superfluous(const char *name)
{
    int idx;

    if (superfluous) {
	idx = parray_index(superfluous, name, strcmp);
	/* "needed" images are not in list */
	if (idx >= 0)
	    parray_delete(superfluous, idx, free);
    }
}



int
save_needed(archive_t *a, int index, int do_save)
{
    char *zip_name, *tmp;
    int ret, zip_index;
    struct zip *zto;
    struct zip_source *source;
    file_location_ext_t *fbh;

    zip_name = archive_name(a);
    zip_index = index;
    ret = 0;
    tmp = NULL;

    if (do_save) {
	tmp = make_needed_name(archive_file(a, index));
	if (tmp == NULL) {
	    myerror(ERRDEF, "cannot create needed file name");
	    ret = -1;
	}
	else if (ensure_dir(tmp, 1) < 0)
	    ret = -1;
	else if ((zto=my_zip_open(tmp, ZIP_CREATE)) == NULL)
	    ret = -1;
	else if ((source=zip_source_zip(zto, archive_zip(a), index,
					0, 0, -1)) == NULL
		 || zip_add(zto, rom_name(archive_file(a, index)),
			    source) < 0) {
	    zip_source_free(source);
	    seterrinfo(rom_name(archive_file(a, index)), tmp);
	    myerror(ERRZIPFILE, "error adding from `%s': %s",
		    archive_name(a), zip_strerror(zto));
	    zip_close(zto);
	    ret = -1;
	}
	else if (zip_close(zto) < 0) {
	    seterrinfo(NULL, tmp);
	    myerror(ERRZIP, "error closing: %s", zip_strerror(zto));
	    zip_unchange_all(zto);
	    zip_close(zto);
	    ret = -1;
	}
	else {
	    zip_name = tmp;
	    zip_index = 0;
	}
    }

    fbh = file_location_ext_new(zip_name, zip_index, ROM_NEEDED);
    map_add(needed_file_map, file_location_default_hashtype(TYPE_ROM),
	    rom_hashes(archive_file(a, index)), fbh);

    free(tmp);
    return ret;
}



int
save_needed_disk(const char *fname, int do_save)
{
    char *tmp;
    const char *name;
    int ret;
    disk_t *d;

    if ((d=disk_new(fname, 0)) == NULL)
	return -1;

    ret = 0;
    tmp = NULL;
    name = fname;

    if (do_save) {
	tmp = make_needed_name_disk(d);
	if (tmp == NULL) {
	    myerror(ERRDEF, "cannot create needed file name");
	    ret = -1;
	}
	else if (ensure_dir(tmp, 1) < 0)
	    ret = -1;
	else if (rename_or_move(fname, tmp) != 0)
	    ret = -1;
	else
	    name = tmp;
    }

    ensure_needed_maps();
    map_add(needed_disk_map, file_location_default_hashtype(TYPE_DISK),
	    disk_hashes(d), file_location_ext_new(name, 0, ROM_NEEDED));

    disk_free(d);
    free(tmp);
    return ret;
}
