#ifndef _HAD_UTIL_H
#define _HAD_UTIL_H

/*
  $NiH: util.h,v 1.11 2007/04/12 21:09:21 dillo Exp $

  util.h -- miscellaneous utility functions
  Copyright (C) 1999-2007 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

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

#include <stdio.h>
#include <string.h>

#include "types.h"



typedef int (*cmpfunc)(const void *, const void *);

char *bin2hex(char *, const unsigned char *, unsigned int);
char *getline(FILE *);
int hex2bin(unsigned char *, const char *, unsigned int);
const char *mybasename(const char *);
char *mydirname(const char *);
name_type_t name_type(const char *);
int psort(void **, int, int, int (*)(const void *, const void *));

#endif
