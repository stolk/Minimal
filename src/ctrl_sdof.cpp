#include "ctrl_sdof.h"

#include "checkogl.h"
#include "offsc.h"
#include "quad.h"
#include "glpr.h"
#include "logx.h"

#if defined( USEES3 ) || defined( USECOREPROFILE )
#	include "sdof_shadersources_glsl_es300.h"
#else
#	include "sdof_shadersources_glsl_es200.h"
#endif

float ctrl_sdof_focal_dist = 0.975f;
float ctrl_sdof_fgnd_blur = 40.0f;
float ctrl_sdof_bgnd_blur = 40.0f;

extern void ctrl_drawScene( void );

// GLSL program handles.
static unsigned int dofProgram=0;
static unsigned int ssamProgram=0;
static unsigned int blurProgram=0;
static unsigned int cocProgram=0;

// Handles to offscreen buffers, provided by offsc.h
static int  buf_render;		// rendered scene.
static int  buf_coc;		// rendered scene, signed circle of confusion in alpha channel.
static int  buf_subsam;		// downscaled rendered scene.
static int  buf_blrhor;		// horizontally blurred rendered scene.
static int  buf_blrvrt;		// fully blurred rendered scene.

// Retina Content Scale Factor, and framebuffer width, height.
static int csf;
static int fbw;
static int fbh;

#define DOWNSCALE	2

bool ctrl_sdof_create_buffers(void)
{
	offsc_init();
	buf_render = offsc_createFramebuffer( fbw, fbh, true );		// with depth component.
	buf_coc    = offsc_createFramebuffer( fbw, fbh, false );	// no depth component.
	buf_subsam = offsc_createFramebuffer( fbw/DOWNSCALE, fbh/DOWNSCALE, false );
	buf_blrhor = offsc_createFramebuffer( fbw/DOWNSCALE, fbh/DOWNSCALE, false );
	buf_blrvrt = offsc_createFramebuffer( fbw/DOWNSCALE, fbh/DOWNSCALE, false );

	return buf_render >= 0 && buf_coc >= 0 && buf_subsam >= 0 && buf_blrhor >= 0 && buf_blrvrt >= 0;
}


void ctrl_sdof_destroy_buffers(void)
{
	if ( buf_render >= 0 )
		offsc_destroyFramebuffer( buf_render );
	if ( buf_coc >= 0 )
		offsc_destroyFramebuffer( buf_coc );
	if ( buf_subsam >= 0 )
		offsc_destroyFramebuffer( buf_subsam );
	if ( buf_blrhor >= 0 )
		offsc_destroyFramebuffer( buf_blrhor );
	if ( buf_blrvrt >= 0 )
		offsc_destroyFramebuffer( buf_blrvrt );
	CHECK_OGL
}


void ctrl_sdof_recreate_buffers(int w, int h)
{
	ctrl_sdof_destroy_buffers();
	fbw = w;
	fbh = h;
	ctrl_sdof_create_buffers();
}

// To do shallow depth of field rendering, we need 5 offscreen renderbuffers.
// The render stages are: normal render, coc, subsample, blurh, blurv.
// In this function we create those buffers, and create shaders for the stages.
bool ctrl_sdof_create( int backingW, int backingH, int sf )
{
        csf = sf;
        fbw = backingW;
        fbh = backingH;

	const char* attribs_dof   = "position,uv";
	const char* attribs_coc   = "position,uv";
	const char* attribs_subs  = "position,uv";
	const char* attribs_blur  = "position,uv";

	const char* uniforms_dof  = "colormap,depthmap,blurmap,focaldist,fgndblur,bgndblur";
	const char* uniforms_coc  = "colormap,depthmap,focaldist,fgndblur,bgndblur";
	const char* uniforms_subs = "colormap";
	const char* uniforms_blur = "colormap,pixshift";

	dofProgram = glpr_load( "Dof",  source_vsh_Dof, source_fsh_Dof, attribs_dof, uniforms_dof );
        ASSERT(dofProgram);
        CHECK_OGL

	ssamProgram = glpr_load( "Subsample",  source_vsh_Subsample, source_fsh_Subsample, attribs_subs, uniforms_subs );
        ASSERT(ssamProgram);
        CHECK_OGL

	blurProgram = glpr_load( "BlurH",  source_vsh_Blur, source_fsh_Blur, attribs_blur, uniforms_blur );
        ASSERT(blurProgram);
        CHECK_OGL

	cocProgram = glpr_load( "Coc",  source_vsh_Coc, source_fsh_Coc, attribs_coc, uniforms_coc );
        ASSERT(cocProgram);
        CHECK_OGL

	LOGI( "dof  program loaded as %d", dofProgram );
	LOGI( "ssam program loaded as %d", ssamProgram );
	LOGI( "blur program loaded as %d", blurProgram );
	LOGI( "coc  program loaded as %d", cocProgram );

	return ctrl_sdof_create_buffers();
}


// Clean up the shader programs and the render buffers related to shallow depth of field post effect.
void ctrl_sdof_destroy( void )
{
	dofProgram=0;
	ssamProgram=0;
	blurProgram=0;
	cocProgram=0;
	ctrl_sdof_destroy_buffers();
	CHECK_OGL
}


// Calculates the circle of confusion for a shallow depth of field effect.
void ctrl_sdof_calculate_coc( void )
{
	offsc_use( buf_coc );
	glViewport( 0,0, csf*fbw, csf*fbh );
	//glClear( GL_COLOR_BUFFER_BIT );
	glpr_use( cocProgram );
	glDisable( GL_CULL_FACE );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_BLEND );
	CHECK_OGL

	static int colormapUniform = glpr_uniform( "colormap" );
	static int depthmapUniform = glpr_uniform( "depthmap" );
	static int focaldistUniform = glpr_uniform( "focaldist" );
	static int fgndblurUniform = glpr_uniform( "fgndblur" );
	static int bgndblurUniform = glpr_uniform( "bgndblur" );
	glUniform1i( colormapUniform, 0 ); // Use texture unit 0 for color.
	glUniform1i( depthmapUniform, 1 ); // Use texture unit 0 for color.
	glUniform1f( focaldistUniform, ctrl_sdof_focal_dist );
	glUniform1f( fgndblurUniform, ctrl_sdof_fgnd_blur );
	glUniform1f( bgndblurUniform, ctrl_sdof_bgnd_blur );
	CHECK_OGL

	glActiveTexture( GL_TEXTURE1 );
	glBindTexture( GL_TEXTURE_2D, offsc_depth_texture[buf_render] );
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, offsc_color_texture[buf_render] );
	quad_draw_dof();
	CHECK_OGL
}


// Subsample WxH framebuffer to W/2,H/2 framebuffer.
void ctrl_sdof_subsample( void )
{
	// Write to the subsample framebuffer
	offsc_use( buf_subsam );
	glViewport( 0,0, csf*fbw/DOWNSCALE, csf*fbh/DOWNSCALE );

	glpr_use( ssamProgram );
	glDisable( GL_CULL_FACE );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_BLEND );
	CHECK_OGL

	static int colormapUniform = glpr_uniform( "colormap" );
	glUniform1i( colormapUniform, 0 ); // Use texture unit 0 for color.
	CHECK_OGL

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, offsc_color_texture[buf_coc] );
	quad_draw_dof();
	CHECK_OGL
}


// Do 7 tap horizontal blur, with special weighting for Coc.
void ctrl_sdof_blurh( void )
{
	static int colormapUniform = glpr_uniform( "colormap" );
	static int pixshiftUniform = glpr_uniform( "pixshift" );

	offsc_use( buf_blrhor );
	glViewport( 0,0, csf*fbw/DOWNSCALE, csf*fbh/DOWNSCALE );

	glpr_use( blurProgram );
	glDisable( GL_CULL_FACE );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_BLEND );

	glUniform1i( colormapUniform, 0 ); // Use texture unit 0 for color.
	glUniform2f( pixshiftUniform, 1.25f / (fbw/2), 0.0 );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, offsc_color_texture[buf_subsam] );
	quad_draw_dof();
	CHECK_OGL
}


// Do 7 tap vertical blur, with special weighting for CoC.
void ctrl_sdof_blurv( void )
{
	static int colormapUniform = glpr_uniform( "colormap" );
	static int pixshiftUniform = glpr_uniform( "pixshift" );

	offsc_use( buf_blrvrt );
	glViewport( 0,0, csf*fbw/DOWNSCALE, csf*fbh/DOWNSCALE );

	glpr_use( blurProgram );

	glUniform1i( colormapUniform, 0 ); // Use texture unit 0 for color.
	glUniform2f( pixshiftUniform, 0.0, 1.25f / (fbh/2) );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, offsc_color_texture[buf_blrhor] );
	quad_draw_dof();
	CHECK_OGL
}


// Do all offscreen passes.
// (Except for shadow render pass, which we assume is done.)
void ctrl_sdof_draw_offscreen( void )
{
	// First pass of shadow render is already done.
	// Second pass: draw the scene into a texture.
	offsc_use( 0 );
	glViewport( 0,0, csf*fbw, csf*fbh );
	glClearColor( 1.0, 0.7, 0.2, 1.0 );
	glClear( GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT );
	CHECK_OGL
	ctrl_drawScene();
	CHECK_OGL

	// Third pass: calculate the circle of confusion.
	ctrl_sdof_calculate_coc();

	// Fourth pass: subsample
	ctrl_sdof_subsample();
	CHECK_OGL

	// Fifth pass: blur horizontally
	ctrl_sdof_blurh();
	CHECK_OGL

	// Sixth pass: blur vertically
	ctrl_sdof_blurv();
	CHECK_OGL
}


// Do final render combining original and blurred buffers.
void ctrl_sdof_draw_final( void )
{
	glpr_use( dofProgram );
	CHECK_OGL
	static int colormapUniform = glpr_uniform( "colormap" );
	static int depthmapUniform = glpr_uniform( "depthmap" );
	static int blurmapUniform  = glpr_uniform( "blurmap" );
	static int focaldistUniform = glpr_uniform( "focaldist" );
	static int fgndblurUniform = glpr_uniform( "fgndblur" );
	static int bgndblurUniform = glpr_uniform( "bgndblur" );
	glUniform1i( colormapUniform, 0 ); // Use texture unit 0 for color.
	glUniform1i( depthmapUniform, 1 ); // Use texture unit 1 for depth.
	glUniform1i( blurmapUniform,  2 ); // Use texture unit 2 for depth.
	glUniform1f( focaldistUniform, ctrl_sdof_focal_dist );
	glUniform1f( fgndblurUniform, ctrl_sdof_fgnd_blur );
	glUniform1f( bgndblurUniform, ctrl_sdof_bgnd_blur );
	CHECK_OGL
	glActiveTexture( GL_TEXTURE2 );
	glBindTexture( GL_TEXTURE_2D, offsc_color_texture[buf_blrvrt] );
	glActiveTexture( GL_TEXTURE1 );
	glBindTexture( GL_TEXTURE_2D, offsc_depth_texture[buf_render] );
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, offsc_color_texture[buf_render] );
	glDisable( GL_BLEND );
	glDisable( GL_CULL_FACE );
	glDisable( GL_DEPTH_TEST );
	quad_draw_dof();
	CHECK_OGL
}

