#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "types.h"
#include "dbl.h"
#include "error.h"
#include "funcs.h"

extern char *prg;

static int match(struct game *game, struct zip *zip, int zno, struct match *m);
static int add_match(struct match *m, enum where where, int zno, int fno,
		     enum state st);
int matchcmp(struct match *m1, struct match *m2);

void warn_game(char *name);
void warn_rom(struct rom *rom, char *fmt, ...);
void warn_file(struct rom *r, char *fmt, ...);

static char *zname[] = {
    "zip",
    "cloneof",
    "grand-cloneof"
};



struct match *
check_game(struct game *game, struct zip **zip, int pno, int gpno)
{
    int i, zno[3];
    struct match *m, *mm;

    m = (struct match *)xmalloc(sizeof(struct match)*game->nrom);

    for (i=0; i<game->nrom; i++) {
	m[i].quality = ROM_UNKNOWN;
	m[i].next = NULL;
    }

    for (i=0; i<3; i++) {
	if (zip[i] && zip[i]->name) {
	    match(game, zip[i], i, m);
	    zno[i] = zip[i]->nrom;
	}
	else
	    zno[i] = 0;
    }

    marry(m, game->nrom, zno);

    /* update zip structures */
    zno[0] = 0;
    zno[1] = pno;
    zno[2] = gpno;

    for (i=0; i<game->nrom; i++) {
	if (m[i].quality > ROM_UNKNOWN
	    && zip[m[i].zno]->rom[m[i].fno].state < ROM_TAKEN) {
	    zip[m[i].zno]->rom[m[i].fno].state = ROM_TAKEN;
	    zip[m[i].zno]->rom[m[i].fno].where = zno[m[i].zno];
	}
	for (mm=m->next; mm; mm=mm->next)
	    if (mm->quality > ROM_UNKNOWN
		&& mm->quality > zip[mm->zno]->rom[mm->fno].state) {
		zip[mm->zno]->rom[mm->fno].state = mm->quality;
		zip[mm->zno]->rom[mm->fno].where = zno[mm->zno];
	    }
    }
    
    return m;
}



static int
match(struct game *game, struct zip *zip, int zno, struct match *m)
{
    int i, j;
    enum state st;
    
    for (i=0; i<game->nrom; i++) {
	for (j=0; j<zip->nrom; j++) {
	    st = romcmp(zip->rom+j, game->rom+i);
	    if ((st == ROM_LONG
		 && (makencrc(zip->name, zip->rom[j].name, game->rom[i].size)
		     == game->rom[i].crc)))
		st = ROM_LONGOK;
	    
	    if (st != ROM_UNKNOWN)
		add_match(m+i, game->rom[i].where, zno, j, st);
	}
    }

    return 0;
}



static int
add_match(struct match *m, enum where where, int zno, int fno, enum state st)
{
    struct match *p, *q;

    p = (struct match *)xmalloc(sizeof(struct match));
    p->where = where;
    p->zno = zno;
    p->fno = fno;
    p->quality = st;

    for (q=m; q->next; q=q->next)
	if (matchcmp(q->next, p) < 0)
	    break;

    p->next = q->next;
    q->next = p;

    return 0;
}



int
matchcmp(struct match *m1, struct match *m2)
{
    if (m1->quality == m2->quality) {
	if (m1->where == m1->zno) {
	    if (m2->where == m2->zno)
		return 0;
	    else
		return 1;
	}
	else {
	    if (m2->where == m2->zno)
		return -1;
	    else
		return 0;
	}
    }
    else
	return m1->quality - m2->quality;
}



void
diagnostics(struct game *game, struct match *m, struct zip **zip)
{
    int i, a;
    warn_game(game->name);

    /* analyze result: roms */
    a = 0;
    for (i=0; i<game->nrom; i++) {
	if ((game->rom[i].where == ROM_INZIP)
	    && (m[i].quality >= ROM_NAMERR)) {
	    a = 1;
	    break;
	}
    }

    if (!a && (output_options & WARN_MISSING)) {
	warn_rom(NULL, "not a single rom found");
    }
    else {
        for (i=0; i<game->nrom; i++) {
	    switch (m[i].quality) {
	    case ROM_UNKNOWN:
		if (output_options & WARN_MISSING)
		    warn_rom(game->rom+i, "missing");
		break;
		
	    case ROM_SHORT:
		if (output_options & WARN_SHORT)
		    warn_rom(game->rom+i, "short (%d)",
			     zip[m[i].zno]->rom[m[i].fno].size);
		break;
		
	    case ROM_LONG:
		if (output_options & WARN_LONG)
		    warn_rom(game->rom+i,
			     "too long, truncating won't help (%d)",
			     zip[m[i].zno]->rom[m[i].fno].size);
		break;
		
	    case ROM_CRCERR:
		if (output_options & WARN_WRONG_CRC)
		    warn_rom(game->rom+i, "wrong crc (%0.8x)",
			     zip[m[i].zno]->rom[m[i].fno].crc);
		break;
		
	    case ROM_NAMERR:
		if (output_options & WARN_WRONG_NAME)
		    warn_rom(game->rom+i, "wrong name (%s)",
			     zip[m[i].zno]->rom[m[i].fno].name);
		break;
		
	    case ROM_LONGOK:
		if (output_options & WARN_LONGOK)
		    warn_rom(game->rom+i, "too long, truncating fixes (%d)",
			     zip[m[i].zno]->rom[m[i].fno].size);
		break;
	    default:
		if (output_options & WARN_CORRECT)
		    warn_rom(game->rom+i, "correct");
		break;
	    }
	    

	    if (m[i].quality != ROM_UNKNOWN
		&& game->rom[i].where != m[i].zno) {
		if (output_options & WARN_WRONG_ZIP)
		    warn_rom(game->rom+i, "should be in %s, is in %s",
			     zname[game->rom[i].where],
			     zname[m[i].zno]);
	    }
	}
    }

    /* analyze result: files */
    for (i=0; i<zip[0]->nrom; i++) {
	if ((zip[0]->rom[i].state == ROM_UNKNOWN
	    || (zip[0]->rom[i].state < ROM_NAMERR
		&& zip[0]->rom[i].where != 0))
	    && (output_options & WARN_UNKNOWN))
	    warn_file(zip[0]->rom+i, "unknown");
	else if ((zip[0]->rom[i].state < ROM_TAKEN)
		 && (output_options & WARN_NOT_USED))
	    warn_file(zip[0]->rom+i, "not used");
	else if ((zip[0]->rom[i].where != 0)
		 && (output_options & WARN_USED))
	    warn_file(zip[0]->rom+i, "used in clone %s",
		      game->clone[zip[0]->rom[i].where-1]);
    }
}



static char *gname;
static int gnamedone;

void
warn_game(char *name)
{
    gname = name;
    gnamedone = 0;
}



void
warn_rom(struct rom *r, char *fmt, ...)
{
    va_list va;

    if (gnamedone == 0) {
	printf("In game %s:\n", gname);
	gnamedone = 1;
    }

    if (r)
	printf("rom  %-12s  size %6ld  crc %.8lx: ",
	       r->name, r->size, r->crc);
    else
	printf("game %-39s: ", gname);
    
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);

    putc('\n', stdout);

    return;
}



void
warn_file(struct rom *r, char *fmt, ...)
{
    va_list va;

    if (gnamedone == 0) {
	printf("In game %s:\n", gname);
	gnamedone = 1;
    }

    printf("file %-12s  size %6ld  crc %.8lx: ",
	   r->name, r->size, r->crc);
    
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);

    putc('\n', stdout);

    return;
}



void
match_free(struct match *m, int n)
{
    struct match *mm;
    int i;

    if (m == NULL)
	return;
    
    for (i=0; i<n; i++)
	while (m[i].next) {
	    mm = m[i].next;
	    m[i].next = mm->next;
	    free(mm);
	}

    free(m);
}
