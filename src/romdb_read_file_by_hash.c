/*
  romdb_read_file_by_hash.c -- read file_by_hash information from db
  Copyright (C) 2005-2014 Dieter Baron and Thomas Klausner

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


#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "file_location.h"
#include "romdb.h"
#include "sq_util.h"

array_t *
romdb_read_file_by_hash(romdb_t *db, filetype_t ft, const hashes_t *hash) {
    sqlite3_stmt *stmt;
    array_t *a;
    file_location_t *fl;
    int ret;

    if ((stmt = dbh_get_statement(romdb_dbh(db), dbh_stmt_with_hashes_and_size(DBH_STMT_QUERY_FILE_FBH, hash, 0))) == NULL)
	return NULL;

    if (sqlite3_bind_int(stmt, 1, ft) != SQLITE_OK || sqlite3_bind_int(stmt, 2, STATUS_NODUMP) != SQLITE_OK || sq3_set_hashes(stmt, 3, hash, 0) != SQLITE_OK)
	return NULL;

    a = array_new(sizeof(file_location_t));

    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
	fl = array_grow(a, NULL);
	file_location_name(fl) = sq3_get_string(stmt, 0);
	file_location_index(fl) = sqlite3_column_int(stmt, 1);
    }

    if (ret != SQLITE_DONE) {
	array_free(a, file_location_finalize);
	return NULL;
    }

    return a;
}
