#include "wld.h"
#include "ode/ode.h"
#include "odb.h"
#include "odb_spawn_misc.h"

#include "categories.h"
#include "collresp.h"

#include "nfy.h"
#include "dbd.h"
#include "logx.h"
#include "checkogl.h"
#include "glpr.h"
#include "meshdb.h"


// physics
static dWorldID world=0;
static dSpaceID space=0;
static dJointGroupID contactgroup=0;

float wld_timestamp=0.0f;		// time since creation of wld
int   wld_framecount = 0;

float wld_fogintensity = 0.002f;		// was: 0.1

bool wld_created;
bool wld_level_ended;
bool wld_level_won;
float wld_time_since_end;
int wld_levelnr;
char wld_leveltag[80];

static int groundplaneid;

#undef THREADED


void wld_create( int levelnr, const char* leveltag )
{
	LOGI( "wld_create( %d, %s )", levelnr, leveltag );

	strncpy( wld_leveltag, leveltag, sizeof(wld_leveltag) );
	wld_levelnr = levelnr;

	wld_level_ended = false;
	wld_level_won = false;
	wld_time_since_end = -1.0f;

#if !defined( THREADED )
	dInitODE();
#else
	dInitODE2( 0 );
	
	dThreadingImplementationID threading = dThreadingAllocateMultiThreadedImplementation();
	
	if ( !threading )
		LOGE( "No threading implementation built into OpenDE." );
	dThreadingThreadPoolID pool = dThreadingAllocateThreadPool( 4, 0, dAllocateFlagBasicData, NULL );
	if ( !pool )
		LOGE( "No threading pool allocated for OpenDE." );
	dThreadingThreadPoolServeMultiThreadedImplementation( pool, threading );
#endif
	
	world = dWorldCreate();
	LOGI( "world created at %p", world );
	// dWorldSetStepIslandsProcessingMaxThreadCount( world, 1 );
	
#if defined(THREADED)
	dWorldSetStepThreadingImplementation( world, dThreadingImplementationGetFunctions(threading), threading );

	dAllocateODEDataForThread( dAllocateMaskAll );
#endif

	space = dHashSpaceCreate (0);
	dHashSpaceSetLevels( space, -2, 7 );
	//space = dSimpleSpaceCreate( space );
	LOGI( "space created at %p", space );

	dGeomSetData( (dGeomID)space, (void*)0 );

	contactgroup = dJointGroupCreate (0);
	dWorldSetGravity( world,0,0,-9.8*0.7f );
	
	//dWorldSetAutoDisableFlag( world, true );
	dWorldSetAutoDisableLinearThreshold( world, 0.03 );
	dWorldSetAutoDisableAngularThreshold( world, 0.09 );
	dWorldSetAutoDisableTime( world, 0.05f );

	dWorldSetQuickStepNumIterations( world, 20 ); // was: 20
#if 1
	dWorldSetQuickStepW( world, 0.75f ); // TESTING!
#endif

	dWorldSetContactMaxCorrectingVel( world, 40.0f ); // was 400
	dWorldSetMaxAngularSpeed( world, 2*62.8 ); // max 20 rps

	dWorldSetERP( world, 0.7f );
	dWorldSetContactSurfaceLayer( world, 0.001f ); // was: 2cm penetration allowed!

	// Create the object database.
	odb_create ( world, space );

	// Spawn a bedrock plane at z=0.
	groundplaneid = odb_spawn_plane( "groundplane", vec3_t( 0,0,0 ) );

	const int coneid = odb_spawn_trafficcone( vec3_t(0,0,4) );

	// Initialize the debug drawing system.
	dbd_init();

	// Initialize the collision response system.
	collresp_init( world, contactgroup );

	const int numGeoms = dSpaceGetNumGeoms( space );
	LOGI( "Space contains %d geoms.", numGeoms );

	wld_timestamp = 0.0f;
	wld_framecount = 0;
	wld_created = true;
}


void wld_destroy( void )
{
	if ( !wld_created ) return;

	LOGI( "wld_destroy()" );
	wld_created = false;
	wld_level_ended = false;
	wld_level_won = false;
	wld_time_since_end = -1;

	// clear the collision mesh db.
	meshdb_clear();

	// Clear out the object database.
	odb_destroy();

	dJointGroupEmpty( contactgroup );
	dJointGroupDestroy( contactgroup );
	contactgroup = 0;
	
	LOGI( "destroying top level space at %p", space );
	dSpaceDestroy( space );
	space = 0;
	
	LOGI( "destroying world at %p", world );
	dWorldDestroy( world );
	world = 0;

	dCloseODE();
}



void wld_draw_main( const rendercontext_t& rc )
{
	static int fogintensityUniform = glpr_uniform( "fogintensity" );
	glUniform1f( fogintensityUniform, wld_fogintensity );
	CHECK_OGL
	odb_draw_main( rc );
	CHECK_OGL

	odb_draw_groundplane( rc, groundplaneid );
	CHECK_OGL
}


void wld_draw_shdw( const rendercontext_t& rc )
{
	odb_draw_shdw( rc );
}


void wld_draw_edge( const rendercontext_t& rc )
{
	static int edge_fogintensityUniform = glpr_uniform( "fogintensity" );
	static int edge_linecolourUniform = glpr_uniform( "linecolour" );

	glUniform4f( edge_linecolourUniform, 0.9f, 0.8f, 0.2f, 1.0f );
	glUniform1f( edge_fogintensityUniform, 0.0f );
	dbd_draw_edge( rc );
	dbd_clear();
	CHECK_OGL

	glUniform4f( edge_linecolourUniform, 0.2f, 0.2f, 0.2f, 1.0f );
	glUniform1f( edge_fogintensityUniform, wld_fogintensity );
	odb_draw_edge( rc );
	CHECK_OGL

	glUniform4f( edge_linecolourUniform, 1.0f, 1.0f, 1.0f, 1.0f );
}


// this is called by dSpaceCollide when two objects in space are
// potentially colliding.

const char* cat( int c )
{
	switch ( c )
	{
		case CATEGORY_TERR : return "terr";
		case CATEGORY_PRLO : return "prlo";
		case CATEGORY_PRHI : return "prhi";
		case CATEGORY_PROP : return "prop";
		case CATEGORY_DIGR : return "digr";
		case CATEGORY_CRAN : return "cran";
		case CATEGORY_DIRT : return "dirt";
		case CATEGORY_TRAC : return "trac";
		case CATEGORY_TRUK : return "truk";
		case CATEGORY_SPKT : return "spkt";
	}
	return "????";
}

static void nearCallback( void* data, dGeomID o0, dGeomID o1 )
{
	assert(o0);
	assert(o1);
	
	const bool isSpace0 = dGeomIsSpace( o0 ) != 0;
	const bool isSpace1 = dGeomIsSpace( o1 ) != 0;
#if 0
	const int  onr0 = (int) (long) dGeomGetData( o0 );
	const int  onr1 = (int) (long) dGeomGetData( o1 );
	if ( isSpace0 && isSpace1 )
	{
		const char* nm0 = odb_spacenames[ -onr0 ];
		const char* nm1 = odb_spacenames[ -onr1 ];
		LOGI( "%s vs %s", nm0, nm1 );
	}
#endif
	if ( isSpace0 || isSpace1 )
	{
		// colliding a space with something
		dSpaceCollide2( o0, o1, data, &nearCallback );
		// Note we do not want to test intersections within a space,
		// only between spaces.
		return;
	}

#if 1
	const long b0 = dGeomGetCategoryBits( o0 );
	const long b1 = dGeomGetCategoryBits( o1 );
	const char* c0 = cat( (int) b0 );
	const char* c1 = cat( (int) b1 );
	//LOGI( "%s-vs-%s", c0, c1 );
	ASSERT( ( b0 | b1 ) != CATEGORY_TERR ); // no terrain vs terrain pls!
#endif

	// Two non space geoms
	const int MAXCONTACTS = 128;
	dContact contacts[ MAXCONTACTS ];
	int n = dCollide ( o0, o1, MAXCONTACTS, &(contacts[0].geom), sizeof(dContact) );
	if ( n==MAXCONTACTS )
		LOGE( "Number of contacts between %s and %s exceeds maximum (%d).", c0, c1, MAXCONTACTS );

	if ( n > 0 )
	{
		// Let a double-dispatched collision type set custom contact parameters.
		collresp_respond( o0, o1, n, contacts );
		// Create constraints for all the contacts between the geometries.
		for (int i=0; i<n; i++)
		{
			const dReal* cp = contacts[i].geom.pos;
			const dReal* cn = contacts[i].geom.normal;
			if ( isnanf( cp[0] ) || isnanf( cp[1] ) || isnanf( cp[2] ) )
			{
				LOGE( "Collider generated contact pos with NaN values." );
				continue;
			}
			if ( isnanf( cn[0] ) || isnanf( cn[1] ) || isnanf( cn[2] ) )
			{
				LOGE( "Collider generated normal with NaN values." );
				continue;
			}
			if ( isnanf( contacts[i].geom.depth ) )
			{
				LOGE( "Collider generated NaN depth value." );
				continue;
			}
			if ( contacts[ i ].geom.g1 == 0 && contacts[ i ].geom.g2 == 0 )
				continue;
			dJointID c = dJointCreateContact ( world, contactgroup, &contacts[i] );
			const dGeomID g0 = contacts[i].geom.g1;
			const dGeomID g1 = contacts[i].geom.g2;
			dBodyID bod0 = g0 ? dGeomGetBody( g0 ) : 0;
			dBodyID bod1 = g1 ? dGeomGetBody( g1 ) : 0;
			assert( bod0 || bod1 );
			dJointAttach( c, bod0, bod1 );
#if 0
			vec3_t p0( cp[0], cp[1], cp[2] );
			vec3_t p1 = p0 + vec3_t( cn[0], cn[1], cn[2] ) * contacts[ i ].geom.depth;
			dbd_vector( p0, p1 );
#endif
		}
	}
}


// Called with 120Hz
void wld_update( float dt, bool camlock )
{
	if ( !wld_created ) return;

	wld_timestamp += dt;

	// run physics
	float simstep = dt;

	dSpaceCollide( space, 0, &nearCallback );
	dWorldQuickStep( world, simstep );
	dJointGroupEmpty( contactgroup );
	
	vec3_t focuspt(0,0,0);

	// This will update the visual models to match the physics bodies.
	const float culldist = 10000.0f;
	odb_update( dt, focuspt, culldist);

#if 0
	if ( wld_level_ended )
	{
		wld_time_since_end += dt;
	}
	else
	{
		if ( win || lose )
		{
			wld_level_ended = true;
			wld_level_won = win;
			wld_time_since_end = 0.0f;
			char m[128];
			snprintf( m, sizeof(m), "gameend win=%d levelnr=%d leveltag=%s timestamp=%f", win, wld_levelnr, wld_leveltag, wld_timestamp );
			nfy_msg( m );
		}
	}
#endif

	// HUH? Two updates? Without it, joints do not visibly line up under movement?
	odb_update( dt, focuspt, 0.80f*culldist );

}


vec3_t wld_poi( void )
{
	return vec3_t(0,0,0);
}


vec3_t wld_doi( void )
{
	return vec3_t(1,0,0);
}


mat44_t wld_toi( void )
{
	mat44_t m;
	m.identity();
	return m;
}


