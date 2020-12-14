#include "view.h"
#include "logx.h"
#include "nfy.h"
#include "toast.h"
#include "offsc.h"
#include "shdw.h"

#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

bool view_enabled[ VIEWCOUNT ];
bool view_cameraMode = false;
bool view_gamepadActive = false;
bool view_inverty = true;

float view_deadzone = 0.24f;

static bool  shldr_l=false;
static bool  shldr_r=false;

static float valOrbSpeed=0.0f;
static float valElvSpeed=0.0f;
static float valSclSpeed=0.0f;

static rect_t rects[ VIEWCOUNT ];

typedef struct
{
	int pointerId;
	int moved;
	float x;
	float y;
	float startx;
	float starty;
} touch_t;


static touch_t touches[ VIEWCOUNT ];

static touch_t camTouches[ 16 ];
static int numCamTouches=0;


rect_t view_rect( int nr ) 
{ 
	ASSERTM( nr >= 0 && nr < VIEWCOUNT, "nr=%d", nr );
	return rects[ nr ]; 
}


static rect_t rectMake( int x, int y, int w, int h )
{
	rect_t r;
	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;
	return r;
}


void view_init( void )
{
	for ( int i=0; i<VIEWCOUNT; ++i )
		view_enabled[ i ] = false;

	for ( int i=0; i<VIEWCOUNT; ++i )
		touches[ i ].pointerId = -1;

	for ( int i=0; i<VIEWCOUNT; ++i )
		touches[ i ].moved = 0;
}


void view_setup( int backingWidth, int backingHeight )
{
	rects[ VIEWMAIN ] = rectMake( 0, 0, backingWidth, backingHeight );

	int lh = backingHeight;
	int lw = lh * 4 / 3;
	int lmx = 0.5f * ( backingWidth - lw );
	rects[ VIEWLVLS ] = rectMake( lmx, 0, lw, lh );

	int th = backingHeight/5;
	int tw = th*2;
	rects[ VIEWTOAS ] = rectMake( backingWidth/2-tw/2, backingHeight/2-th/2, tw, th );
}


static int viewHit( int x, int y )
{
	for ( int i=0; i<VIEWCOUNT; ++i )
	{
		if ( view_enabled[ i ] )
		{
			if ( rects[ i ].x <= x && rects[ i ].y <= y && (rects[ i ].x+rects[ i ].w) >= x && (rects[ i ].y+rects[ i ].h) >= y )
				return i;
		}
	}
	return -1;
}


static int viewForPointerId( int pointerId )
{
	for ( int i=0; i<VIEWCOUNT; ++i )
		if ( touches[ i ].pointerId == pointerId )
			return i;
	return -1;
}


static bool removeCamTouch( int pointerId )
{
	for ( int j=0; j<numCamTouches; ++j )
		if ( camTouches[ j ].pointerId == pointerId )
		{
			if ( numCamTouches == 1 )
			{
				numCamTouches = 0;
				return true;
			}
			camTouches[ j ] = camTouches[ numCamTouches-1 ];
			numCamTouches--;
			return true;
		}
	return false;
}


static bool addCamTouch( int pointerId, float x, float y )
{
	for ( int j=0; j<numCamTouches; ++j )
		if ( camTouches[ j ].pointerId == pointerId )
			return false;
	camTouches[ numCamTouches ].pointerId = pointerId;
	camTouches[ numCamTouches ].x = x;
	camTouches[ numCamTouches ].y = y;
	numCamTouches++;
	return true;
}


void view_touchDown( int pointerCount, int pointerIdx, int* pointerIds, float* pointerX, float* pointerY )
{
	{
		int i = pointerIdx;
		int pointerId = pointerIds[ i ];
		float x = pointerX[ i ];
		float y = pointerY[ i ];
		int v = viewHit( x, y );
		//LOGI( "pointer %d(%d) hit view %d at %.1f,%.1f", i, pointerId, v, x, y );
		if ( v != -1 )
		{
			touches[ v ].x = touches[ v ].startx = x;
			touches[ v ].y = touches[ v ].starty = y;
			touches[ v ].pointerId = pointerId;
			touches[ v ].moved = 0;
#if defined( USETOUCH )
			char m[128];
			float nx,ny;
#endif
			switch( v )
			{
				case VIEWMAIN :
					if ( !addCamTouch( pointerId, x, y ) )
						LOGE( "Failed to add camera touch for pointerId %d. numCamTouches = %d", pointerId, numCamTouches );
					ASSERT( numCamTouches <= 16 );
					break;
			}
		}
	}
}


void view_touchUp( int pointerCount, int pointerIdx, int* pointerIds, float* pointerX, float* pointerY )
{
	{
		int i = pointerIdx;
		int pointerId = pointerIds[ i ];
		removeCamTouch( pointerId );
		int v;
		float x = pointerX[ i ];
		float y = pointerY[ i ];
		(void) x;
		(void) y;
		do
		{
			v = viewForPointerId( pointerId );
			if ( v != -1 )
			{
				//LOGI( "touch up for view %d", v );
				touches[ v ].pointerId = -1;
				switch( v )
				{
				}
			}
		} while ( v != -1 );
	}
}
	

void view_touchCancel( int pointerCount, int pointerIdx, int* pointerIds, float* pointerX, float* pointerY )
{
	view_touchUp( pointerCount, pointerIdx, pointerIds, pointerX, pointerY );
}


void view_touchMove( int pointerCount, int pointerIdx, int* pointerIds, float* pointerX, float* pointerY )
{
	for ( int i=0; i<pointerCount; ++i )
	{
		//int i = pointerIdx;
		int pointerId = pointerIds[ i ];
		float x = pointerX[ i ];
		float y = pointerY[ i ];
		char msg[80];

		// Handle camera touches
		for ( int j=0; j<numCamTouches; ++j )
			if ( camTouches[ j ].pointerId == pointerId )
			{
				if ( numCamTouches == 1 )
				{
					float dx = x - camTouches[ 0 ].x;
					float dy = y - camTouches[ 0 ].y;
					camTouches[ 0 ].x = x;
					camTouches[ 0 ].y = y;
					const float deltaX = dx / (float)rects[ VIEWMAIN ].w;
					const float deltaY = dy / (float)rects[ VIEWMAIN ].h;
        				snprintf( msg, 80, "cameraControl elevationDelta=%f orbitDelta=%f", -3*deltaY, -7*deltaX );
				        nfy_msg( msg );
				}
				else
				{
					float fr0x = camTouches[ 0 ].x;
					float fr0y = camTouches[ 0 ].y;
					float fr1x = camTouches[ 1 ].x;
					float fr1y = camTouches[ 1 ].y;
					camTouches[ j ].x = x;
					camTouches[ j ].y = y;
					float to0x = camTouches[ 0 ].x;
					float to0y = camTouches[ 0 ].y;
					float to1x = camTouches[ 1 ].x;
					float to1y = camTouches[ 1 ].y;
			                const float fr_l = sqrtf( ( fr1x - fr0x ) * ( fr1x - fr0x ) + ( fr1y - fr0y ) * ( fr1y - fr0y ) );
			                const float to_l = sqrtf( ( to1x - to0x ) * ( to1x - to0x ) + ( to1y - to0y ) * ( to1y - to0y ) );
					snprintf( msg, 80, "cameraControl distScale=%f", fr_l/to_l );
					nfy_msg( msg );
				}
			}

		int v = viewForPointerId( pointerId );
		if ( v != -1 )
		{
			//LOGI( "pointer %d(%d) moved for view %d", i, pointerId, v );
			float dx = x - touches[ v ].x;
			float dy = y - touches[ v ].y;
			touches[ v ].x = x;
			touches[ v ].y = y;
			if ( fabsf( dx ) > 4.0f || fabsf( dy ) > 4.0f )
				touches[ v ].moved = 1;
			switch( v )
			{
			}
		}
	}
}


void view_setKeyStatus( int keysym, bool pressed )
{
	if ( pressed && toast_text )
		toast_text = 0;

	//if (pressed) LOGI("keysym 0x%x", keysym);

	switch( keysym )
	{
		case 32:		// space
		case 13:		// return
		case 0x40000058:	// kp enter
			break;
		case 96:		// quote/tilde
			if ( pressed ) nfy_msg( "hudtoggle" );
			break;
		case 0x40000054:	// kp divide
			if ( pressed )
			{
				shdw_dump();
				offsc_dump();
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				offsc_dump_framebuffer("final-rgb.ppm", "final-a.ppm", "final-z.pgm", rects[VIEWMAIN].w, rects[VIEWMAIN].h);
			}
			break;
		case 0x40000055:	// kp multiply
			if ( pressed )
				nfy_msg( "camlock" );
			break;
	}
}


void view_set_scrollwheel( float v )
{
}


void view_update( float dt )
{
}


static float applyDeadZone( float value, float factor=1.0f )
{
	const float deadzone = factor * view_deadzone;
	const float workzone = ( 1.0f - deadzone );
	if ( fabsf( value ) < deadzone )
		value=0.0f;
	else
		value = ( ( value < 0.0f ) ? ( value+deadzone) : (value-deadzone) ) / workzone;
	return value;
}


void view_setControllerAxisValue( const char* axisName, float value )
{
	view_gamepadActive = true;
	if ( view_cameraMode )
	{
		if ( !strcmp( axisName, "RX" ) )
		{
			value = applyDeadZone( value );
			valOrbSpeed = -2.0f * value;
		}
		if ( !strcmp( axisName, "RY" ) )
		{
			value = applyDeadZone( value );
			valElvSpeed = 2.0f * value;
		}
		if ( !strcmp( axisName, "LY" ) )
		{
			value = applyDeadZone( value );
			valSclSpeed = value;
		}
		return;
	}
}


static void view_setCameraMode( bool enabled )
{
	view_cameraMode = enabled;
	char m[80];
	snprintf( m, sizeof(m), "camcontrol active=%d", enabled?1:0 );
	nfy_msg( m );
}


static void change_mode( void )
{
	if ( !shldr_l && !shldr_r )
	{
		// Not holding shoulder buttons: driving mode.
		if ( view_cameraMode )
			view_setCameraMode( false );
	}
	else if ( shldr_l )
	{
		// Holding left shoulder button: camera mode.
		if ( !view_cameraMode )
			view_setCameraMode( true );
	}
}


void view_setControllerButtonValue( const char* butName, bool value )
{
	view_gamepadActive = true;

	if ( value && toast_text )
		toast_text = 0;

	if ( !strcmp( butName, "BUT-A" ) )
	{
	}
	if ( !strcmp( butName, "BUT-B" ) )
	{
	}
	if ( !strcmp( butName, "BUT-Y" ) )
	{
	}
	if ( !strcmp( butName, "BUT-X" ) )
	{
	}
	if ( !strcmp( butName, "BUT-R1" ) )
	{
		shldr_r = value;
		change_mode();
	}
	if ( !strcmp( butName, "BUT-L1" ) )
	{
		shldr_l = value;
		change_mode();
	}
	if ( !strcmp( butName, "DPAD-D" ) )
	{
	}
	if ( !strcmp( butName, "DPAD-U" ) )
	{
	}
	if ( !strcmp( butName, "DPAD-L" ) )
	{
	}
	if ( !strcmp( butName, "DPAD-R" ) )
	{
	}
}

