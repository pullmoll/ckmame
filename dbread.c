#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>

#include "error.h"
#include "types.h"
#include "dbl.h"
#include "funcs.h"
#include "r.h"

int ngames, sgames;
char **games;

static int add_name(char *s);
static char *extract(char *s, regmatch_t m);
static int game_add(DB* db, struct game *g);
void familymeeting(DB* db, struct game *parent, struct game *child);
int lost(struct game *a);

void game_free(struct game *g, int fullp);

enum regs { r_gamestart, r_name, r_romof, r_rom, r_sampleof, r_sample,
	    r_gameend, r_END };

static char *sregs[] = {
    "^[ \t]*(game|resource)[ \t]*\\(",
    "^[ \t]*name[ \t]*([^ \t\n]*)",
    "^[ \t]*romof[ \t]*([^ \t\n]*)",
    "^[ \t]*rom[ \t]+\\([ \t]+name[ \t]+([^ ]*)[ \t]+size[ \t]+([^ ]*)[ \t]+crc[ \t]+([^ ]*)[ \t]+\\)",
    "^[ \t]*sampleof[ \t]*([^ \t\n]*)",
    "^[ \t]*sample[ \t]*([^ \t\n]*)",
    "^[ \t]*\\)",
};

regex_t regs[r_END];



int
dbread_init(void)
{
    int i, err;
    char b[8192];

    for (i=0; i<r_END; i++) {
        if ((err=regcomp(&regs[i], sregs[i], REG_EXTENDED|REG_ICASE)) != 0) {
	    regerror(err, &regs[i], b, 8192);
	    myerror(ERRDEF, "can't compile regex pattern %d: %s", i, b);
	    return -1;
	}
    }

    sgames = ngames = 0;

    return 0;
}



int
dbread(DB* db, char *fname)
{
    FILE *fin;
    regmatch_t match[6];
    char l[8192], *p;
    int ingame, i;
    /* XXX: every game is only allowed 101 roms */
    struct rom r[1000], s[1000];
    struct game *g;
    struct game *parent;
    char **lostchildren;
    int nlost, lostmax, stillost;
    int nr, ns;

    if ((fin=fopen(fname, "r")) == NULL) {
	myerror(ERRSTR, "can\'t open romlist file `%s'", fname);
	return -1;
    }

    lostmax = 100;
    lostchildren = (char **)xmalloc(lostmax*sizeof(char *));
    
    nlost = nr = ns = ingame = 0;
    while (fgets(l, 8192, fin)) {
	for (i=0; i<r_END; i++)
	    if (regexec(&regs[i], l, 6, match, 0) == 0)
		break;

	switch (i) {
	case r_gamestart:
	    g = (struct game *)xmalloc(sizeof(struct game));
	    g->name = g->cloneof[0] = g->cloneof[1] = g->sampleof = NULL;
	    g->nrom = g->nsample = 0;
	    ingame = 1;
	    nr = ns = 0;
	    break;

	case r_name:
	    g->name = extract(l, match[1]);
	    break;
 
	case r_romof:
	    g->cloneof[0] = extract(l, match[1]);
	    break;

	case r_rom:
	    r[nr].name = extract(l, match[1]);
	    p = extract(l, match[2]);
	    r[nr].size = strtol(p, NULL, 10);
	    free(p);
	    p = extract(l, match[3]);
	    r[nr].crc = strtoul(p, NULL, 16);
	    free(p);
	    r[nr].where = ROM_INZIP;
	    nr++;
	    break;

	case r_sampleof:
	    g->sampleof = extract(l, match[1]);
	    break;

	case r_sample:
	    s[ns].name = extract(l, match[1]);
	    p = extract(l, match[2]);
	    s[ns].size = strtol(p, NULL, 10);
	    free(p);
	    ns++;
	    break;

	case r_gameend:
	    g->nrom = nr;
	    g->rom = r;
	    g->nsample = ns;
	    g->sample = s;

	    if (g->cloneof[0])
		if (strcmp(g->cloneof[0], g->name) == 0) {
		    free(g->cloneof[0]);
		    g->cloneof[0] = NULL;
		}
	    
	    if (g->cloneof[0]) {
		if (((parent=r_game(db, g->cloneof[0]))==NULL) || 
		    lost(parent)) {
		    if (nlost > lostmax - 2) {
			lostmax += 100;
			lostchildren = (char **)xrealloc(lostchildren,
					lostmax*sizeof(char *));
		    }
		    lostchildren[nlost++] = strdup(g->name);
		    if (parent)
			game_free(parent, 1);
		}
		else {
		    familymeeting(db, parent, g);
		    w_game(db, parent);
		    game_free(parent, 1);
		}
		
	    }
	    game_add(db, g);
	    game_free(g, 0);
	    break;
	}
    }

    if (nlost > 0)
	stillost = 1;
    while (stillost > 0) {
	stillost = 0;
	for (i=0; i<nlost; i++) {
	    if (lostchildren[i]==NULL) {
		/* this child is already done */
		continue;
	    }
	    /* get current lost child from database, get parent,
	       look if parent is still lost, if not, do child */
	    if ((g=r_game(db, lostchildren[i]))==NULL) {
		myerror(ERRDEF, "internal database error: "
			"child really lost");
		return 1;
	    }
	    if ((parent=r_game(db, g->cloneof[0]))==NULL) {
		myerror(ERRDEF, "input database not consistent: "
			"parent %s not found", g->cloneof[0]);
		return 1;
	    }
	    if (lost(parent)) {
		stillost = 1;
		if (parent)
		    game_free(parent, 1);
		game_free(g, 1);
		continue;
	    }
	    else {
		/* parent found */
		familymeeting(db, parent, g);
		w_game(db, parent);
		game_free(parent, 1);
		w_game(db, g);
		game_free(g, 1);
		free(lostchildren[i]);
		lostchildren[i] = NULL;
	    }
	}
    }
	 
    qsort(games, ngames, sizeof(char *),
	  (int (*)(const void *, const void *))strpcasecmp);
    w_list(db, "/list", games, ngames);
    
    return 0;
}



void
familymeeting(DB *db, struct game *parent, struct game *child)
{
    struct game *gparent;
    int i, j;
    
    /* tell grandparent of his new grandchild */
    if (parent->cloneof[0]) {
	gparent = r_game(db, parent->cloneof[0]);
	gparent->clone = (char **)xrealloc(gparent->clone,
					   (gparent->nclone+1)*sizeof(char *));
	gparent->clone[gparent->nclone++] = strdup(child->name);
	w_game(db, gparent);
	game_free(gparent, 0);
    }

    /* tell child of his grandfather */
    if (parent->cloneof[0])
	child->cloneof[1] = strdup(parent->cloneof[0]);

    /* tell father of his child */
    parent->clone = (char **)xrealloc(parent->clone,
				      sizeof (char *)*(parent->nclone+1));
    parent->clone[parent->nclone++] = strdup(child->name);

    /* look for roms in parent */
    for (i=0; i<child->nrom; i++)
	for (j=0; j<parent->nrom; j++)
	    if (romcmp(child->rom+i, parent->rom+j)==ROM_OK) {
		child->rom[i].where = parent->rom[j].where + 1;
		break;
	    }

    return;
}


int
lost(struct game *a)
{
    int i;

    if (a->cloneof[0] == NULL)
	return 0;

    for (i=0; i<a->nrom; i++)
	if (a->rom[i].where != ROM_INZIP)
	    return 0;

    return 1;
}
    


static char *
extract(char *s, regmatch_t m)
{
    char *t;
    
    t=(char *)xmalloc(m.rm_eo-m.rm_so+1);
    
    strncpy(t, s+m.rm_so, m.rm_eo-m.rm_so);
    t[m.rm_eo-m.rm_so] = '\0';
    
    return t;
}



static int
game_add(DB* db, struct game *g)
{
    int err;
    
    err = w_game(db, g);

    if (err != 0) {
	myerror(ERRSTR, "can't write game `%s' to db", g->name);
    }
    else
	add_name(g->name);

    return err;
}



static int
add_name(char *s)
{
    if (ngames >= sgames) {
	sgames += 1024;
	if (ngames == 0)
	    games = (char **)xmalloc(sizeof(char *)*sgames);
	else
	    games = (char **)xrealloc(games, sizeof(char *)*sgames);
    }

    games[ngames++] = strdup(s);

    return 0;
}
