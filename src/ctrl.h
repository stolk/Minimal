#ifndef CTRL_H
#define CTRL_H

extern void ctrl_create( int backingW, int backingH, float csf=1.0f );

extern void ctrl_destroy( void );

extern void ctrl_drawShadow( void );

extern const char* ctrl_drawFrame( float dt );

extern void ctrl_pause( void );

extern void ctrl_update( float dt );

extern bool ctrl_onBack( void );

extern const char*	ctrl_bundlePath;
extern const char*	ctrl_configPath;
extern bool		ctrl_dof_enabled;

#endif

