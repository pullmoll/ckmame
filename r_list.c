/*
  $NiH: r_list.c,v 1.11 2004/02/26 02:26:11 wiz Exp $

  r_list.c -- read list struct from db
  Copyright (C) 1999, 2003, 2004 Dieter Baron and Thomas Klausner

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



/* read list of strings from db */
#include <stdlib.h>
#include <string.h>

#include "dbh.h"
#include "r.h"
#include "xmalloc.h"



int
r_list(DB *db, const char *key, char ***listp)
{
    int n;
    DBT v;
    void *data;

    if (ddb_lookup(db, key, &v) != 0)
	return -1;

    data = v.data;

    n = r__array(&v, r__pstring, (void **)listp, sizeof(char *));

    free(data);

    return n;
}
