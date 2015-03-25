#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

static write_record (char *par, char *val, FILE *f) {
	if (NULL != f) {
		if (NULL == val || strlen (val) < 1) {
			fprintf (f, "%s |\n", par);
		}
		else {
			fprintf (f, "%s | (%s)\n", par, val);
		}
	}
	else {
		printf ("\t");
	}

	if (NULL == val || strlen (val) < 1) {
		printf ("%s |\n", par);
	}
	else {
		printf ("%s | (%s)\n", par, val);
	}
}

static int read_record (int fd, FILE* f) {
	char lenpar;
	char param[256];
	unsigned short lenval;
	char value[65536];
	int x;

	x = read (fd, &lenpar, 1);
	if (1 != x) {
		if (NULL != f) {
			if (errno != 0) {
				printf ("error reading file descriptor %d : %s\n", fd, strerror(errno));
			}
			else {
				printf ("error reading file descriptor %d @ %ld\n", fd, lseek (fd, 0, SEEK_CUR));
			}
		}
		return errno == 0 ? -1 : 1;
	}
	bzero (param, sizeof(param));
	x = read (fd, param, lenpar);
	if (lenpar != x) {
		if (NULL != f) {
			printf ("error reading parameter : %d <> %d : %s\n", x, (int)lenpar, strerror(errno));
		}
		return 2;
	}
	x = read (fd, &lenval, 2);
	if (2 != x) {
		if (NULL != f) {
			printf ("error reading value length : %d <> %d : %s\n", x, (int)lenval, strerror(errno));
		}
		return 3;
	}
	if (0 == lenval) {
		write_record (param, "", f);
		/*
		if (NULL != f) {
			printf ("no value for parameter '%s'\n", param);
		} */
		return 0;
	}
	bzero (value, sizeof(value));
	if (lenval != read (fd, value, lenval)) {
		if (NULL != f) {
			printf ("error reading value of parameter '%s' : %d <> %d : %s\n", param, x, (int)lenval, strerror(errno));
		}
		return 4;
	}
	/*
	if (value[lenval-1] == '\n') {
		value[lenval-1] = 0;
	}*/
	write_record (param, value, f);
	return 0;
}

/**
 * http://www.dd-wrt.com/phpBB2/viewtopic.php?t=33164&highlight=nvram+csv
 */
int main (int ac, char* av[]) {
	char* fisier;
	FILE* outf = NULL;
	int fdi;
	struct stat buf;
	char header[8];
	unsigned short nbrecords = 0;
	int i, j;

	if (ac < 2) {
		printf ("Usage : %s <in file> [<out file>]\n", av[0]);
		exit (1);
	}
	fisier = av[1];
	if ((fdi = open (fisier, O_RDONLY)) == -1) {
		printf ("error opening '%s' : %s\n", fisier, strerror(errno));
		exit (2);
	}
	if (fstat (fdi, &buf)) {
		printf ("error reading status of '%s' : %s\n", fisier, strerror(errno));
		close (fdi);
		exit (2);
	}
	if (buf.st_size < 8) {
		printf ("error reading size of '%s' : %ld bytes\n", fisier, buf.st_size);
		close (fdi);
		exit (3);
	}

	if (ac > 2) {
		outf = fopen (av[2], "w");
		printf ("decoding '%s' in '%s'\n", fisier, av[2]);
		if (NULL == outf) {
			printf ("error creating file '%s'\n", av[2]);
			close (fdi);
			exit (4);
		}
	}

	if (read (fdi, header, sizeof (header)) < sizeof (header)) {
		printf ("error reading header of '%s'\n", fisier);
		close (fdi);
		if (NULL != outf) {
			fclose (outf);
			unlink (av[2]);
		}
		exit (5);
	}
	if (0 != strncmp (header, "DD-WRT", 6)) {
		header[6] = 0;
		printf ("error reading header of '%s' : %s\n", fisier, header);
		close (fdi);
		if (NULL != outf) {
			fclose (outf);
			unlink (av[2]);
		}
		exit (6);
	}
	nbrecords = (unsigned char)header[6]; /* ((unsigned short)header[6]) << 8 | header[7]; */
	if (4 != header[7]) {
		printf ("error reading header of '%s' : %d\n", fisier, (int)header[7]);
		close (fdi);
		if (NULL != outf) {
			fclose (outf);
			unlink (av[2]);
		}
		exit (7);
	}
	if (NULL != outf) {
		printf ("%d saved records in '%s'\n", nbrecords, fisier);
	}
	/*
	for (i = 0; i < nbrecords; i++) {
		int j = read_record (fdi, outf);
		if (j) {
			if (NULL != outf) {
				printf ("read %d records\n", i);
			}

			close (fdi);
			if (j > 0) {
				fclose (outf);
				unlink (av[2]);
			}
			exit ( j > 0 ? 8 : 0);
		}
	} */
	i = 0;
	do {
		j = read_record (fdi, outf);
		if (j) {
			printf ("read %d records\n", i);
		}
		else {
			++i;
		}
	} while (!j);

	if (NULL != outf) {
		fclose (outf);
	}
	close (fdi);
	return 0;
}
