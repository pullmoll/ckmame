/*
  $NiH: r_game.c,v 1.19 2004/02/26 02:26:10 wiz Exp $

  r_game.c -- read game struct from db
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



/* read struct game from db */

#include <string.h>
#include <stdlib.h>

#include "types.h"
#include "dbh.h"
#include "xmalloc.h"
#include "r.h"



struct game *
r_game(DB *db, const char *name)
{
    DBT v;
    struct game *game;
    void *data;

    if (ddb_lookup(db, name, &v) != 0)
	return NULL;

    data = v.data;

    game = (struct game *)xmalloc(sizeof(struct game));
    
    game->name = xstrdup(name);
    game->description = r__string(&v);
    game->cloneof[0] = r__string(&v);
    game->cloneof[1] = r__string(&v);
    game->nclone = r__array(&v, r__pstring, (void *)&game->clone,
			    sizeof(char *));
    game->nrom = r__array(&v, r__rom, (void *)&game->rom, sizeof(struct rom));
    game->sampleof[0] = r__string(&v);
    game->sampleof[1] = r__string(&v);
    game->nsclone = r__array(&v, r__pstring, (void *)&game->sclone,
			    sizeof(char *));
    game->nsample = r__array(&v, r__rom, (void *)&game->sample,
			     sizeof(struct rom));
    game->ndisk = r__array(&v, r__disk, (void *)&game->disk,
			   sizeof(struct disk));
    
    free(data);

    return game;
}



void
r__disk(DBT *v, void *vd)
{
    struct disk *d;
    
    d = (struct disk *)vd;

    d->name = r__string(v);
    d->crctypes = r__ushort(v);
    r__mem(v, d->sha1, sizeof(d->sha1));
    r__mem(v, d->md5, sizeof(d->md5));
}



void
r__rom(DBT *v, void *vr)
{
    struct rom *r;
    
    r = (struct rom *)vr;

    r->name = r__string(v);
    r->merge = r__string(v);
    r->naltname = r__array(v, r__pstring, (void *)&r->altname, sizeof(char *));
    r->size = r__ulong(v);
    r->crctypes = r__ushort(v);
    r->crc = r__ulong(v);
    r__mem(v, r->sha1, sizeof(r->sha1));
    r->flags = r__ushort(v);
    r->where = r__ushort(v);
    r->state = 0;
}
