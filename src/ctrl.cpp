#include "ctrl.h"
#include "ctrl_sdof.h"

#include "camera.h"
#include "light.h"
#include "nfy.h"
#include "checkogl.h"
#include "rendercontext.h"

#include "glpr.h"
#include "logx.h"

#include "wld.h"
#include "shdw.h"
#include "geomdb.h"

#include "view.h"

#include "quad.h"
#include "text.h"
#include "menu.h"

#include "toast.h"
#include "kv.h"

#define FARZ	80.0f

#include <stdlib.h>	// for system()
#include <errno.h>
#include <sys/stat.h>	// for mkdir()
#include <unistd.h>	// for unlink()

#if defined( USEES3 )
#include "shadersources_glsl_es100.h"
#else
#include "shadersources_glsl_150.h"
#endif

#if defined( USEGLDEBUGPROC )
#	include "gldebugproc.inl"
#endif


const char* ctrl_bundlePath = ".";
const char* ctrl_configPath = ".";

bool ctrl_dof_enabled = false;

static rendercontext_t renderContext;

static bool virgin  = true;
static bool hidehud = false;
static bool camlock = false;

static unsigned int mainProgram=0;
static unsigned int edgeProgram=0;
static unsigned int shdwProgram=0;
static unsigned int hudProgram=0;
static unsigned int fontProgram=0;

static bool supportsDepthTexture=false;
static bool supportsDiscardFramebuffer=false;
static bool supportsVertexArray=false;
static bool supportsDebugOutput=false;

static float low_pass_fps=60.0f;

static int fbw=0;
static int fbh=0;

static float csf=1.0f; // default is no retina: content scale factor is 1.0


#define VIEWPORT(r) \
	glViewport( csf*r.x, csf*r.y, csf*r.w, csf*r.h )


static void ctrl_enable_menu( bool a )
{
	view_enabled[ VIEWLVLS ] = a;
	view_enabled[ VIEWMAIN ] = !a;
}


static void onAspect( const char* m )
{
	float ratio = nfy_flt( m, "ratio" );
	const int w = nfy_int( m, "width" );
	const int h = nfy_int( m, "height" );
	const bool offaxis_projection = false;
	camera_setAspectRatio( ratio, 0.1f, FARZ, offaxis_projection );

	if (fbw != w || fbh != h)
	{
		fbw = w;
		fbh = h;
		if (ctrl_dof_enabled)
		{
			LOGI("We should resize the offscreen buffers.");
			ctrl_sdof_recreate_buffers(fbw, fbh);
		}
	}

	view_setup( fbw, fbh );
}


static void onPause( const char* m )
{
	ctrl_onBack();
}


static void onResume( const char* m )
{
	ctrl_enable_menu( false );
	camlock = false;
	bool sdof = kv_get_int( "settings_sdof", 0 );
	if (sdof && !ctrl_dof_enabled)
	{
		ctrl_sdof_destroy();
		ctrl_sdof_create(fbw, fbh, csf);
	}
	if (!sdof && ctrl_dof_enabled)
	{
		ctrl_sdof_destroy();
	}
	ctrl_dof_enabled = sdof;
}


static void onStart( const char* m )
{
	const int levelnr = nfy_int( m, "levelnr" ) ;
	char leveltag[80];
	nfy_str( m, "leveltag", leveltag, sizeof(leveltag) );
	assert( levelnr > INT_MIN );

	if ( wld_created )
	{
		if ( wld_levelnr == levelnr )
		{
			// Starting the same level!
			// Resuming: do not delete old and create new world.
			toast_text = 0;
			onResume( m );
			return;
		}
		else
			wld_destroy();
	}
	wld_create( levelnr, leveltag );
	onResume( m );
}


static void onGameend( const char* m )
{
}


static void onCamlock( const char* m )
{
	camlock = !camlock;
}


static void onHudtoggle( const char* m )
{
	hidehud = !hidehud;
}



static bool checkForExtension( const char* searchName )
{
#if defined( USECOREPROFILE )
	GLint n=0;
	glGetIntegerv( GL_NUM_EXTENSIONS, &n );
	for ( int i = 0; i < n; i++ )
	{
		const GLubyte* name = glGetStringi( GL_EXTENSIONS, i );
		if ( name && strstr( (const char*) name, searchName ) ) return true;
	}
	return false;
#else
	const unsigned char* extensions = glGetString( GL_EXTENSIONS );
	CHECK_OGL
	if (!extensions) return false;

	return ( strstr( (char*)extensions, searchName ) != 0 );
#endif
}


//! Done once per lifetime of the process.
static void ctrl_init( void )
{
#ifdef DEBUG
	LOGI( "DEBUG build" );
#else
	LOGI( "OPTIMIZED build" );
#endif
	LOGI( "Built on " __DATE__ " at " __TIME__ );
	
	const unsigned char* renderer = glGetString( GL_RENDERER );
	LOGI( "GL_RENDERER: %s", (const char*)renderer );
	const unsigned char* glversion = glGetString( GL_VERSION );
	LOGI( "GL_VERSION: %s", glversion );
	const unsigned char* slversion = glGetString( GL_SHADING_LANGUAGE_VERSION );
	LOGI( "GL_SHADING_LANGUAGE_VERSION: %s", slversion?(const char*)slversion:"None" );

	camera_init( 0.27f * M_PI );
        camera_minDist = 1.5f;
	camera_maxDist = 38.0f;
	view_init();

	virgin = false;
}


void ctrl_create( int backingW, int backingH, float sf )
{
	csf = sf;

	fbw = backingW;
	fbh = backingH;
	if ( virgin ) ctrl_init();

#if defined( OSX ) || defined( MSWIN ) || defined( XWIN ) || defined( IPHN ) || defined( ANDROID )
	supportsDepthTexture = true;
	supportsDiscardFramebuffer = true;
	supportsVertexArray = true;
	const bool supportsHalfFloat = true;
#else
	supportsDepthTexture = checkForExtension( "_depth_texture" );
	supportsDiscardFramebuffer = checkForExtension( "_discard_framebuffer" );
	supportsVertexArray = checkForExtension( "_vertex_array" );
	const bool supportsHalfFloat = checkForExtension( "_texture_half_float" );
#endif
	LOGI( "Does %s depth textures.", supportsDepthTexture ? "support" : "not support" );
	LOGI( "Does %s discard framebuffer.", supportsDiscardFramebuffer ? "support" : "not support" );
	LOGI( "Does %s vertex array.", supportsVertexArray ? "support" : "not support" );
	LOGI( "Does %s half float textures.", supportsHalfFloat ? "support" : "not support" );

	supportsDebugOutput = checkForExtension( "_debug_output" );
	LOGI( "Does %s debug output.", supportsDebugOutput ? "support" : "not support" );
	
#if defined( USEGLDEBUGPROC )
	if ( supportsDebugOutput )
	{
		glDebugMessageCallbackARB( (GLDEBUGPROCARB) gl_debug_proc, NULL );
		glEnable( GL_DEBUG_OUTPUT );
#if defined( DEBUG )
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
#endif
	}
#endif

	const bool offaxis_projection = false;
	camera_setAspectRatio( backingW / (float)backingH, 0.1, FARZ, offaxis_projection );
	camera_forceCOI( vec3_t( 3.2,3.2,3.2 ) );

	// Camera very loosely coupled to car.
	camera_setPanPid( -0.05, -0.05, -0.002 );
	camera_setCoiPid( -0.05, -0.05, -0.002 );

	nfy_msg( "cameraControl distSetting=4.4 elevationSetting=0.32 orbitSetting=-0.2 reset=1" );

	light_init();

	nfy_obs_add( "aspect", onAspect );
	nfy_obs_add( "pause", onPause );
	nfy_obs_add( "start", onStart );
	nfy_obs_add( "resume", onResume );
	nfy_obs_add( "gameend", onGameend );
	nfy_obs_add( "camlock", onCamlock );
	nfy_obs_add( "hudtoggle", onHudtoggle );

	const char* attribs_main = "position,surfacenormal,rgb";
	const char* attribs_shdw = "position";
	const char* attribs_edge = "position";
	const char* attribs_hud  = "position,uv";
	const char* attribs_font = "position";

	const char* uniforms_main = "modelcamviewprojmat,basecolour,modellightviewprojmat,modellightviewmat,shadowmap,fogintensity";
	const char* uniforms_shdw = "modellightviewprojmat";
	const char* uniforms_edge = "modelcamviewprojmat,linecolour,fogintensity";
	const char* uniforms_hud  = "rotx,roty,texturemap,translation";
	const char* uniforms_font = "rotx,roty,translation,colour,fadedist";

	glpr_init();

	mainProgram = glpr_load( "Main",  source_vsh_Main, source_fsh_Main, attribs_main, uniforms_main );
        ASSERT(mainProgram);
        CHECK_OGL

	edgeProgram = glpr_load( "Edge",  source_vsh_Edge, source_fsh_Edge, attribs_edge, uniforms_edge );
        ASSERT(edgeProgram);
        CHECK_OGL

	shdwProgram = glpr_load( "Shadow",  source_vsh_Shadow, source_fsh_Shadow, attribs_shdw, uniforms_shdw );
        ASSERT(shdwProgram);
        CHECK_OGL

	hudProgram = glpr_load( "Hud",  source_vsh_Hud, source_fsh_Hud, attribs_hud, uniforms_hud );
        ASSERT(hudProgram);
        CHECK_OGL

	fontProgram = glpr_load( "Font",  source_vsh_Font, source_fsh_Font, attribs_font, uniforms_font );
        ASSERT(fontProgram);
        CHECK_OGL

	LOGI( "Main program loaded as %d", mainProgram );
	LOGI( "Edge program loaded as %d", edgeProgram );
	LOGI( "Shdw program loaded as %d", shdwProgram );
	LOGI( "Hud  program loaded as %d", hudProgram );
	LOGI( "Font program loaded as %d", fontProgram );

	glpr_validate( mainProgram );
	glpr_validate( edgeProgram );
	glpr_validate( shdwProgram );
	glpr_validate( hudProgram );
	glpr_validate( fontProgram );

	shdw_createFramebuffer( supportsDepthTexture );

	ctrl_dof_enabled = kv_get_int("settings_sdof", 1);
	if ( ctrl_dof_enabled )
	{
		ctrl_dof_enabled = ctrl_sdof_create( backingW, backingH, sf );
		if ( !ctrl_dof_enabled )
		{
			LOGE( "Could not initialize shallow-depth-of-field rendering." );
			ctrl_sdof_destroy();
		}
	}
	LOGI( "%s shallow depth-of-field rendering", ctrl_dof_enabled ? "Enabled" : "Disabled" );

	int numLoaded = geomdb_load_vbos();
	LOGI( "Loaded %d geometry VBOs", numLoaded );

	quad_init();
	glpr_use( hudProgram );
	quad_prepare();

	view_setup( fbw, fbh );

	nfy_msg("start levelnr=0 leveltag=sandbox");
	

	glClearDepthf( 1.0f );
	CHECK_OGL

	glEnable(GL_FRAMEBUFFER_SRGB);

	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
}


void ctrl_destroy( void )
{
	wld_destroy();

	nfy_obs_rmv( "aspect", onAspect );
	nfy_obs_rmv( "pause", onPause );
	nfy_obs_rmv( "start", onStart );
	nfy_obs_rmv( "resume", onResume );
	nfy_obs_rmv( "gameend", onGameend );
	nfy_obs_rmv( "camlock", onCamlock );
	nfy_obs_rmv( "hudtoggle", onHudtoggle );

	quad_exit();
	CHECK_OGL

	const int numUnloaded = geomdb_unload_vbos();
	LOGI( "Unloaded %d geometry VBOs", numUnloaded );
	CHECK_OGL

	glpr_clear();
	CHECK_OGL

	mainProgram=0;
	shdwProgram=0;
	hudProgram=0;
	edgeProgram=0;
	fontProgram=0;

	shdw_destroyFramebuffer();
	CHECK_OGL

	ctrl_sdof_destroy();
	CHECK_OGL
}


void ctrl_drawShadow( void )
{
	glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );

	glCullFace( GL_FRONT );
	glEnable( GL_CULL_FACE );
	
	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );
	
	glEnable( GL_POLYGON_OFFSET_FILL );
	glPolygonOffset( 1.02, 0.0 );
	
	glDisable( GL_BLEND );

	glpr_use( shdwProgram );
	wld_draw_shdw( renderContext );
	CHECK_OGL
}


void ctrl_drawScene( void )
{
	if ( view_enabled[ VIEWMAIN ] )
	{
		//wld_fogintensity = 0.10f + ( 0.125f / log2f( camera_dist() ) );
		wld_fogintensity = 0.01f + ( 0.0125f / log2f( camera_dist() ) );

		VIEWPORT( view_rect( VIEWMAIN ) );
		// Draw to viewport
		glClearColor( 0.2, 0.2, 0.8, 1.0 );
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		CHECK_OGL
		glEnable( GL_DEPTH_TEST );

		glCullFace( GL_BACK );
		glEnable( GL_CULL_FACE );
		CHECK_OGL

		// Bind the shadow map to texture unit 0
		glActiveTexture ( GL_TEXTURE0 );
		glBindTexture ( GL_TEXTURE_2D, shdw_texture() );
		CHECK_OGL

		glEnable( GL_POLYGON_OFFSET_FILL );
		glPolygonOffset( 1.005, 0.0 );
		CHECK_OGL

		glpr_use( edgeProgram );
		CHECK_OGL

		wld_draw_edge( renderContext );
		CHECK_OGL

		glEnable( GL_DEPTH_TEST );
		CHECK_OGL

		glpr_use( mainProgram );
		static int main_basecolourUniform = glpr_uniform( "basecolour" );
		static int main_shadowmapUniform  = glpr_uniform( "shadowmap" );
		CHECK_OGL

		VIEWPORT( view_rect( VIEWMAIN ) );

		// Bind the shadow map to texture unit 0
		glActiveTexture ( GL_TEXTURE0 );
		glBindTexture ( GL_TEXTURE_2D, shdw_texture() );
		CHECK_OGL

		glUniform1i( main_shadowmapUniform, 0 ); // Use texture unit 0
		glUniform4f( main_basecolourUniform, 1,1,1,1 );
		wld_draw_main( renderContext );
		CHECK_OGL
	}
}


const char* ctrl_drawFrame( float dt )
{
	if ( dt > 0 && dt < 1 )
	{
		float fps = 1.0f / dt;
		low_pass_fps = 0.9f * low_pass_fps + 0.1f * fps;
	}

	if ( view_enabled[ VIEWLVLS ] )
	{
		glClearColor( 0.05,0.06,0.2, 1.0 );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		glDisable( GL_DEPTH_TEST );
		glDisable( GL_CULL_FACE );
		glpr_use( hudProgram );
		glEnable( GL_BLEND );
	}

	if ( view_enabled[ VIEWMAIN ] )
	{
		if ( ctrl_dof_enabled )
			ctrl_sdof_draw_final();
		else
			ctrl_drawScene();
	}

	view_enabled[ VIEWTOAS ] = toast_text != 0;
	if ( view_enabled[ VIEWTOAS ] )
	{
		VIEWPORT( view_rect( VIEWTOAS ) );
		glDisable( GL_DEPTH_TEST );
		glpr_use( fontProgram );
		toast_draw( renderContext );
	}

	if ( !hidehud )
	{
		glpr_use( fontProgram );
		glDisable( GL_DEPTH_TEST );
		glDisable( GL_CULL_FACE );
		VIEWPORT( view_rect( VIEWMAIN ) );
		char s[16];
		snprintf( s, 16, "%03d", (int) low_pass_fps );
		text_draw_string( s, renderContext, vec3_t(1,-1,0), vec3_t(0.023, 0.04, 0.0 ), "right", "bottom" );
		CHECK_OGL
	}
	geomdb_rotate_buffers();

	return 0;
}


//! Now called at 120Hz.
void ctrl_update( float dt )
{
	// Process all queued notifications.
	nfy_process_queue();

	view_update( dt );
	if ( !view_enabled[ VIEWMAIN ] ) return;

	wld_update( dt, camlock );

	const vec3_t poi = wld_poi();
	vec3_t doi = wld_doi();
	doi[2]=0.0f;
	doi.normalize();

	const float pan = atan2f( doi[1], doi[0] );
	//LOGI( "pan = %f   doi = %f,%f,%f", pan, doi[0], doi[1], doi[2] );

	if ( !camlock )
	{
		camera_setCOI( poi );
		camera_setPan( pan );
	}

	light_setCOI( poi );
	light_setPos( poi + vec3_t(0,0,10) );

	camera_update( dt );
	light_update( dt );

	light_setProjectionSize( 1.00*camera_dist(), 1.0, 44.0 );

	// Update the render context
	camera_getViewTransform( renderContext.camview );
	camera_getProjTransform( renderContext.camproj );

	light_getViewTransform( renderContext.lightview );
	light_getProjTransform( renderContext.lightproj );

	ctrl_sdof_focal_dist = camera_depthInZBuffer(poi);
	const float cd = camera_dist();
	ctrl_sdof_fgnd_blur = camera_dist() * 3 * cd * logf(cd);
	ctrl_sdof_bgnd_blur = camera_dist() * 3 * cd * logf(cd);
}


bool ctrl_onBack( void )
{
	// If game main window is open, go back to top level menu.
	if ( view_enabled[ VIEWMAIN ] )
	{
		ctrl_enable_menu( true );
		toast_text = 0;
		return true;
	}
	// We were already in top level menu, we should signal to quit the program.
	return false;
}


void ctrl_pause(void)
{
}


