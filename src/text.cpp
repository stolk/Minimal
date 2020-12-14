#include "text.h"

extern "C" 
{
#include "dblunt.h"
}

#include "vmath.h"
#include "checkogl.h"
#include "rendercontext.h"
#include "logx.h"


#define SCRATCHBUFSZ	(24*1024)
static float scratchbuf[ SCRATCHBUFSZ ];

void text_draw_string
(
	const char* str,
	const rendercontext_t& rc,
	vec3_t pos,
	vec3_t sz,
	const char* halign,
	const char* valign
)
{
	// Convert the ASCII text to a renderable format (2D triangles.)
	float textw=-1;
	float texth=-1;
	const int numtria = dblunt_string_to_vertices
	(
		str,
		scratchbuf,
		sizeof(scratchbuf),
		pos[0], pos[1],
		sz[0], sz[1],
		-1,
		&textw,
		&texth
	);

	float shiftx = 0.0f;
	float shifty = 0.0f;

	if ( !strcmp( valign, "center" ) )
		shifty = 0.5f * texth;
	if ( !strcmp( valign, "bottom" ) )
		shifty = 1.0f * texth;

	if ( !strcmp( halign, "center" ) )
		shiftx = -0.5f * textw;
	if ( !strcmp( halign, "right" ) )
		shiftx = -1.0f * textw;

	for ( int i=0; i<numtria*3; ++i )
	{
		scratchbuf[2*i+0] += shiftx;
		scratchbuf[2*i+1] += shifty;
	}

	// Now we have a whole bunch of triangle vertices that we need to put into a VBO.
	const int numv = numtria * 3;
	GLuint vbo=0;
#ifdef USE_VAO
	GLuint vao=0;
	glGenVertexArrays( 1, &vao );
	glBindVertexArray( vao );
#endif
	glGenBuffers( 1, &vbo );
	glBindBuffer( GL_ARRAY_BUFFER, vbo );
	glBufferData( GL_ARRAY_BUFFER, numv*2*sizeof(float), (void*)scratchbuf, GL_STREAM_DRAW );
	CHECK_OGL
	glVertexAttribPointer( ATTRIB_VERTEX, 2, GL_FLOAT, 0, 2 * sizeof(float), (void*) 0 /* offset in vbo */ );
	CHECK_OGL
	glEnableVertexAttribArray( ATTRIB_VERTEX );
	CHECK_OGL

	// The VBO has been constructed, so we can draw the triangles.
	glDrawArrays( GL_TRIANGLES, 0, numv );
	CHECK_OGL

#ifdef USE_VAO
	glBindVertexArray( 0 );
	glDeleteVertexArrays( 1, &vao );
	CHECK_OGL
#endif
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glDeleteBuffers( 1, &vbo );
	CHECK_OGL
}

