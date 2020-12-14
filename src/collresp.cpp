#include "collresp.h"
#include "logx.h"
#include "odb.h"
#include "nfy.h"
#include "dbd.h"
#include "logx.h"

#define TERRAINSIM

typedef void (*collrespfunc_t)( dGeomID g0, dGeomID g1, int num_contacts, dContact* contacts );

static dWorldID world;
static dJointGroupID contactgroup;


static void response_terr_tyre( dGeomID g0, dGeomID g1, int num_contacts, dContact* contact )
{
	const dGeomID gterr = g0;
	const dGeomID gtyre = g1;
	const int nr = (int)(long) dGeomGetData( gtyre );
	const dBodyID b = odb_bodies[ nr ];
	const dReal* av = dBodyGetAngularVel( b );
	const float abswheelspeed = dLENGTH( av );
	const dReal* m = dGeomGetRotation( gtyre );
	vec3_t wheel_dir( m[2],m[6],m[10] );
	for ( int i=0; i<num_contacts; ++i )
	{
		contact[i].surface.mode = dContactSoftERP | dContactSoftCFM | dContactApprox1;
		contact[i].surface.soft_erp = 0.4f;
		contact[i].surface.soft_cfm = 0.3f;
		contact[i].surface.slip1 = 0.0f;
		contact[i].surface.slip2 = 0.0f;
		const int side = contact[i].geom.side1;
		const dReal* cn = contact[i].geom.normal;
		if ( side != -1 )
		{
			dVector3 v0={0,0,0,0};
			dVector3 v1={0,0,0,0};
			dVector3 v2={0,0,0,0};
			dGeomTriMeshGetTriangle( gterr, side, &v0, &v1, &v2 );
			vec3_t edge0( v1[0]-v0[0], v1[1]-v0[1], v1[2]-v0[2] );
			vec3_t edge1( v2[0]-v1[0], v2[1]-v1[1], v2[2]-v1[2] );
			vec3_t nrm = edge0.crossProduct( edge1 );
			nrm.normalize();
			vec3_t oldnrm( cn[0],cn[1],cn[2] );
			const float nrmsdot = oldnrm.dotProduct( nrm );
			// Throw away all contacts whose normal does not align with triangle normal.
			if ( nrmsdot < 0.7f )
			{
				//dbd_line( l0, l0+oldnrm*contact[i].geom.depth*20 );
				contact[i].geom.g1 = 0;
				contact[i].geom.g2 = 0;
				continue;
			}
		}
		if ( fabsf( dCalcVectorDot3( wheel_dir, contact[ i ].geom.normal ) ) < 0.9f )
		{
			const float lateral_friction = 1.0f;
			const float axial_friction   = 0.7f;
			dCalcVectorCross3( contact[i].fdir1, contact[i].geom.normal, wheel_dir );
			dNormalize3( contact[i].fdir1 );
			const float slip = ( abswheelspeed < 0.1f ) ? 0.1f : abswheelspeed;
			const float fds = 0.22f;
			contact[i].surface.slip2 = fds * slip; // slip perpendicular to roll direction
			contact[i].surface.slip1 = fds * slip; // slip in roll direction
			contact[i].surface.mode =
			dContactApprox1 |
			dContactFDir1 |
			dContactSlip1 |
			dContactSlip2 |
			dContactMu2 |
			dContactSoftERP |
			dContactSoftCFM
			;
			contact[i].surface.mu  = axial_friction;
			contact[i].surface.mu2 = lateral_friction;
		}
	}
}


static void response_generic( dGeomID g0, dGeomID g1, int num_contacts, dContact* contact )
{
	for ( int i=0; i<num_contacts; ++i )
	{
		contact[i].surface.mode = dContactSoftERP | dContactSoftCFM | dContactApprox1;
		contact[i].surface.soft_erp = 0.50f;
		contact[i].surface.soft_cfm = 0.03f;
		contact[i].surface.mu       = 1.00f;
	}
}


static const int collresp_cnt=1;

static collrespfunc_t collresp_funcs[ collresp_cnt ] =
{
	response_terr_tyre,
};


static unsigned long collresp_signatures[ collresp_cnt ] =
{
	CATEGORY_TERR | CATEGORY_TYRE,
};


static __inline__ collrespfunc_t lookupfunc( unsigned long signature )
{
	for ( int i=0; i<collresp_cnt; ++i )
		if ( signature == collresp_signatures[ i ] )
			return collresp_funcs[ i ];
	return 0;
}


void collresp_init( dWorldID w, dJointGroupID cg )
{
	contactgroup = cg;
	world = w;
}


// Respond to a collision event.
void collresp_respond( dGeomID g0, dGeomID g1, int num_contacts, dContact* contacts )
{
	const unsigned long b0 = dGeomGetCategoryBits( g0 );
	const unsigned long b1 = dGeomGetCategoryBits( g1 );

	const unsigned long signature = b0 | b1;
	collrespfunc_t response_func = lookupfunc( signature );
	if ( !response_func )
		response_func = response_generic;
	
	const int d0 = (int)(long)dGeomGetData( g0 );
	const int d1 = (int)(long)dGeomGetData( g1 );
	if ( ODB_TYPE_REGULAR == ( d0 & ODB_TYPE_MASK ) )
	{
		ASSERT( d0 >= 0 && d0 < odb_objCnt );
		odb_timesincecoll[ d0 ] = 0.0f;
	}
	if ( ODB_TYPE_REGULAR == ( d1 & ODB_TYPE_MASK) )
	{
		ASSERT( d1 >= 0 && d1 < odb_objCnt );
		odb_timesincecoll[ d1 ] = 0.0f;
	}
	if ( b1 >= b0 )
		response_func( g0, g1, num_contacts, contacts );
	else
		response_func( g1, g0, num_contacts, contacts );
}

