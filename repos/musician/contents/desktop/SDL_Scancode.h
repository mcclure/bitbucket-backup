#ifndef SDL_SCANCODE_HEADER
#define SDL_SCANCODE_HEADER

/*
 *  SDL_Scancode.h
 *  Jumpcore
 *
 *  Composed from SDL2 sources.
 *
 */

#if WINDOWS

/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2013 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../include/SDL_scancode.h"

/* Windows scancode to SDL scancode mapping table */
/* derived from Microsoft scan code document, http://download.microsoft.com/download/1/6/1/161ba512-40e2-4cc9-843a-923143f3456c/scancode.doc */

/* *INDENT-OFF* */
enum SDL_Scancode 
{ 
	/*	0						1							2							3							4						5							6							7 */
	/*	8						9							A							B							C						D							E							F */
	SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_ESCAPE,		SDL_SCANCODE_1,				SDL_SCANCODE_2,				SDL_SCANCODE_3,			SDL_SCANCODE_4,				SDL_SCANCODE_5,				SDL_SCANCODE_6,			/* 0 */
	SDL_SCANCODE_7,				SDL_SCANCODE_8,				SDL_SCANCODE_9,				SDL_SCANCODE_0,				SDL_SCANCODE_MINUS,		SDL_SCANCODE_EQUALS,		SDL_SCANCODE_BACKSPACE,		SDL_SCANCODE_TAB,		/* 0 */

	SDL_SCANCODE_Q,				SDL_SCANCODE_W,				SDL_SCANCODE_E,				SDL_SCANCODE_R,				SDL_SCANCODE_T,			SDL_SCANCODE_Y,				SDL_SCANCODE_U,				SDL_SCANCODE_I,			/* 1 */
	SDL_SCANCODE_O,				SDL_SCANCODE_P,				SDL_SCANCODE_LEFTBRACKET,	SDL_SCANCODE_RIGHTBRACKET,	SDL_SCANCODE_RETURN,	SDL_SCANCODE_LCTRL,			SDL_SCANCODE_A,				SDL_SCANCODE_S,			/* 1 */

	SDL_SCANCODE_D,				SDL_SCANCODE_F,				SDL_SCANCODE_G,				SDL_SCANCODE_H,				SDL_SCANCODE_J,			SDL_SCANCODE_K,				SDL_SCANCODE_L,				SDL_SCANCODE_SEMICOLON,	/* 2 */
	SDL_SCANCODE_APOSTROPHE,	SDL_SCANCODE_GRAVE,			SDL_SCANCODE_LSHIFT,		SDL_SCANCODE_BACKSLASH,		SDL_SCANCODE_Z,			SDL_SCANCODE_X,				SDL_SCANCODE_C,				SDL_SCANCODE_V,			/* 2 */

	SDL_SCANCODE_B,				SDL_SCANCODE_N,				SDL_SCANCODE_M,				SDL_SCANCODE_COMMA,			SDL_SCANCODE_PERIOD,	SDL_SCANCODE_SLASH,			SDL_SCANCODE_RSHIFT,		SDL_SCANCODE_PRINTSCREEN,/* 3 */
	SDL_SCANCODE_LALT,			SDL_SCANCODE_SPACE,			SDL_SCANCODE_CAPSLOCK,		SDL_SCANCODE_F1,			SDL_SCANCODE_F2,		SDL_SCANCODE_F3,			SDL_SCANCODE_F4,			SDL_SCANCODE_F5,		/* 3 */

	SDL_SCANCODE_F6,			SDL_SCANCODE_F7,			SDL_SCANCODE_F8,			SDL_SCANCODE_F9,			SDL_SCANCODE_F10,		SDL_SCANCODE_NUMLOCKCLEAR,	SDL_SCANCODE_SCROLLLOCK,	SDL_SCANCODE_HOME,		/* 4 */
	SDL_SCANCODE_UP,			SDL_SCANCODE_PAGEUP,		SDL_SCANCODE_KP_MINUS,		SDL_SCANCODE_LEFT,			SDL_SCANCODE_KP_5,		SDL_SCANCODE_RIGHT,			SDL_SCANCODE_KP_PLUS,		SDL_SCANCODE_END,		/* 4 */

	SDL_SCANCODE_DOWN,			SDL_SCANCODE_PAGEDOWN,		SDL_SCANCODE_INSERT,		SDL_SCANCODE_DELETE,		SDL_SCANCODE_UNKNOWN_2,	SDL_SCANCODE_UNKNOWN_3,		SDL_SCANCODE_NONUSBACKSLASH,SDL_SCANCODE_F11,		/* 5 */
	SDL_SCANCODE_F12,			SDL_SCANCODE_PAUSE,			SDL_SCANCODE_UNKNOWN_4,		SDL_SCANCODE_LGUI,			SDL_SCANCODE_RGUI,		SDL_SCANCODE_APPLICATION,	SDL_SCANCODE_UNKNOWN_5,		SDL_SCANCODE_UNKNOWN_6,	/* 5 */

	SDL_SCANCODE_UNKNOWN_7,		SDL_SCANCODE_UNKNOWN_8,		SDL_SCANCODE_UNKNOWN_9,		SDL_SCANCODE_UNKNOWN_10,	SDL_SCANCODE_F13,		SDL_SCANCODE_F14,			SDL_SCANCODE_F15,			SDL_SCANCODE_F16,		/* 6 */
	SDL_SCANCODE_F17,			SDL_SCANCODE_F18,			SDL_SCANCODE_F19,			SDL_SCANCODE_UNKNOWN_11,	SDL_SCANCODE_UNKNOWN_12,SDL_SCANCODE_UNKNOWN_12,	SDL_SCANCODE_UNKNOWN_13,	SDL_SCANCODE_UNKNOWN_14,/* 6 */
	
	SDL_SCANCODE_UNKNOWN_15,	SDL_SCANCODE_UNKNOWN_16,	SDL_SCANCODE_UNKNOWN_17,	SDL_SCANCODE_UNKNOWN_18,	SDL_SCANCODE_UNKNOWN_19,SDL_SCANCODE_UNKNOWN_20,	SDL_SCANCODE_UNKNOWN_21,	SDL_SCANCODE_UNKNOWN_22,/* 7 */
	SDL_SCANCODE_UNKNOWN_23,	SDL_SCANCODE_UNKNOWN_24,	SDL_SCANCODE_UNKNOWN_25,	SDL_SCANCODE_UNKNOWN,		SDL_SCANCODE_UNKNOWN_26,SDL_SCANCODE_UNKNOWN_27,	SDL_SCANCODE_UNKNOWN_28,	SDL_SCANCODE_UNKNOWN_29	/* 7 */
};
/* *INDENT-ON* */

#elsif LINUX

/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2013 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../include/SDL_scancode.h"

/* Linux virtual key code to SDL_Keycode mapping table
   Sources:
   - Linux kernel source input.h
*/
/* *INDENT-OFF* */
enum SDL_Scancode {
    /*  0 */    SDL_SCANCODE_UNKNOWN,
    /*  1 */    SDL_SCANCODE_ESCAPE,
    /*  2 */    SDL_SCANCODE_1,
    /*  3 */    SDL_SCANCODE_2,
    /*  4 */    SDL_SCANCODE_3,
    /*  5 */    SDL_SCANCODE_4,
    /*  6 */    SDL_SCANCODE_5,
    /*  7 */    SDL_SCANCODE_6,
    /*  8 */    SDL_SCANCODE_7,
    /*  9 */    SDL_SCANCODE_8,
    /*  10 */    SDL_SCANCODE_9,
    /*  11 */    SDL_SCANCODE_0,
    /*  12 */    SDL_SCANCODE_MINUS,
    /*  13 */    SDL_SCANCODE_EQUALS,
    /*  14 */    SDL_SCANCODE_BACKSPACE,
    /*  15 */    SDL_SCANCODE_TAB,
    /*  16 */    SDL_SCANCODE_Q,
    /*  17 */    SDL_SCANCODE_W,
    /*  18 */    SDL_SCANCODE_E,
    /*  19 */    SDL_SCANCODE_R,
    /*  20 */    SDL_SCANCODE_T,
    /*  21 */    SDL_SCANCODE_Y,
    /*  22 */    SDL_SCANCODE_U,
    /*  23 */    SDL_SCANCODE_I,
    /*  24 */    SDL_SCANCODE_O,
    /*  25 */    SDL_SCANCODE_P,
    /*  26 */    SDL_SCANCODE_LEFTBRACKET,
    /*  27 */    SDL_SCANCODE_RIGHTBRACKET,
    /*  28 */    SDL_SCANCODE_RETURN,
    /*  29 */    SDL_SCANCODE_LCTRL,
    /*  30 */    SDL_SCANCODE_A,
    /*  31 */    SDL_SCANCODE_S,
    /*  32 */    SDL_SCANCODE_D,
    /*  33 */    SDL_SCANCODE_F,
    /*  34 */    SDL_SCANCODE_G,
    /*  35 */    SDL_SCANCODE_H,
    /*  36 */    SDL_SCANCODE_J,
    /*  37 */    SDL_SCANCODE_K,
    /*  38 */    SDL_SCANCODE_L,
    /*  39 */    SDL_SCANCODE_SEMICOLON,
    /*  40 */    SDL_SCANCODE_APOSTROPHE,
    /*  41 */    SDL_SCANCODE_GRAVE,
    /*  42 */    SDL_SCANCODE_LSHIFT,
    /*  43 */    SDL_SCANCODE_BACKSLASH,
    /*  44 */    SDL_SCANCODE_Z,
    /*  45 */    SDL_SCANCODE_X,
    /*  46 */    SDL_SCANCODE_C,
    /*  47 */    SDL_SCANCODE_V,
    /*  48 */    SDL_SCANCODE_B,
    /*  49 */    SDL_SCANCODE_N,
    /*  50 */    SDL_SCANCODE_M,
    /*  51 */    SDL_SCANCODE_COMMA,
    /*  52 */    SDL_SCANCODE_PERIOD,
    /*  53 */    SDL_SCANCODE_SLASH,
    /*  54 */    SDL_SCANCODE_RSHIFT,
    /*  55 */    SDL_SCANCODE_KP_MULTIPLY,
    /*  56 */    SDL_SCANCODE_LALT,
    /*  57 */    SDL_SCANCODE_SPACE,
    /*  58 */    SDL_SCANCODE_CAPSLOCK,
    /*  59 */    SDL_SCANCODE_F1,
    /*  60 */    SDL_SCANCODE_F2,
    /*  61 */    SDL_SCANCODE_F3,
    /*  62 */    SDL_SCANCODE_F4,
    /*  63 */    SDL_SCANCODE_F5,
    /*  64 */    SDL_SCANCODE_F6,
    /*  65 */    SDL_SCANCODE_F7,
    /*  66 */    SDL_SCANCODE_F8,
    /*  67 */    SDL_SCANCODE_F9,
    /*  68 */    SDL_SCANCODE_F10,
    /*  69 */    SDL_SCANCODE_NUMLOCKCLEAR,
    /*  70 */    SDL_SCANCODE_SCROLLLOCK,
    /*  71 */    SDL_SCANCODE_KP_7,
    /*  72 */    SDL_SCANCODE_KP_8,
    /*  73 */    SDL_SCANCODE_KP_9,
    /*  74 */    SDL_SCANCODE_KP_MINUS,
    /*  75 */    SDL_SCANCODE_KP_4,
    /*  76 */    SDL_SCANCODE_KP_5,
    /*  77 */    SDL_SCANCODE_KP_6,
    /*  78 */    SDL_SCANCODE_KP_PLUS,
    /*  79 */    SDL_SCANCODE_KP_1,
    /*  80 */    SDL_SCANCODE_KP_2,
    /*  81 */    SDL_SCANCODE_KP_3,
    /*  82 */    SDL_SCANCODE_KP_0,
    /*  83 */    SDL_SCANCODE_KP_PERIOD,
    0,
    /*  85 */    SDL_SCANCODE_UNKNOWN_2, /* KEY_ZENKAKUHANKAKU */
    /*  86 */    SDL_SCANCODE_NONUSBACKSLASH, /* KEY_102ND */
    /*  87 */    SDL_SCANCODE_F11,
    /*  88 */    SDL_SCANCODE_F12,
    /*  89 */    SDL_SCANCODE_INTERNATIONAL1, /* KEY_RO */
    /*  90 */    SDL_SCANCODE_LANG3, /* KEY_KATAKANA */
    /*  91 */    SDL_SCANCODE_LANG4, /* KEY_HIRAGANA */
    /*  92 */    SDL_SCANCODE_INTERNATIONAL4, /* KEY_HENKAN */
    /*  93 */    SDL_SCANCODE_INTERNATIONAL2, /* KEY_KATAKANAHIRAGANA */
    /*  94 */    SDL_SCANCODE_INTERNATIONAL5, /* KEY_MUHENKAN */
    /*  95 */    SDL_SCANCODE_INTERNATIONAL5, /* KEY_KPJPCOMMA */
    /*  96 */    SDL_SCANCODE_KP_ENTER,
    /*  97 */    SDL_SCANCODE_RCTRL,
    /*  98 */    SDL_SCANCODE_KP_DIVIDE,
    /*  99 */    SDL_SCANCODE_SYSREQ,
    /*  100 */    SDL_SCANCODE_RALT,
    /*  101 */    SDL_SCANCODE_UNKNOWN_3, /* KEY_LINEFEED */
    /*  102 */    SDL_SCANCODE_HOME,
    /*  103 */    SDL_SCANCODE_UP,
    /*  104 */    SDL_SCANCODE_PAGEUP,
    /*  105 */    SDL_SCANCODE_LEFT,
    /*  106 */    SDL_SCANCODE_RIGHT,
    /*  107 */    SDL_SCANCODE_END,
    /*  108 */    SDL_SCANCODE_DOWN,
    /*  109 */    SDL_SCANCODE_PAGEDOWN,
    /*  110 */    SDL_SCANCODE_INSERT,
    /*  111 */    SDL_SCANCODE_DELETE,
    /*  112 */    SDL_SCANCODE_UNKNOWN_4, /* KEY_MACRO */
    /*  113 */    SDL_SCANCODE_MUTE,
    /*  114 */    SDL_SCANCODE_VOLUMEDOWN,
    /*  115 */    SDL_SCANCODE_VOLUMEUP,
    /*  116 */    SDL_SCANCODE_POWER,
    /*  117 */    SDL_SCANCODE_KP_EQUALS,
    /*  118 */    SDL_SCANCODE_KP_PLUSMINUS,
    /*  119 */    SDL_SCANCODE_PAUSE,
    0,
    /*  121 */    SDL_SCANCODE_KP_COMMA,
    /*  122 */    SDL_SCANCODE_LANG1, /* KEY_HANGUEL */
    /*  123 */    SDL_SCANCODE_LANG2, /* KEY_HANJA */
    /*  124 */    SDL_SCANCODE_INTERNATIONAL3, /* KEY_YEN */
    /*  125 */    SDL_SCANCODE_LGUI,
    /*  126 */    SDL_SCANCODE_RGUI,
    /*  127 */    SDL_SCANCODE_UNKNOWN_5, /* KEY_COMPOSE */
    /*  128 */    SDL_SCANCODE_STOP,
    /*  129 */    SDL_SCANCODE_AGAIN,
    /*  130 */    SDL_SCANCODE_UNKNOWN_6, /* KEY_PROPS */
    /*  131 */    SDL_SCANCODE_UNDO,
    /*  132 */    SDL_SCANCODE_UNKNOWN_7, /* KEY_FRONT */
    /*  133 */    SDL_SCANCODE_COPY,
    /*  134 */    SDL_SCANCODE_UNKNOWN_8, /* KEY_OPEN */
    /*  135 */    SDL_SCANCODE_PASTE,
    /*  136 */    SDL_SCANCODE_FIND,
    /*  137 */    SDL_SCANCODE_CUT,
    /*  138 */    SDL_SCANCODE_HELP,
    /*  139 */    SDL_SCANCODE_MENU,
    /*  140 */    SDL_SCANCODE_CALCULATOR,
    /*  141 */    SDL_SCANCODE_UNKNOWN_9, /* KEY_SETUP */
    /*  142 */    SDL_SCANCODE_SLEEP,
    /*  143 */    SDL_SCANCODE_UNKNOWN_10, /* KEY_WAKEUP */
    /*  144 */    SDL_SCANCODE_UNKNOWN_11, /* KEY_FILE */
    /*  145 */    SDL_SCANCODE_UNKNOWN_12, /* KEY_SENDFILE */
    /*  146 */    SDL_SCANCODE_UNKNOWN_13, /* KEY_DELETEFILE */
    /*  147 */    SDL_SCANCODE_UNKNOWN_14, /* KEY_XFER */
    /*  148 */    SDL_SCANCODE_UNKNOWN_15, /* KEY_PROG1 */
    /*  149 */    SDL_SCANCODE_UNKNOWN_16, /* KEY_PROG2 */
    /*  150 */    SDL_SCANCODE_UNKNOWN_17, /* KEY_WWW */
    /*  151 */    SDL_SCANCODE_UNKNOWN_18, /* KEY_MSDOS */
    /*  152 */    SDL_SCANCODE_UNKNOWN_19, /* KEY_COFFEE */
    /*  153 */    SDL_SCANCODE_UNKNOWN_20, /* KEY_DIRECTION */
    /*  154 */    SDL_SCANCODE_UNKNOWN_21, /* KEY_CYCLEWINDOWS */
    /*  155 */    SDL_SCANCODE_MAIL,
    /*  156 */    SDL_SCANCODE_AC_BOOKMARKS,
    /*  157 */    SDL_SCANCODE_COMPUTER,
    /*  158 */    SDL_SCANCODE_AC_BACK,
    /*  159 */    SDL_SCANCODE_AC_FORWARD,
    /*  160 */    SDL_SCANCODE_UNKNOWN_22, /* KEY_CLOSECD */
    /*  161 */    SDL_SCANCODE_EJECT, /* KEY_EJECTCD */
    /*  162 */    SDL_SCANCODE_UNKNOWN_23, /* KEY_EJECTCLOSECD */
    /*  163 */    SDL_SCANCODE_AUDIONEXT, /* KEY_NEXTSONG */
    /*  164 */    SDL_SCANCODE_AUDIOPLAY, /* KEY_PLAYPAUSE */
    /*  165 */    SDL_SCANCODE_AUDIOPREV, /* KEY_PREVIOUSSONG */
    /*  166 */    SDL_SCANCODE_UNKNOWN_24, /* KEY_STOPCD */
    /*  167 */    SDL_SCANCODE_UNKNOWN_25, /* KEY_RECORD */
    /*  168 */    SDL_SCANCODE_UNKNOWN_26, /* KEY_REWIND */
    /*  169 */    SDL_SCANCODE_UNKNOWN_27, /* KEY_PHONE */
    /*  170 */    SDL_SCANCODE_UNKNOWN_28, /* KEY_ISO */
    /*  171 */    SDL_SCANCODE_UNKNOWN_29, /* KEY_CONFIG */
    /*  172 */    SDL_SCANCODE_AC_HOME,
    /*  173 */    SDL_SCANCODE_AC_REFRESH,
    /*  174 */    SDL_SCANCODE_UNKNOWN_30, /* KEY_EXIT */
    /*  175 */    SDL_SCANCODE_UNKNOWN_31, /* KEY_MOVE */
    /*  176 */    SDL_SCANCODE_UNKNOWN_32, /* KEY_EDIT */
    /*  177 */    SDL_SCANCODE_UNKNOWN_33, /* KEY_SCROLLUP */
    /*  178 */    SDL_SCANCODE_UNKNOWN_34, /* KEY_SCROLLDOWN */
    /*  179 */    SDL_SCANCODE_KP_LEFTPAREN,
    /*  180 */    SDL_SCANCODE_KP_RIGHTPAREN,
    /*  181 */    SDL_SCANCODE_UNKNOWN_35, /* KEY_NEW */
    /*  182 */    SDL_SCANCODE_UNKNOWN_36, /* KEY_REDO */
    /*  183 */    SDL_SCANCODE_F13,
    /*  184 */    SDL_SCANCODE_F14,
    /*  185 */    SDL_SCANCODE_F15,
    /*  186 */    SDL_SCANCODE_F16,
    /*  187 */    SDL_SCANCODE_F17,
    /*  188 */    SDL_SCANCODE_F18,
    /*  189 */    SDL_SCANCODE_F19,
    /*  190 */    SDL_SCANCODE_F20,
    /*  191 */    SDL_SCANCODE_F21,
    /*  192 */    SDL_SCANCODE_F22,
    /*  193 */    SDL_SCANCODE_F23,
    /*  194 */    SDL_SCANCODE_F24,
    0, 0, 0, 0,
    /*  200 */    SDL_SCANCODE_UNKNOWN_37, /* KEY_PLAYCD */
    /*  201 */    SDL_SCANCODE_UNKNOWN_38, /* KEY_PAUSECD */
    /*  202 */    SDL_SCANCODE_UNKNOWN_39, /* KEY_PROG3 */
    /*  203 */    SDL_SCANCODE_UNKNOWN_40, /* KEY_PROG4 */
    0,
    /*  205 */    SDL_SCANCODE_UNKNOWN_41, /* KEY_SUSPEND */
    /*  206 */    SDL_SCANCODE_UNKNOWN_42, /* KEY_CLOSE */
    /*  207 */    SDL_SCANCODE_UNKNOWN_43, /* KEY_PLAY */
    /*  208 */    SDL_SCANCODE_UNKNOWN_44, /* KEY_FASTFORWARD */
    /*  209 */    SDL_SCANCODE_UNKNOWN_45, /* KEY_BASSBOOST */
    /*  210 */    SDL_SCANCODE_UNKNOWN_46, /* KEY_PRINT */
    /*  211 */    SDL_SCANCODE_UNKNOWN_47, /* KEY_HP */
    /*  212 */    SDL_SCANCODE_UNKNOWN_48, /* KEY_CAMERA */
    /*  213 */    SDL_SCANCODE_UNKNOWN_49, /* KEY_SOUND */
    /*  214 */    SDL_SCANCODE_UNKNOWN_50, /* KEY_QUESTION */
    /*  215 */    SDL_SCANCODE_UNKNOWN_51, /* KEY_EMAIL */
    /*  216 */    SDL_SCANCODE_UNKNOWN_52, /* KEY_CHAT */
    /*  217 */    SDL_SCANCODE_AC_SEARCH,
    /*  218 */    SDL_SCANCODE_UNKNOWN_53, /* KEY_CONNECT */
    /*  219 */    SDL_SCANCODE_UNKNOWN_54, /* KEY_FINANCE */
    /*  220 */    SDL_SCANCODE_UNKNOWN_55, /* KEY_SPORT */
    /*  221 */    SDL_SCANCODE_UNKNOWN_56, /* KEY_SHOP */
    /*  222 */    SDL_SCANCODE_ALTERASE,
    /*  223 */    SDL_SCANCODE_CANCEL,
    /*  224 */    SDL_SCANCODE_BRIGHTNESSDOWN,
    /*  225 */    SDL_SCANCODE_BRIGHTNESSUP,
    /*  226 */    SDL_SCANCODE_UNKNOWN_57, /* KEY_MEDIA */
    /*  227 */    SDL_SCANCODE_DISPLAYSWITCH, /* KEY_SWITCHVIDEOMODE */
    /*  228 */    SDL_SCANCODE_KBDILLUMTOGGLE,
    /*  229 */    SDL_SCANCODE_KBDILLUMDOWN,
    /*  230 */    SDL_SCANCODE_KBDILLUMUP,
    /*  231 */    SDL_SCANCODE_UNKNOWN_58, /* KEY_SEND */
    /*  232 */    SDL_SCANCODE_UNKNOWN_59, /* KEY_REPLY */
    /*  233 */    SDL_SCANCODE_UNKNOWN_60, /* KEY_FORWARDMAIL */
    /*  234 */    SDL_SCANCODE_UNKNOWN_61, /* KEY_SAVE */
    /*  235 */    SDL_SCANCODE_UNKNOWN_62, /* KEY_DOCUMENTS */
    /*  236 */    SDL_SCANCODE_UNKNOWN_63, /* KEY_BATTERY */
};
/* *INDENT-ON* */

#else // APPLE

/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2013 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/* Mac virtual key code to SDL scancode mapping table
   Sources:
   - Inside Macintosh: Text <http://developer.apple.com/documentation/mac/Text/Text-571.html>
   - Apple USB keyboard driver source <http://darwinsource.opendarwin.org/10.4.6.ppc/IOHIDFamily-172.8/IOHIDFamily/Cosmo_USB2ADB.c>
   - experimentation on various ADB and USB ISO keyboards and one ADB ANSI keyboard
*/
/* *INDENT-OFF* */
enum SDL_Scancode {
    /*   0 */   SDL_SCANCODE_A,
    /*   1 */   SDL_SCANCODE_S,
    /*   2 */   SDL_SCANCODE_D,
    /*   3 */   SDL_SCANCODE_F,
    /*   4 */   SDL_SCANCODE_H,
    /*   5 */   SDL_SCANCODE_G,
    /*   6 */   SDL_SCANCODE_Z,
    /*   7 */   SDL_SCANCODE_X,
    /*   8 */   SDL_SCANCODE_C,
    /*   9 */   SDL_SCANCODE_V,
    /*  10 */   SDL_SCANCODE_NONUSBACKSLASH, /* SDL_SCANCODE_NONUSBACKSLASH on ANSI and JIS keyboards (if this key would exist there), SDL_SCANCODE_GRAVE on ISO. (The USB keyboard driver actually translates these usage codes to different virtual key codes depending on whether the keyboard is ISO/ANSI/JIS. That's why you have to help it identify the keyboard type when you plug in a PC USB keyboard. It's a historical thing - ADB keyboards are wired this way.) */
    /*  11 */   SDL_SCANCODE_B,
    /*  12 */   SDL_SCANCODE_Q,
    /*  13 */   SDL_SCANCODE_W,
    /*  14 */   SDL_SCANCODE_E,
    /*  15 */   SDL_SCANCODE_R,
    /*  16 */   SDL_SCANCODE_Y,
    /*  17 */   SDL_SCANCODE_T,
    /*  18 */   SDL_SCANCODE_1,
    /*  19 */   SDL_SCANCODE_2,
    /*  20 */   SDL_SCANCODE_3,
    /*  21 */   SDL_SCANCODE_4,
    /*  22 */   SDL_SCANCODE_6,
    /*  23 */   SDL_SCANCODE_5,
    /*  24 */   SDL_SCANCODE_EQUALS,
    /*  25 */   SDL_SCANCODE_9,
    /*  26 */   SDL_SCANCODE_7,
    /*  27 */   SDL_SCANCODE_MINUS,
    /*  28 */   SDL_SCANCODE_8,
    /*  29 */   SDL_SCANCODE_0,
    /*  30 */   SDL_SCANCODE_RIGHTBRACKET,
    /*  31 */   SDL_SCANCODE_O,
    /*  32 */   SDL_SCANCODE_U,
    /*  33 */   SDL_SCANCODE_LEFTBRACKET,
    /*  34 */   SDL_SCANCODE_I,
    /*  35 */   SDL_SCANCODE_P,
    /*  36 */   SDL_SCANCODE_RETURN,
    /*  37 */   SDL_SCANCODE_L,
    /*  38 */   SDL_SCANCODE_J,
    /*  39 */   SDL_SCANCODE_APOSTROPHE,
    /*  40 */   SDL_SCANCODE_K,
    /*  41 */   SDL_SCANCODE_SEMICOLON,
    /*  42 */   SDL_SCANCODE_BACKSLASH,
    /*  43 */   SDL_SCANCODE_COMMA,
    /*  44 */   SDL_SCANCODE_SLASH,
    /*  45 */   SDL_SCANCODE_N,
    /*  46 */   SDL_SCANCODE_M,
    /*  47 */   SDL_SCANCODE_PERIOD,
    /*  48 */   SDL_SCANCODE_TAB,
    /*  49 */   SDL_SCANCODE_SPACE,
    /*  50 */   SDL_SCANCODE_GRAVE, /* SDL_SCANCODE_GRAVE on ANSI and JIS keyboards, SDL_SCANCODE_NONUSBACKSLASH on ISO (see comment about virtual key code 10 above) */
    /*  51 */   SDL_SCANCODE_BACKSPACE,
    /*  52 */   SDL_SCANCODE_KP_ENTER, /* keyboard enter on portables */
    /*  53 */   SDL_SCANCODE_ESCAPE,
    /*  54 */   SDL_SCANCODE_RGUI,
    /*  55 */   SDL_SCANCODE_LGUI,
    /*  56 */   SDL_SCANCODE_LSHIFT,
    /*  57 */   SDL_SCANCODE_CAPSLOCK,
    /*  58 */   SDL_SCANCODE_LALT,
    /*  59 */   SDL_SCANCODE_LCTRL,
    /*  60 */   SDL_SCANCODE_RSHIFT,
    /*  61 */   SDL_SCANCODE_RALT,
    /*  62 */   SDL_SCANCODE_RCTRL,
    /*  63 */   SDL_SCANCODE_RGUI_2, /* fn on portables, acts as a hardware-level modifier already, so we don't generate events for it, also XK_Meta_R */
    /*  64 */   SDL_SCANCODE_F17,
    /*  65 */   SDL_SCANCODE_KP_PERIOD,
    /*  66 */   SDL_SCANCODE_UNKNOWN, /* unknown (unused?) */
    /*  67 */   SDL_SCANCODE_KP_MULTIPLY,
    /*  68 */   SDL_SCANCODE_UNKNOWN_1, /* unknown (unused?) */
    /*  69 */   SDL_SCANCODE_KP_PLUS,
    /*  70 */   SDL_SCANCODE_UNKNOWN_2, /* unknown (unused?) */
    /*  71 */   SDL_SCANCODE_NUMLOCKCLEAR,
    /*  72 */   SDL_SCANCODE_VOLUMEUP,
    /*  73 */   SDL_SCANCODE_VOLUMEDOWN,
    /*  74 */   SDL_SCANCODE_MUTE,
    /*  75 */   SDL_SCANCODE_KP_DIVIDE,
    /*  76 */   SDL_SCANCODE_KP_ENTER_2, /* keypad enter on external keyboards, fn-return on portables */
    /*  77 */   SDL_SCANCODE_UNKNOWN_3, /* unknown (unused?) */
    /*  78 */   SDL_SCANCODE_KP_MINUS,
    /*  79 */   SDL_SCANCODE_F18,
    /*  80 */   SDL_SCANCODE_F19,
    /*  81 */   SDL_SCANCODE_KP_EQUALS,
    /*  82 */   SDL_SCANCODE_KP_0,
    /*  83 */   SDL_SCANCODE_KP_1,
    /*  84 */   SDL_SCANCODE_KP_2,
    /*  85 */   SDL_SCANCODE_KP_3,
    /*  86 */   SDL_SCANCODE_KP_4,
    /*  87 */   SDL_SCANCODE_KP_5,
    /*  88 */   SDL_SCANCODE_KP_6,
    /*  89 */   SDL_SCANCODE_KP_7,
    /*  90 */   SDL_SCANCODE_UNKNOWN_4, /* unknown (unused?) */
    /*  91 */   SDL_SCANCODE_KP_8,
    /*  92 */   SDL_SCANCODE_KP_9,
    /*  93 */   SDL_SCANCODE_INTERNATIONAL3, /* Cosmo_USB2ADB.c says "Yen (JIS)" */
    /*  94 */   SDL_SCANCODE_INTERNATIONAL1, /* Cosmo_USB2ADB.c says "Ro (JIS)" */
    /*  95 */   SDL_SCANCODE_KP_COMMA, /* Cosmo_USB2ADB.c says ", JIS only" */
    /*  96 */   SDL_SCANCODE_F5,
    /*  97 */   SDL_SCANCODE_F6,
    /*  98 */   SDL_SCANCODE_F7,
    /*  99 */   SDL_SCANCODE_F3,
    /* 100 */   SDL_SCANCODE_F8,
    /* 101 */   SDL_SCANCODE_F9,
    /* 102 */   SDL_SCANCODE_LANG2, /* Cosmo_USB2ADB.c says "Eisu" */
    /* 103 */   SDL_SCANCODE_F11,
    /* 104 */   SDL_SCANCODE_LANG1, /* Cosmo_USB2ADB.c says "Kana" */
    /* 105 */   SDL_SCANCODE_PRINTSCREEN, /* On ADB keyboards, this key is labeled "F13/print screen". Problem: USB has different usage codes for these two functions. On Apple USB keyboards, the key is labeled "F13" and sends the F13 usage code (SDL_SCANCODE_F13). I decided to use SDL_SCANCODE_PRINTSCREEN here nevertheless since SDL applications are more likely to assume the presence of a print screen key than an F13 key. */
    /* 106 */   SDL_SCANCODE_F16,
    /* 107 */   SDL_SCANCODE_SCROLLLOCK, /* F14/scroll lock, see comment about F13/print screen above */
    /* 108 */   SDL_SCANCODE_UNKNOWN_5, /* unknown (unused?) */
    /* 109 */   SDL_SCANCODE_F10,
    /* 110 */   SDL_SCANCODE_APPLICATION, /* windows contextual menu key, fn-enter on portables */
    /* 111 */   SDL_SCANCODE_F12,
    /* 112 */   SDL_SCANCODE_UNKNOWN_6, /* unknown (unused?) */
    /* 113 */   SDL_SCANCODE_PAUSE, /* F15/pause, see comment about F13/print screen above */
    /* 114 */   SDL_SCANCODE_INSERT, /* the key is actually labeled "help" on Apple keyboards, and works as such in Mac OS, but it sends the "insert" usage code even on Apple USB keyboards */
    /* 115 */   SDL_SCANCODE_HOME,
    /* 116 */   SDL_SCANCODE_PAGEUP,
    /* 117 */   SDL_SCANCODE_DELETE,
    /* 118 */   SDL_SCANCODE_F4,
    /* 119 */   SDL_SCANCODE_END,
    /* 120 */   SDL_SCANCODE_F2,
    /* 121 */   SDL_SCANCODE_PAGEDOWN,
    /* 122 */   SDL_SCANCODE_F1,
    /* 123 */   SDL_SCANCODE_LEFT,
    /* 124 */   SDL_SCANCODE_RIGHT,
    /* 125 */   SDL_SCANCODE_DOWN,
    /* 126 */   SDL_SCANCODE_UP,
    /* 127 */   SDL_SCANCODE_POWER
};
/* *INDENT-ON* */
#endif

#endif // SDL_SCANCODE_HEADER