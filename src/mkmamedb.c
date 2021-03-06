/*
  mkmamedb.c -- create mamedb
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <zip.h>

#include "compat.h"
#include "dbh_cache.h"
#include "error.h"
#include "funcs.h"
#include "globals.h"
#include "output.h"
#include "parse.h"
#include "romdb.h"
#include "types.h"
#include "xmalloc.h"

char *usage = "Usage: %s [-huV] [-C types] [-F fmt] [-o dbfile] [-x pat] [--detector xml-file] [--no-directory-cache] [--only-files pat] [--prog-description d] [--prog-name name] [--prog-version version] [--skip-files pat] [rominfo-file ...]\n";

char help_head[] = "mkmamedb (" PACKAGE ") by Dieter Baron and"
		   " Thomas Klausner\n\n";

char help[] = "\n"
	      "  -h, --help                      display this help message\n"
	      "  -V, --version                   display version number\n"
	      "  -C, --hash-types types          specify hash types to compute (default: all)\n"
	      "  -F, --format [cm|dat|db|mtree]  specify output format [default: db]\n"
	      "  -o, --output dbfile             write to database dbfile\n"
	      "  -u, --roms-unzipped             ROMs are files on disk, not contained in zip archives\n"
	      "  -x, --exclude pat               exclude games matching shell glob PAT\n"
	      "      --detector xml-file         use header detector\n"
	      "      --no-directory-cache        don't create cache of scanned input directory\n"
	      "      --only-files pat            only use zip members matching shell glob PAT\n"
	      "      --prog-description d        set description of rominfo\n"
	      "      --prog-name name            set name of program rominfo is from\n"
	      "      --prog-version version      set version of program rominfo is from\n"
	      "      --runtest                   output special format for use in ckmame test suite\n"
	      "      --skip-files pat            don't use zip members matching shell glob PAT\n"
	      "\n"
	      "Report bugs to " PACKAGE_BUGREPORT ".\n";

char version_string[] = "mkmamedb (" PACKAGE " " VERSION ")\n"
			"Copyright (C) 1999-2014 Dieter Baron and Thomas Klausner\n" PACKAGE " comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n";

#define OPTIONS "hC:F:o:uVx:"

enum { OPT_DETECTOR = 256, OPT_NO_DIRECTORY_CACHE, OPT_ONLY_FILES, OPT_PROG_DESCRIPTION, OPT_PROG_NAME, OPT_PROG_VERSION, OPT_RUNTEST, OPT_SKIP_FILES };

struct option options[] = {
    {"help", 0, 0, 'h'}, {"version", 0, 0, 'V'}, {"no-directory-cache", 0, 0, OPT_NO_DIRECTORY_CACHE}, {"detector", 1, 0, OPT_DETECTOR}, {"exclude", 1, 0, 'x'}, {"format", 1, 0, 'F'}, {"hash-types", 1, 0, 'C'}, {"output", 1, 0, 'o'}, {"only-files", 1, 0, OPT_ONLY_FILES}, {"prog-description", 1, 0, OPT_PROG_DESCRIPTION}, {"prog-name", 1, 0, OPT_PROG_NAME}, {"prog-version", 1, 0, OPT_PROG_VERSION}, {"roms-unzipped", 0, 0, 'u'}, {"runtest", 0, 0, OPT_RUNTEST}, {"skip-files", 1, 0, OPT_SKIP_FILES}, {NULL, 0, 0, 0},
};

#define DEFAULT_FILES_ONLY "*.dat"

static int process_file(const char *, const parray_t *, const dat_entry_t *, const parray_t *, const parray_t *, output_context_t *);
static int process_stdin(const parray_t *, const dat_entry_t *, output_context_t *);

static int hashtypes;
static bool cache_directory;
static int parser_flags;

int
main(int argc, char **argv) {
    output_context_t *out;
    char *dbname;
    parray_t *exclude;
    parray_t *only_files;
    parray_t *skip_files;
    dat_entry_t dat;
    output_format_t fmt;
    char *detector_name;
    int c, i;
    int flags;
    bool runtest;
    int ret = 0;

    setprogname(argv[0]);

    runtest = false;
    detector = NULL;
    roms_unzipped = 0;
    cache_directory = true;
    flags = 0;
    parser_flags = 0;

    dbname = getenv("MAMEDB");
    if (dbname == NULL)
	dbname = DBH_DEFAULT_DB_NAME;
    dat_entry_init(&dat);
    exclude = NULL;
    only_files = skip_files = NULL;
    fmt = OUTPUT_FMT_DB;
    hashtypes = HASHES_TYPE_CRC | HASHES_TYPE_MD5 | HASHES_TYPE_SHA1;
    detector_name = NULL;

    opterr = 0;
    while ((c = getopt_long(argc, argv, OPTIONS, options, 0)) != EOF) {
	switch (c) {
	case 'h':
	    fputs(help_head, stdout);
	    printf(usage, getprogname());
	    fputs(help, stdout);
	    exit(0);
	case 'V':
	    fputs(version_string, stdout);
	    exit(0);
	case 'C':
	    hashtypes = hash_types_from_str(optarg);
	    if (hashtypes == 0) {
		fprintf(stderr, "%s: illegal hash types '%s'\n", getprogname(), optarg);
		exit(1);
	    }
	    break;
	case 'F':
	    if (strcmp(optarg, "cm") == 0)
		fmt = OUTPUT_FMT_CM;
	    else if (strcmp(optarg, "dat") == 0)
		fmt = OUTPUT_FMT_DATAFILE_XML;
	    else if (strcmp(optarg, "db") == 0)
		fmt = OUTPUT_FMT_DB;
	    else if (strcmp(optarg, "mtree") == 0)
		fmt = OUTPUT_FMT_MTREE;
	    else {
		fprintf(stderr, "%s: unknown output format '%s'\n", getprogname(), optarg);
		exit(1);
	    }
	    break;
	case 'o':
	    dbname = optarg;
	    break;
	case 'u':
	    roms_unzipped = 1;
	    break;
	case 'x':
	    if (exclude == NULL)
		exclude = parray_new();
	    parray_push(exclude, xstrdup(optarg));
	    break;
	case OPT_DETECTOR:
	    detector_name = optarg;
	    break;
	case OPT_NO_DIRECTORY_CACHE:
	    cache_directory = false;
	    break;
	case OPT_ONLY_FILES:
	    if (only_files == NULL)
		only_files = parray_new();
	    parray_push(only_files, xstrdup(optarg));
	    break;
	case OPT_PROG_DESCRIPTION:
	    dat_entry_description(&dat) = xstrdup(optarg);
	    break;
	case OPT_PROG_NAME:
	    dat_entry_name(&dat) = xstrdup(optarg);
	    break;
	case OPT_RUNTEST:
	    runtest = true;
	    break;
	case OPT_PROG_VERSION:
	    dat_entry_version(&dat) = xstrdup(optarg);
	    break;
	case OPT_SKIP_FILES:
	    if (skip_files == NULL)
		skip_files = parray_new();
	    parray_push(skip_files, xstrdup(optarg));
	    break;
	default:
	    fprintf(stderr, usage, getprogname());
	    exit(1);
	}
    }

    if (argc - optind > 1 && dat_entry_name(&dat)) {
	fprintf(stderr,
		"%s: warning: multiple input files specified, \n\t"
		"--prog-name and --prog-version are ignored",
		getprogname());
    }
    if (runtest) {
	fmt = OUTPUT_FMT_MTREE;
	flags |= OUTPUT_FL_EXTENDED;
	parser_flags = PARSER_FL_FULL_ARCHIVE_NAME;
	cache_directory = false;
    }

    if ((out = output_new(fmt, dbname, flags)) == NULL)
	exit(1);

    if (detector_name) {
	seterrinfo(detector_name, NULL);
	detector = detector_parse(detector_name);
	if (detector != NULL)
	    output_detector(out, detector);
    }


    /* TODO: handle errors */
    if (optind == argc)
	process_stdin(exclude, &dat, out);
    else {
	if (only_files == NULL) {
	    only_files = parray_new();
	    parray_push(only_files, xstrdup(DEFAULT_FILES_ONLY));
	}

	for (i = optind; i < argc; i++) {
	    if (process_file(argv[i], exclude, &dat, only_files, skip_files, out) < 0) {
		i = argc;
		ret = -1;
	    }
	}
    }

    if (ret == 0) {
	output_close(out);
    }

    if (detector)
	detector_free(detector);

    if (exclude)
	parray_free(exclude, free);

    if (only_files)
	parray_free(only_files, free);
    if (skip_files)
	parray_free(skip_files, free);
    if (roms_unzipped)
	dbh_cache_close_all();

    return 0;
}


int
process_file(const char *fname, const parray_t *exclude, const dat_entry_t *dat, const parray_t *files_only, const parray_t *files_skip, output_context_t *out) {
    romdb_t *mdb;
    parser_source_t *ps;
    struct zip *za;

    if ((mdb = romdb_open(fname, DBH_READ)) != NULL)
	return export_db(mdb, exclude, dat, out);
    else if (roms_unzipped == 0 && (za = zip_open(fname, 0, NULL)) != NULL) {
	int i;
	const char *name;
	int err;

	err = 0;
	for (i = 0; i < zip_get_num_files(za); i++) {
	    name = zip_get_name(za, i, 0);

	    if (!name_matches(name, files_only) || name_matches(name, files_skip))
		continue;

	    if ((ps = ps_new_zip(fname, za, name)) == NULL) {
		err = -1;
		continue;
	    }
	    if (parse(ps, exclude, dat, out, parser_flags) < 0)
		err = -1;
	}
	zip_close(za);

	return err;
    }
    else {
	struct stat st;

	if (stat(fname, &st) == -1) {
	    myerror(ERRSTR, "can't stat() file '%s'", fname);
	    return -1;
	}
	if ((st.st_mode & S_IFMT) == S_IFDIR) {
	    parser_context_t *ctx;
	    int ret;

	    if (cache_directory)
		archive_register_cache_directory(fname);

	    ctx = parser_context_new(NULL, exclude, dat, out, parser_flags);
	    ret = parse_dir(fname, ctx, hashtypes);
	    parser_context_free(ctx);
	    return ret;
	}

	if (roms_unzipped) {
	    myerror(ERRDEF, "argument '%s' is not a directory", fname);
	    return -1;
	}

	if ((ps = ps_new_file(fname)) == NULL)
	    return -1;

	return parse(ps, exclude, dat, out, parser_flags);
    }
}


int
process_stdin(const parray_t *exclude, const dat_entry_t *dat, output_context_t *out) {
    parser_source_t *ps;

    if ((ps = ps_new_stdin()) == NULL)
	return -1;

    return parse(ps, exclude, dat, out, parser_flags);
}
