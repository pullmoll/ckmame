#ifndef HAD_ARCHIVE_H
#define HAD_ARCHIVE_H

/*
  $NiH$

  rom.h -- information about one rom
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



#include <zip.h>

#include "array.h"
#include "rom.h"

struct archive {
    char *name;
    array_t *files;
    struct zip *za;
};

typedef struct archive archive_t;



#define archive_file(a, i)	((rom_t *)array_get(archive_files(a), (i)))
#define archive_files(a)	((a)->files)
#define archive_last_file(a)	(archive_file((a), archive_num_files(a)-1))
#define archive_name(a)		((a)->name)
#define archive_num_files(a)	(array_length(archive_files(a)))
#define archive_zip(a)		((a)->za)

int archive_ensure_zip(archive_t *, int);
int archive_find_offset(archive_t *, int, int, const hashes_t *);
int archive_free(archive_t *);
archive_t *archive_new(const char *, filetype_t, const char *);

#endif /* archive.h */