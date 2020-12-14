#ifndef VIEW_H
#define VIEW_H

enum
{
	// GENERIC
	VIEWLVLS,
	VIEWTOAS,
	VIEWMAIN,

	VIEWCOUNT
};

typedef struct 
{
	int x;
	int y;
	int w;
	int h;
} rect_t;


extern bool view_enabled[ VIEWCOUNT ];

extern bool view_gamepadActive;

extern bool view_inverty;

extern bool view_rstickthrottle;

extern float view_deadzone;

extern rect_t view_rect( int nr );

extern void view_init( void );

extern void view_setup( int backingW, int backingH );

extern void view_touchDown( int pointerCount, int pointerIdx, int* pointerIds, float* pointerX, float* pointerY );

extern void view_touchUp( int pointerCount, int pointerIdx, int* pointerIds, float* pointerX, float* pointerY );

extern void view_touchCancel( int pointerCount, int pointerIdx, int* pointerIds, float* pointerX, float* pointerY );

extern void view_touchMove( int pointerCount, int pointerIdx, int* pointerIds, float* pointerX, float* pointerY );

//! Keyboard information is fed into this.
extern void view_setKeyStatus( int keysym, bool down );

//! Gamepad joystick information is fed into this.
extern void view_setControllerAxisValue( const char* axisName, float value );

//! Gamepad button information is fed into this.
extern void view_setControllerButtonValue( const char* butName, bool value );

//! Scrollwheel information is fed into this.
extern void view_set_scrollwheel( float v );

extern void view_update( float dt );

#endif

