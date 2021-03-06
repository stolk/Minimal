#include "vmath.h"
#include "rendercontext.h"

extern void text_draw_string
(
	const char* str,
	const rendercontext_t& rc,
	vec3_t pos,
	vec3_t sz,
	const char* halign="center",
	const char* valign="center"
);

