#ifndef WLD_H
#define WLD_H

#include "vmath.h"
#include "rendercontext.h"

extern void wld_create( int levelnr, const char* leveltag );
extern void wld_destroy( void );
extern void wld_draw_main( const rendercontext_t& rc );
extern void wld_draw_edge( const rendercontext_t& rc );
extern void wld_draw_shdw( const rendercontext_t& rc );
extern void wld_update( float dt, bool camlock );
extern vec3_t wld_poi( void );
extern vec3_t wld_doi( void );
extern mat44_t wld_toi( void );

extern float wld_fogintensity;

extern float wld_timestamp;
extern bool  wld_created;
extern int   wld_levelnr;
extern bool  wld_level_ended;
extern bool  wld_level_won;
extern float wld_time_since_end;

#endif

