#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

static void update_header (FILE* f, int nbr) {
	unsigned char c = (unsigned char)nbr;
	fseek (f, 6, SEEK_SET);
	fwrite (&c, 1, 1, f);
}

static void write_header (FILE* f) {
	unsigned char c = 4;
	fprintf (f, "DD-WRT");
	fwrite (&c, 1, 1, f);
	fwrite (&c, 1, 1, f);
}

// http://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way
static char* trim (char *str) {
char *end;

  // Trim leading space
  //while(isspace(*str)) str++;
	while(isblank (*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  //while(end > str && isspace(*end)) end--;
	while(end > str && isblank(*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}

static int read_other_lines (FILE *fin, char* b) {
	char buf[65536], *s;

	while (1) {
		s = (char *)fgets (buf, sizeof (buf), fin);
		if (NULL == s) {
			printf ("error reading file : %s\n", strerror(errno));
			return 1;
		}
		s = index (buf, ')');
		if (NULL != s) { /* end found */
			*s = 0;
			strcpy (b, buf);
			return 0;
		}
		else {
			strcpy (b, buf);
			b+= strlen (buf);
		}
    }
	return 0;
}

/*
 * one byte (binary) - length of parameter name
 * parameter name - ASCII text
 * two bytes (binary) - length of parameter value
 * parameters' value - always ASCII text
 */
static void write_pv (FILE* f, char *p, char* v) {
	unsigned char pl;
	unsigned short vl;

	pl = (unsigned char)strlen (p);
	vl = (unsigned short)strlen (v);
	fwrite (&pl, 1, 1, f);
	fputs (p, f);
	fwrite (&vl, 1, 2, f);
	fputs (v, f);
}

int read_param_value (FILE* fin, char *param, char *val) {
	char buf[1300], *s, *v, *s1;
	char bv[65536];

	s = (char *)fgets (buf, sizeof (buf), fin);
	if (NULL == s) {
		printf ("error reading file : %s\n", strerror(errno));
		return 0;
	}
	/* printf ("ligne = '%s'", s); */
	s = index (buf, '|');
	if (NULL == s) {
		printf ("erreur ligne = '%s'\n", buf);
		return -1;
	}
	v = s + 1;
	*s = 0;
	strcpy (param, trim(buf));
	s = index (v, ')');
	s1 = index (v, '(');
	if (NULL == s) { // multiline
		if (NULL == s1) { /* no value */
			*val = 0;
			return 1;
		}
		strcpy (bv, s1 + 1);
		if (read_other_lines (fin, bv + strlen(bv))) {
			printf ("bad multiline read\n");
			return -2;
		}
		strcpy (val, trim (bv));
	}
	else {
		if (NULL == s1) {
			printf ("bad line read: '%s': '(' but no ')'!\n", buf);
			return -3;
		}
		*s = 0;
		strcpy (val, s1 + 1);
	}

	return 1; /* OK, 1 param + 1 value read */
}

int main (int ac, char* av[]) {
	char* fisier;
	FILE* outf = NULL;
	FILE *fdi = NULL;
	int nbrecords, nbr;

	if (ac < 2) {
		printf ("Usage : %s <in file> [<out file>]\n", av[0]);
		printf (" default out file : <in>.bin\n");
		exit (1);
	}
	fisier = av[1];
	if ((fdi = fopen (fisier, "r")) == NULL) {
		printf ("error opening '%s' : %s\n", fisier, strerror(errno));
		exit (2);
	}
	if (ac > 2) {
		outf = fopen (av[2], "w");
		printf ("saving backup '%s' in '%s'\n", fisier, av[2]);
	}
	else {
		int size = strlen (fisier) + 5;
		char* buf = (char*) malloc (size);
		if (NULL == buf) {
			printf ("error allocating buffer of size %d\n", size);
			fclose (fdi);
			exit (3);
		}
		sprintf (buf, "%s.bin", fisier);
		outf = fopen (buf, "w");
		printf ("saving backup in '%s'\n", buf);
		free (buf);
	}
	if (NULL == outf) {
		printf ("error creating backup file: '%s'\n", strerror(errno));
		fclose (fdi);
		exit (4);
	}

	write_header (outf);

	nbrecords = 0;
	do {
		char param[256], value[65536];
		nbr = read_param_value (fdi, param, value);
		if (nbr > 0) {
			write_pv (outf, param, value);
			nbrecords += nbr;
		}
	} while (nbr > 0);
	printf ("%d values read\n", nbrecords);
	update_header (outf, nbrecords);

	fclose (fdi);
	fclose (outf);
	return 0;
}
