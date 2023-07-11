#ifndef __PIRC_STR_VIEW_H__
#define __PIRC_STR_VIEW_H__

#include <stddef.h>

struct sv {
	char* base;
	size_t len;
};


#define SV(cstr_lit) \
	(struct sv) { \
		.base = cstr_lit, \
		.len = sizeof(cstr_lit) - 1 \
	} \

#define SV_FMT "%.*s"
#define SV_ARGS(sv) (int)sv.len, sv.base

struct sv sv_from_parts(char* str, size_t len);
struct sv sv_from_cstr(char* cstr);
char* sv_to_cstr(struct sv sv);
struct sv sv_chop_by_delim(struct sv* sv, const char delim);
int sv_comp(struct sv a, struct sv b);

#endif // __PIRC_STR_VIEW_H__
