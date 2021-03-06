#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>

#include "log.h"

const char *
sc_read_file(const char *filename)
{
	FILE *f;
	char * buffer = 0;
	long length;

	f = fopen(filename, "r");

	if (f == NULL) {
		ELOG("error: Unable to open file. %s\n", filename);
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	length = ftell(f);
	fseek(f, 0, SEEK_SET);
	buffer = malloc(length+1);
	if (buffer) {
		fread(buffer, 1, length, f);
	}
	buffer[length] = '\0';
	fclose(f);

	return buffer;
}
