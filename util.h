#ifndef _HAD_UTIL_H
#define _HAD_UTIL_H

/*
  $NiH: util.h,v 1.8 2002/06/06 09:27:00 dillo Exp $

  util.h -- miscellaneous utility functions
  Copyright (C) 1999 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

typedef int (*cmpfunc)(const void *, const void *);

char *findzip(char *name, int sample);

void init_rompath(void);

void *memdup(const void *mem, int len);
unsigned char *memmem(const unsigned char *big, int biglen, 
		      const unsigned char *little, int littlelen);
int strpcasecmp(char **sp1, char **sp2);

#endif
