/*
  $NiH: delete_list.c,v 1.1.2.1 2005/07/31 21:13:01 dillo Exp $

  delete_list.h -- list of files to delete
  Copyright (C) 2005 Dieter Baron and Thomas Klausner

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



#include <stdlib.h>
#include <zip.h>

#include "delete_list.h"
#include "error.h"
#include "funcs.h"
#include "xmalloc.h"



static int
my_zip_close(struct zip *z, const char *name)
{
    if (z) {
	if (zip_close(z) < 0) {
	    seterrinfo(NULL, name);
	    myerror(ERRZIP, "cannot delete files: %s", zip_strerror(z));
	    zip_unchange_all(z);
	    zip_close(z);
	    return -1;
	}
    }

    return 0;
}



void
delete_list_free(delete_list_t *dl)
{
    if (dl == NULL)
	return;

    parray_free(dl->array, file_by_hash_free);
    free(dl);
}



int
delete_list_execute(delete_list_t *dl)
{
    int i;
    const char *name;
    const file_by_hash_t *fbh;
    struct zip *z;
    int ret;

    parray_sort_unique(dl->array, file_by_hash_entry_cmp);

    name = NULL;
    z = NULL;
    ret = 0;
    for (i=0; i<delete_list_length(dl); i++) {
	fbh = delete_list_get(dl, i);

	if (file_by_hash_name(fbh) != name) {
	    if (my_zip_close(z, name) == -1)
		ret = -1;

	    name = file_by_hash_name(fbh);
	    if ((z=my_zip_open(name, 0)) == NULL)
		ret = -1;
	}
	if (z) {
	    /* XXX: print message if FIX_PRINT */
	    zip_delete(z, file_by_hash_index(fbh));
	}
    }

    if (my_zip_close(z, name) == -1)
	ret = -1;
    
    return ret;
}



delete_list_t *
delete_list_new(void)
{
    delete_list_t *dl;

    dl = xmalloc(sizeof(*dl));
    dl->array = parray_new();
    dl->mark = 0;

    return dl;
}
