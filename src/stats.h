#ifndef _HAD_STATS_H
#define _HAD_STATS_H

/*
 stats.h -- store stats of the ROM set
 Copyright (C) 2018 Dieter Baron and Thomas Klausner

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

#include <stdint.h>
#include <stdio.h>

#include "disk.h"
#include "file.h"
#include "types.h"

struct stats_files {
    uint64_t files_total;
    uint64_t files_good;
    uint64_t bytes_total;
    uint64_t bytes_good;
};

typedef struct stats_files stats_files_t;

struct stats {
    uint64_t games_total;
    uint64_t games_good;
    uint64_t games_partial;
    stats_files_t files[TYPE_MAX];
};

typedef struct stats stats_t;

void stats_add_disk(stats_t *stats, const disk_t *disk, quality_t status);
void stats_add_game(stats_t *stats, game_status_t status);
void stats_add_rom(stats_t *stats, int type, const file_t *rom, quality_t status);
void stats_free(stats_t *stats);
stats_t *stats_new();
void stats_print(stats_t *stats, FILE *f, bool total_only);

#endif /* _HAD_STATS_H */
