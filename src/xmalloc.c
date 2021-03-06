/*
  xmalloc.c -- malloc routines with exit on failure
  Copyright (C) 1999-2014 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
  3. The name of the author may not be used to endorse or promote
     products derived from this software without specific prior
     written permission.

  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "xmalloc.h"

int
xasprintf(char **ret, const char *format, ...) {
    va_list va;
    int retval;

    va_start(va, format);

    if (((retval = vasprintf(ret, format, va)) < 0) && (errno == ENOMEM)) {
	myerror(ERRDEF, "malloc failure");
	exit(1);
    }

    va_end(va);

    return retval;
}


void *
xmalloc(size_t size) {
    void *p;

    if ((p = malloc(size)) == NULL) {
	myerror(ERRDEF, "malloc failure");
	exit(1);
    }

    return p;
}


void *
xmemdup(const void *src, size_t size) {
    void *dst;

    dst = xmalloc(size);
    memcpy(dst, src, size);

    return dst;
}


char *
xstrdup(const char *str) {
    char *p;

    if (str == NULL)
	return NULL;

    if ((p = malloc(strlen(str) + 1)) == NULL) {
	myerror(ERRDEF, "malloc failure");
	exit(1);
    }

    strcpy(p, str);

    return p;
}

void *
xrealloc(void *p, size_t size) {
    if (p == NULL)
	return xmalloc(size);

    if ((p = realloc(p, size)) == NULL) {
	myerror(ERRDEF, "malloc failure");
	exit(1);
    }

    return p;
}
