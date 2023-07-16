#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "sv.h"

struct sv sv_from_parts(char* str, size_t len) {
	struct sv sv = {
		.base = str,
		.len = len
	};

	return sv;
}

struct sv sv_from_cstr(char* cstr) {
	return sv_from_parts(cstr, strlen(cstr));
}

struct sv sv_chop_by_delim(struct sv* sv, const char delim) {
	size_t i = 0;
	for (; i < sv->len && sv->base[i] != delim; i++);

	struct sv res = sv_from_parts(sv->base, i);

	if (i < sv->len) {
		sv->base += i + 1;
		sv->len  -= i + 1;
	} else {
		sv->base += i;
		sv->len  -= i;
	}

	return res;
}

int sv_comp(struct sv a, struct sv b) {
	if (a.len != b.len)
		return -1;

	return strncmp(a.base, b.base, a.len);
}

char* sv_to_cstr(struct sv sv) {
	char* res = malloc(sizeof(char) * (sv.len + 1));
	memcpy(res, sv.base, sv.len);
	res[sv.len] = '\0';
	return res;
}

int sv_contains(const struct sv a, const char c) {
	for (size_t i = 0; i < a.len; i++) {
		if (a.base[i] == c)
			return i;
	}

	return -1;
}
