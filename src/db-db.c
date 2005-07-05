/*
  $NiH: db-db.c,v 1.25 2005/07/01 01:35:56 dillo Exp $

  db-db.c -- low level routines for Berkeley db 
  Copyright (C) 1999, 2003, 2004, 2005 Dieter Baron and Thomas Klausner

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



#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "db-db.h"
#include "dbl.h"
#include "xmalloc.h"



DB*
ddb_open(const char *name, int flags)
{
    DB *db;
    HASHINFO hi;

    hi.bsize = 1024;
    hi.ffactor = 8;
    hi.nelem = 1500;
    hi.cachesize = 65536; /* taken from db's hash.h */
    hi.hash = NULL;
    hi.lorder = 0;

    db = dbopen(name, (flags & DDB_WRITE) ? O_RDWR|O_CREAT : O_RDONLY,
		0666, DB_HASH, &hi);

    if (db && ddb_check_version(db, flags) != 0) {
	ddb_close(db);
	return NULL;
    }
    
    return db;
}



int
ddb_close(DB *db)
{
    (db->close)(db);
    return 0;
}



int
ddb_insert_l(DB *db, DBT *key, const DBT *value)
{
    return (db->put)(db, key, value, 0);
}



int
ddb_lookup_l(DB *db, DBT *key, DBT *value)
{
    return (db->get)(db, key, value, 0);
}



const char *
ddb_error_l(void)
{
    return strerror(errno);
}



int
ddb_foreach(DB *db, int (*f)(const DBT *, const DBT *, void *), void *ud)
{
    DBT key, value;
    int ret;

    /* test inside body to catch empty database */
    for (ret=db->seq(db, &key, &value, R_FIRST); ;
	 ret=db->seq(db, &key, &value, R_NEXT)) {
	if (ret != 0)
	    break;
	if ((ret=f(&key, &value, ud)) != 0)
	    break;
    }

    return ret < 0 ? -1 : 0;
}