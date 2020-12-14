#include "checkogl.h"
#include "ctrl.h"
#include "ctrl_sdof.h"
#include "view.h"
#include "logx.h"
#include "shdw.h"
#include "nfy.h"
#include "kv.h"
#include "num_steps.h"

#include <SDL.h>
#include <SDL_opengl.h>

#include <stdlib.h>
#include <fenv.h>

#if defined( USESTEAM )
#	include "steam/steam_api.h"
#	include "steamstats.h"
#endif

#if 0
#	include "icon64pixels.h"
#endif

int fbw = 1280;
int fbh = 720;
int fs  = 0;

static SDL_Window *window = 0;
static SDL_GLContext glcontext = 0;
static SDL_GameController *controller = NULL;
static bool should_quit = false;

static void draw_frame( float elapsed )
{
	const int numsteps = num_120Hz_steps( elapsed );

	for ( int i=0; i<numsteps; ++i )
	{
		float dt = 1 / 120.0f;
		ctrl_update( dt );
	}

	// Render light view into texture
	shdw_use();
	ctrl_drawShadow();
	CHECK_OGL

	if ( ctrl_dof_enabled )
		ctrl_sdof_draw_offscreen();

	// Now render to the main frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fbw, fbh);
	CHECK_OGL

	ctrl_drawFrame( elapsed );
	CHECK_OGL
}


static void handle_key(SDL_KeyboardEvent keyev)
{
	const bool pressed = keyev.state == SDL_PRESSED;
	const bool repeat = keyev.repeat;
	const bool altdown = keyev.keysym.mod & KMOD_ALT;
	switch (keyev.keysym.sym)
	{
	case SDLK_ESCAPE:
		if (pressed)
			if (!ctrl_onBack())
				should_quit = true;
		break;
#if !defined( OSX )
	case SDLK_F11:
	case SDLK_RETURN:
		if (repeat) return;
		if (keyev.keysym.sym == SDLK_RETURN && !altdown) 
			return view_setKeyStatus( keyev.keysym.sym, pressed);
		LOGI("Going in/out of full screen. Pressed=%d", pressed);
		if ( pressed )
		{
			const bool was_fs = ( SDL_GetWindowFlags( window ) & SDL_WINDOW_FULLSCREEN_DESKTOP ) != 0;
			LOGI("was fullscreen: %d", was_fs);
			SDL_SetWindowFullscreen( window, was_fs ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP );
			SDL_ShowCursor( was_fs ? 1 : 0 );
		}
		break;
#endif
	default:
		view_setKeyStatus( keyev.keysym.sym, pressed );
	}
}


static void handle_controllerbutton(SDL_ControllerButtonEvent cbev)
{
	const bool pressed = cbev.state == SDL_PRESSED;
	switch (cbev.button)
	{
	case SDL_CONTROLLER_BUTTON_A:
		view_setControllerButtonValue("BUT-A", pressed);
		break;
	case SDL_CONTROLLER_BUTTON_B:
		view_setControllerButtonValue("BUT-B", pressed);
		break;
	case SDL_CONTROLLER_BUTTON_X:
		view_setControllerButtonValue("BUT-X", pressed);
		break;
	case SDL_CONTROLLER_BUTTON_Y:
		view_setControllerButtonValue("BUT-Y", pressed);
		break;
	case SDL_CONTROLLER_BUTTON_START:
		view_setControllerButtonValue("START", pressed);
		break;
	case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
		view_setControllerButtonValue("BUT-L1", pressed);
		break;
	case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
		view_setControllerButtonValue("BUT-R1", pressed);
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_UP:
		view_setControllerButtonValue("DPAD-U", pressed);
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		view_setControllerButtonValue("DPAD-D", pressed);
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
		view_setControllerButtonValue("DPAD-L", pressed);
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
		view_setControllerButtonValue("DPAD-R", pressed);
		break;
	case SDL_CONTROLLER_BUTTON_BACK:
		if (pressed)
			if (!ctrl_onBack())
				should_quit = true;
		break;
	}
}


static void handle_controlleraxismotion(SDL_ControllerAxisEvent axev )
{
	const float v = axev.value > 0 ? axev.value / 32767.0f : axev.value / 32768.0f;
	switch (axev.axis)
	{
	case SDL_CONTROLLER_AXIS_LEFTX:
		view_setControllerAxisValue("LX", v);
		break;
	case SDL_CONTROLLER_AXIS_LEFTY:
		view_setControllerAxisValue("LY", v);
		break;
	case SDL_CONTROLLER_AXIS_RIGHTX:
		view_setControllerAxisValue("RX", v);
		break;
	case SDL_CONTROLLER_AXIS_RIGHTY:
		view_setControllerAxisValue("RY", v);
		break;
	case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
		view_setControllerAxisValue("LT", v*2-1);
		break;
	case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
		view_setControllerAxisValue("RT", v*2-1);
		break;
	}
}


static int pointerId = 0;
static void handle_mousebutton(SDL_MouseButtonEvent mbev)
{
	const bool down = mbev.state == SDL_PRESSED;
	float mousex = mbev.x;
	float mousey = fbh - 1 - mbev.y;
	if (down)
		view_touchDown(1, 0, &pointerId, &mousex, &mousey);
	else
		view_touchUp(1, 0, &pointerId, &mousex, &mousey);
}


static void handle_mousemotion(SDL_MouseMotionEvent mmev)
{
	float mousex = mmev.x;
	float mousey = fbh - 1 - mmev.y;
	view_touchMove(1, 0, &pointerId, &mousex, &mousey);
}


static void handle_mousewheel(SDL_MouseWheelEvent mwev)
{
	const float scl = 1.0f - 0.05f * mwev.y;
	char m[80];
	snprintf(m, sizeof(m), "cameraControl distScale=%f", scl);
	nfy_msg(m);
}


static void present_assert(const char* condition, const char* file, int line)
{
	char text[512];
	snprintf(text, sizeof(text), "Assertion failed:\n%s\n%s(%d)", condition, file, line);
	SDL_ShowSimpleMessageBox
	(
		SDL_MESSAGEBOX_ERROR,
		"Assertion Failure",
		text,
		NULL
	);
}


static bool add_joystick( int idx )
{
	const int numjoy = SDL_NumJoysticks();
	LOGI("Number of joysticks: %d", numjoy);
	ASSERT( idx < numjoy );
	if ( SDL_IsGameController( idx ) )
	{
		controller = SDL_GameControllerOpen( idx );
		if (!controller)
		{
			LOGE("Could not open gamecontroller %i: %s", idx, SDL_GetError());
			return false;
		}
		LOGI( "Using controller %s", SDL_GameControllerName(controller) );
		return true;
	}
	else
	{
		LOGI( "Joystick %d is not a gamecontroller. Not using it.", idx );
		return false;
	}
}


static bool rmv_joystick( int idx )
{
	if ( SDL_IsGameController( idx ) )
	{
		LOGI( "Game Controller was removed." );
	}
	else
	{
		return false;
	}
	return true;
}


int main(int argc, char* argv[])
{
	ctrl_configPath = ".";
	asserthook = present_assert;
#if defined( USESTEAM )
	if ( SteamAPI_RestartAppIfNecessary( 361430 ) )
	{
		LOGI( "Attempting a restart via steam." );
		exit(1);
	}
	bool steam_initialized = SteamAPI_Init();
	LOGI( "%s initialize steam.", steam_initialized ? "Could" : "Could not" );
	if (!steam_initialized || !SteamUser())
	{
		SDL_ShowSimpleMessageBox
			(
			SDL_MESSAGEBOX_ERROR,
			"Missing Steam Client",
			"You need to be running the Steam client to play this game.\n"
			"The reason for this is that without Steam running, this game does not know where on your harddisk to save the game data.\n",
			NULL
			);
		exit( 101 );
	}
	static char userfolder[ 256 ];
	userfolder[0] = 0;
	const bool gotfolder = SteamUser()->GetUserDataFolder( userfolder, sizeof(userfolder) );
	if (!gotfolder)
	{
		LOGE("Failed to get the user data folder from steam.");
	}
	else
	{
		LOGI("User data folder is '%s'", userfolder);
		ctrl_configPath = userfolder;
		char logfname[ 256 ];
		snprintf( logfname, sizeof( logfname ), "%s/cranelog.txt", ctrl_configPath );
		logx_file = fopen( logfname, "w" );
		LOGI( "%s log file.", logx_file ? "Opened" : "Could not open" );
	}
	steamstats_init();
#endif

	for (int i = 1; i < argc; ++i)
	{
		const char* a = argv[i];
		if ( !strncmp( a, "completeframebufferfix=", 23 ) )
		{
			shdw_completeframebufferfix = atoi(a + 23);
		}
	}

	// Hard stop on NaN.
	feenableexcept( FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW );

	const int initerr0 = SDL_InitSubSystem( SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER );
	if ( initerr0 ) LOGE( "Failed to initialize joystick and game controller subsystem of SDL2: %s", SDL_GetError() );

	const int initerr1 = SDL_InitSubSystem( SDL_INIT_TIMER );
	if ( initerr1 ) LOGE( "Failed to initialize timer subsystem of SDL2: %s", SDL_GetError() );

	const int initerr2 = SDL_InitSubSystem( SDL_INIT_VIDEO );
	if ( initerr2 ) LOGE( "Failed to initialize video subsystem of SDL2: %s", SDL_GetError() );

	// Before we create the window, we set the attributes that we require from it.
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	// This causes problems for some linux machines. Why???
	//SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
	SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);

	// See what resolution the desktop is.
	SDL_DisplayMode dm;
	if (SDL_GetDesktopDisplayMode(0, &dm) != 0)
	{
		LOGE( "SDL_GetDesktopDisplayMode failed: %s", SDL_GetError() );
	}
	else
	{
		// Use desktop resolution.
		LOGI( "The desktop resolution is %dx%d", dm.w, dm.h );
	}

	// Create a window. Window mode MUST include SDL_WINDOW_OPENGL for use with OpenGL.
	window = SDL_CreateWindow(
		"The Little Crane That Could",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		fbw, fbh,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
	);
	ASSERT( window );

#if 0
	SDL_Surface* surface = SDL_CreateRGBSurfaceFrom( (void*) icon64pixels, 64, 64, 16, 64 * 2, 0x0f00, 0x00f0, 0x000f, 0xf000 );
	if (surface)
	{
		SDL_SetWindowIcon( window, surface );
		SDL_FreeSurface( surface );
	}
#endif
	// Create an OpenGL context associated with the window.
	glcontext = SDL_GL_CreateContext( window );
	if ( !glcontext )
	{
		SDL_ShowSimpleMessageBox
		(
			SDL_MESSAGEBOX_ERROR,
			"Missing GL context",
			"SDL_GL_CreateContext() failed.\n",
			NULL
		);
		exit( 102 );
	}
	else
	{
		LOGI( "Successfully Created OpenGL context." );
	}

	if (fs)
	{
		SDL_SetWindowFullscreen( window, SDL_WINDOW_FULLSCREEN_DESKTOP );
		//SDL_ShowCursor( 0 );
		//SDL_SetRelativeMouseMode( SDL_TRUE );
	}

	SDL_GL_GetDrawableSize( window, &fbw, &fbh );
	LOGI( "drawable size: %dx%d", fbw, fbh );

	SDL_GL_SetSwapInterval(1);

	/* Open the first available controller. */
	const int numjoy = SDL_NumJoysticks();
	LOGI("Number of joysticks: %d", numjoy);
	const char* db_fname = "gamecontrollerdb.txt";
	const int mappings_added = SDL_GameControllerAddMappingsFromFile( db_fname );
	LOGI( "Added %d gamecontroller mappings.", mappings_added );

#if defined( MSWIN )
	const int version = gl3wInit();
	LOGI("version of gl3w: %d", version);
#endif

	kv_init( ctrl_configPath );
	
	int w=0, h=0;
	SDL_GL_GetDrawableSize( window, &w, &h );
	const float csf = w / (float)fbw;
	ctrl_create( fbw, fbh, csf );

	//nfy_msg("start nr=0 tag=quickbattle");

	SDL_Event e;
	do
	{
		int gotevent;
		do
		{
			gotevent = SDL_PollEvent(&e);
			if (!gotevent)
				break;

			if (e.type == SDL_WINDOWEVENT)
			{
				int w,h;
				char msg[128];
				switch (e.window.event)
				{
					case SDL_WINDOWEVENT_RESIZED:
						fbw = e.window.data1;
						fbh = e.window.data2;
						SDL_GL_GetDrawableSize( window, &w, &h );
						const float csf = w / (float)fbw;
						snprintf( msg, sizeof(msg), "aspect ratio=%f width=%d height=%d csf=%f", fbw / (float)fbh, w, h, csf );
						nfy_msg( msg );
						break;
				}
			}
			if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
			{
				handle_key(e.key);
			}
			if (e.type == SDL_CONTROLLERBUTTONDOWN || e.type == SDL_CONTROLLERBUTTONUP)
			{
				handle_controllerbutton(e.cbutton);
			}
			if (e.type == SDL_CONTROLLERAXISMOTION)
			{
				handle_controlleraxismotion(e.caxis);
			}
			if (e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEBUTTONDOWN)
			{
				handle_mousebutton(e.button);
			}
			if (e.type == SDL_MOUSEMOTION)
			{
				handle_mousemotion(e.motion);
			}
			if (e.type == SDL_MOUSEWHEEL)
			{
				handle_mousewheel(e.wheel);
			}
			if (e.type == SDL_JOYDEVICEADDED)
			{
				LOGI( "Joystick device (id %d) was added.", e.cdevice.which );
				add_joystick( e.cdevice.which );
			}
			if (e.type == SDL_JOYDEVICEREMOVED)
			{
				LOGI( "Joystick device (id %d) ws removed.", e.cdevice.which );
				rmv_joystick( e.cdevice.which );
			}
		} while (gotevent);
    
		glClearColor( 0.6, 0.7, 1.0, 1.0 );
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);   

		static float prevtim = SDL_GetTicks();
		float currtim = SDL_GetTicks();
		const float elapsed = ( currtim - prevtim ) / 1000.0f;
		prevtim = currtim;
		draw_frame( elapsed );

#if defined( USESTEAM )
		// Give SteamAPI the opportunity to fire off some callbacks.
		SteamAPI_RunCallbacks();
#endif

		SDL_GL_SwapWindow(window);	// Swap the window/buffer to display the result.
		//SDL_Delay(10);		// Pause briefly before moving on to the next cycle.
	} while ( e.type != SDL_QUIT && !should_quit );

	ctrl_destroy();

	// Once finished with OpenGL functions, the SDL_GLContext can be deleted.
	SDL_GL_DeleteContext(glcontext);  

	// Done! Close the window, clean-up and exit the program. 
	SDL_DestroyWindow(window);

	LOGI( "Shutting down SDL2 framework." );
	SDL_Quit();

#if defined( USESTEAM )
	LOGI( "Shutting down SteamAPI." );
	SteamAPI_Shutdown();
#endif

	return 0;
}

