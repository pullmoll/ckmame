/* read list of strings from db */

#include "dbl.h"



int
r_list(DB *db, char *key, char ***listp)
{
    int n;
    DBT k, v;
    void *data;

    k.size = strlen(key);
    k.data = xmalloc(k.size);
    strncpy(k.data, key, k.size);

    if (db_lookup(db, k, v) != 0) {
	free(k.data);
	return -1;
    }
    data = v.data;

    n = r__array(v, r__string, listp, sizeof(char *));

    free(k.data);
    free(data);

    return n;
}
