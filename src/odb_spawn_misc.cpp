#include "odb_spawn_misc.h"
#include "odb.h"

#include "vmath.h"
#include "categories.h"
#include "logx.h"
#include "geomdb.h"	// to store render meshes.
#include "meshdb.h"	// to store collision meshes.

#include "ode/ode.h"
#include "checkogl.h"
#include "glpr.h"

#include "geom_groundplane.h"
#include "geom_trafficcone.h"

GEOMDEF( groundplane )
GEOMDEF( trafficcone )


int odb_spawn_trafficcone( vec3_t pos )
{
	ASSERT( odb_objCnt < WORLD_MAX_OBJS );
	odb_descriptions[ odb_objCnt ] = &dsc_trafficcone;
	geomdb_add( odb_descriptions[ odb_objCnt ] );
	odb_names[ odb_objCnt ] = "trafficcone";
	dBodyID body = dBodyCreate( odb_world );
	odb_bodies[ odb_objCnt ] = body;
	odb_trfs[ odb_objCnt ].identity();
	odb_trfs[ odb_objCnt ].setTranslation( pos );

	dBodySetData( body, (void*)(long)odb_objCnt );
	dBodySetPosition( body, pos[0],pos[1],pos[2] );
	dMass m;
	dMassSetSphere( &m, 4.000f, 0.09f );
	dBodySetMass( body, &m );
	//LOGI( "Mass of traffic cone: %f", m.mass );
	dBodySetAngularDamping( body, 0.05f );
	dBodySetLinearDamping( body, 0.007f );
	dBodySetAutoDisableLinearThreshold( body, 0.02 );
	dBodySetAutoDisableAngularThreshold( body, 0.1 );
	dBodySetAutoDisableTime( body, 0.09f );

	dReal sizes[2][3] = {
		0.09, 0.09, 0.208,
		0.25, 0.25, 0.05,
	};
	dReal offsets[2][3] = {
		0.0, 0.0,  0.02,
		0.0, 0.0, -0.109,
	};
	for ( int i=0; i<2; ++i )
	{
		dGeomID geom = dCreateBox( odb_rootspace, sizes[i][0], sizes[1][1], sizes[i][2] );
		dGeomSetBody( geom, body );
		dGeomSetOffsetPosition( geom, offsets[i][0], offsets[i][1], offsets[i][2] );
		dGeomSetData( geom, (void*)(long)odb_objCnt );
		dGeomSetCategoryBits( geom, CATEGORY_PERS );
		dGeomSetCollideBits ( geom, CATEGORIES_DYNAMIC );
	}
	return odb_objCnt++;
}


int odb_spawn_plane( const char* name, vec3_t pos )
{
	ASSERT( odb_objCnt < WORLD_MAX_OBJS );
	odb_descriptions[ odb_objCnt ] = 0;
	geomdb_add( &dsc_groundplane );
	odb_names[ odb_objCnt ] = name;
	odb_bodies[ odb_objCnt ] = 0;
	odb_trfs[ odb_objCnt ].identity();
	odb_trfs[ odb_objCnt ].setTranslation( pos );
	dGeomID geom = dCreatePlane( odb_rootspace, 0, 0, 1, pos[2] );
	dGeomSetData( geom, (void*)(long)odb_objCnt );
	dGeomSetCategoryBits( geom, CATEGORY_TERR );
	dGeomSetCollideBits( geom, CATEGORIES_DYNAMIC );
	LOGI( "Groundplane spawned." );
	return odb_objCnt++;
}


void odb_draw_groundplane( const rendercontext_t& rc, int groundplaneid)
{
	const mat44_t camViewProjMat = rc.camproj * rc.camview;
	const mat44_t lightViewProjMat = odb_adjustTrf * rc.lightproj * rc.lightview;
	geomdesc_t* geomdesc = &dsc_groundplane;
	mat44_t& trf = odb_trfs[ groundplaneid ];
	if ( geomdesc && geomdesc->numt )
	{
		mat44_t modelCamViewProjMat     = camViewProjMat * trf;
		mat44_t modelLightViewProjMat   = lightViewProjMat * trf;
		static int mcvpUniform = glpr_uniform( "modelcamviewprojmat" );
		static int mlvpUniform = glpr_uniform( "modellightviewprojmat" );
		glUniformMatrix4fv( mcvpUniform, 1, false, modelCamViewProjMat.data );
		glUniformMatrix4fv( mlvpUniform, 1, false, modelLightViewProjMat.data );
		ASSERT(geomdesc->vaos[0]);
                glBindVertexArray( geomdesc->vaos[0] );
		CHECK_OGL
                const int offset = 0;
                glDrawArrays( GL_TRIANGLES, offset, geomdesc->numt * 3 );
		CHECK_OGL
                glBindVertexArray( 0 );
		CHECK_OGL
        }
}

