#ifndef __UTILS_H_
#define __UTILS_H_

#include <unistd.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXIT_ON_ERROR(eval) do { \
	if ((eval)) { \
		fprintf(stderr, "%s:%d: (%d)%s\n", \
				__FILE__, __LINE__, errno, strerror(errno)); \
		exit(EXIT_FAILURE); \
	} \
} while(0) \

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#endif // __UTILS_H_
