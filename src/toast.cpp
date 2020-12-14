#include "toast.h"
#include "text.h"
#include "glpr.h"
#include "nfy.h"


const char* toast_text=0;

void toast_draw( const rendercontext_t& rc )
{
	if ( toast_text )
	{
		static int font_colourUniform = glpr_uniform( "colour" );
		glUniform4f( font_colourUniform, 0.1f, 0.1f, 0.55f, 1.0f );
		const char pill[ 2 ] = { 0x11, 0 };
		text_draw_string( pill, rc, vec3_t(0,0,0), vec3_t(1.8,2*1.8,0), "center", "center" );
		const vec3_t sz_lg( 0.15, 0.30, 0 );
		const vec3_t sz_sm( 0.067, 0.16, 0 );
		const bool use_small = strlen( toast_text ) > 24;
		const vec3_t sz = use_small ? sz_sm : sz_lg;

		glUniform4f( font_colourUniform, 1.0f, 1.0f, 1.0f, 1.0f );
		text_draw_string( toast_text,  rc, vec3_t(0,0,0), sz, "center", "center" );
	}
}


