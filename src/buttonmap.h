// buttonmap.h

#ifndef BUTTONMAP_H
#define BUTTONMAP_H

///////////////////////////////

#include "SDL/SDL.h"

///////////////////////////////

#define BUTTON_NA	-1
#define CODE_NA		-1
#define JOY_NA		-1
#define AXIS_NA		-1

///////////////////////////////

#define BUTTON_UP 		SDLK_UP
#define BUTTON_DOWN 	SDLK_DOWN
#define BUTTON_LEFT 	SDLK_LEFT
#define BUTTON_RIGHT 	SDLK_RIGHT

#define BUTTON_SELECT 	SDLK_RCTRL
#define BUTTON_START 	SDLK_RETURN

#define BUTTON_MENU	 	SDLK_ESCAPE
#define BUTTON_POWER 	SDLK_POWER

#ifdef PLATFORM_DESKTOP
    #define BUTTON_A 		SDLK_d
    #define BUTTON_B 		SDLK_s
    #define BUTTON_X 		SDLK_w
    #define BUTTON_Y 		SDLK_a

    #define BUTTON_L1 		SDLK_q
    #define BUTTON_R1 		SDLK_e
    #define BUTTON_L2 		SDLK_z
    #define BUTTON_R2 		SDLK_c

    #define	BUTTON_PLUS		SDLK_PLUS
    #define	BUTTON_MINUS	SDLK_MINUS
#else
    #define BUTTON_A 		SDLK_SPACE
    #define BUTTON_B 		SDLK_LCTRL
    #define BUTTON_X 		SDLK_LSHIFT
    #define BUTTON_Y 		SDLK_LALT
    
    #define BUTTON_L1 		SDLK_e
    #define BUTTON_R1 		SDLK_t
    #define BUTTON_L2 		SDLK_TAB
    #define BUTTON_R2 		SDLK_BACKSPACE

    #define	BUTTON_PLUS		SDLK_RSUPER
    #define	BUTTON_MINUS	SDLK_LSUPER
#endif

///////////////////////////////

#define CODE_UP			103
#define CODE_DOWN		108
#define CODE_LEFT		105
#define CODE_RIGHT		106

#define CODE_SELECT		97
#define CODE_START		28

#define CODE_A			57
#define CODE_B			29
#define CODE_X			42
#define CODE_Y			56

#define CODE_L1			18
#define CODE_R1			20
#define CODE_L2			15
#define CODE_R2			14

#define CODE_MENU		1
#define CODE_POWER		116

#define CODE_PLUS		115
#define CODE_MINUS		114

#endif