#include "zip.h"
#include "error.h"

char *prg;
#define BUFSIZE 65536

int
main(int argc, char *argv[])
{
    int i;
    struct zf *zf;
    struct zf_file *zff1, *zff2;
    char buf1[BUFSIZE], buf2[BUFSIZE];
    
    prg = argv[0];
    
    if (argc != 2) {
	myerror(ERRDEF, "call with one option: the zip-file to destroy"
		"^H^H^H^H^H^H^Htest");
	return 1;
    }

    seterrinfo(NULL, argv[1]);
    if ((zf=zip_open(argv[1], 0))==NULL) {
	myerror(ERRZIPSTR, "can't open file");
	return 1;
    }

    for (i=0; i<zf->nentry; i++)
	printf("%8d %s\n", zf->entry[i].uncomp_size, zf->entry[i].fn);

    zff1= zff_open_index(zf, 1);
    i = zff_read(zff1, buf1, 100);
    if (i < 0)
	fprintf(stderr, "read error: %s\n", zip_err_str[zff1->flags]);
    else {
	buf1[i] = 0;
	printf("read %d bytes: '%s'\n", i, buf1);
    }
    zff2 = zff_open_index(zf, 1);
    i = zff_read(zff2, buf2, 200);
    if (i < 0)
	fprintf(stderr, "read error: %s\n", zip_err_str[zff2->flags]);
    else {
	buf2[i] = 0;
	printf("read %d bytes: '%s'\n", i, buf2);
    }
    i = zff_read(zff1, buf1, 100);
    if (i < 0)
	fprintf(stderr, "read error: %s\n", zip_err_str[zff1->flags]);
    else {
	buf1[i] = 0;
	printf("read %d bytes: '%s'\n", i, buf1);
    }
    zff_close(zff1);
    zff_close(zff2);

    
    if (zip_close(zf)!=0) {
	myerror(ERRZIPSTR, "can't close file");
	return 1;
    }
    
    return 0;
}
