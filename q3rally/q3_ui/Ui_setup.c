// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

SETUP MENU

=======================================================================
*/


#include "ui_local.h"


#define SETUP_MENU_VERTICAL_SPACING		34

// BAGPUSS
/*
#define ART_BACK0		"menu/art/back_0"
#define ART_BACK1		"menu/art/back_1"	
#define ART_FRAMEL		"menu/art/frame2_l"
#define ART_FRAMER		"menu/art/frame1_r"
*/
// END

#define ID_CUSTOMIZEPLAYER		10
#define ID_CUSTOMIZECONTROLS	11
#define ID_SYSTEMCONFIG			12
#define ID_GAME					13
#define ID_CDKEY				14
#define ID_LOAD					15
#define ID_SAVE					16
#define ID_DEFAULTS				17
#define ID_BACK					18
// STONELANCE
#define ID_Q3ROPTIONS			19
// END


typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
// BAGPUSS
/*
	menubitmap_s	framel;
	menubitmap_s	framer;
*/
// END
	menutext_s		setupplayer;
	menutext_s		setupcontrols;
	menutext_s		setupsystem;
	menutext_s		game;
	menutext_s		cdkey;
//	menutext_s		load;
//	menutext_s		save;
// STONELANCE
	menutext_s		q3rOptions;
// END
	menutext_s		defaults;
// BAGPUSS
//	menubitmap_s	back;
	menutext_s		back;
// END
} setupMenuInfo_t;

static setupMenuInfo_t	setupMenuInfo;


/*
=================
Setup_ResetDefaults_Action
=================
*/
static void Setup_ResetDefaults_Action( qboolean result ) {
	if( !result ) {
		return;
	}
	trap_Cmd_ExecuteText( EXEC_APPEND, "exec default.cfg\n");
	trap_Cmd_ExecuteText( EXEC_APPEND, "cvar_restart\n");
	trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
}


/*
=================
Setup_ResetDefaults_Draw
=================
*/
static void Setup_ResetDefaults_Draw( void ) {
	UI_DrawProportionalString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 0, "WARNING: This will reset *ALL*", UI_CENTER|UI_SMALLFONT, color_yellow );
	UI_DrawProportionalString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 1, "options to their default values.", UI_CENTER|UI_SMALLFONT, color_yellow );
}


/*
===============
UI_SetupMenu_Event
===============
*/
static void UI_SetupMenu_Event( void *ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
/*
	case ID_CUSTOMIZEPLAYER:
		UI_PlayerSettingsMenu();
		break;

	case ID_CUSTOMIZECONTROLS:
		UI_ControlsMenu();
		break;

	case ID_SYSTEMCONFIG:
		UI_GraphicsOptionsMenu();
		break;

	case ID_GAME:
		UI_PreferencesMenu();
		break;

	case ID_CDKEY:
		UI_CDKeyMenu();
		break;

//	case ID_LOAD:
//		UI_LoadConfigMenu();
//		break;

//	case ID_SAVE:
//		UI_SaveConfigMenu();
//		break;

	case ID_DEFAULTS:
		UI_ConfirmMenu( "SET TO DEFAULTS?", Setup_ResetDefaults_Draw, Setup_ResetDefaults_Action );
		break;

	case ID_BACK:
		UI_PopMenu();
		break;
*/
	default:
		setupMenuInfo.menu.transitionMenu = ((menucommon_s*)ptr)->id;
		uis.transitionOut = uis.realtime;
		break;
// END
	}
}


// STONELANCE
/*
=================
UI_SetupMenu_ChangeMenu
=================
*/
void UI_SetupMenu_ChangeMenu( int menuID ){

	switch(menuID){
	case ID_CUSTOMIZEPLAYER:
		UI_PlayerSettingsMenu();
		break;

	case ID_CUSTOMIZECONTROLS:
		UI_ControlsMenu();
		break;

	case ID_SYSTEMCONFIG:
		UI_GraphicsOptionsMenu();
		break;

	case ID_GAME:
		UI_PreferencesMenu();
		break;

	case ID_CDKEY:
		UI_CDKeyMenu();
		break;

//	case ID_LOAD:
//		UI_LoadConfigMenu();
//		break;

//	case ID_SAVE:
//		UI_SaveConfigMenu();
//		break;

	case ID_Q3ROPTIONS:
		UI_Q3ROptionsMenu();
		break;

	case ID_DEFAULTS:
		UI_ConfirmMenu( "SET TO DEFAULTS?", Setup_ResetDefaults_Draw, Setup_ResetDefaults_Action );
		break;

	case ID_BACK:
//		uis.transitionIn = uis.realtime;
		UI_PopMenu();
		break;
	}
}


/*
=================
UI_SetupMenu_RunTransition
=================
*/
void UI_SetupMenu_RunTransition(float frac){
	uis.text_color[0] = text_color_normal[0];
	uis.text_color[1] = text_color_normal[1];
	uis.text_color[2] = text_color_normal[2];
	uis.text_color[3] = text_color_normal[3] * frac;

	setupMenuInfo.banner.color = uis.text_color;

	setupMenuInfo.cdkey.color = uis.text_color;
	setupMenuInfo.defaults.color = uis.text_color;
	setupMenuInfo.game.color = uis.text_color;
	setupMenuInfo.q3rOptions.color = uis.text_color;
	setupMenuInfo.setupcontrols.color = uis.text_color;
	setupMenuInfo.setupplayer.color = uis.text_color;
	setupMenuInfo.setupsystem.color = uis.text_color;
}


/*
===============
UI_SetupMenu_Init
===============
*/
static void UI_SetupMenu_Init( void ) {
	int				y;

	UI_SetupMenu_Cache();

	memset( &setupMenuInfo, 0, sizeof(setupMenuInfo) );
	setupMenuInfo.menu.wrapAround = qtrue;
	setupMenuInfo.menu.fullscreen = qtrue;
// STONELANCE
	setupMenuInfo.menu.transition = UI_SetupMenu_RunTransition;
	setupMenuInfo.menu.changeMenu = UI_SetupMenu_ChangeMenu;
// END

	setupMenuInfo.banner.generic.type				= MTYPE_BTEXT;
	setupMenuInfo.banner.generic.x					= 320;
	setupMenuInfo.banner.generic.y					= 16;
	setupMenuInfo.banner.string						= "SETUP";
	setupMenuInfo.banner.color						= color_white;
	setupMenuInfo.banner.style						= UI_CENTER;

// BAGPUSS
/*
	setupMenuInfo.framel.generic.type				= MTYPE_BITMAP;
	setupMenuInfo.framel.generic.name				= ART_FRAMEL;
	setupMenuInfo.framel.generic.flags				= QMF_INACTIVE;
	setupMenuInfo.framel.generic.x					= 0;  
	setupMenuInfo.framel.generic.y					= 78;
	setupMenuInfo.framel.width  					= 256;
	setupMenuInfo.framel.height  					= 329;

	setupMenuInfo.framer.generic.type				= MTYPE_BITMAP;
	setupMenuInfo.framer.generic.name				= ART_FRAMER;
	setupMenuInfo.framer.generic.flags				= QMF_INACTIVE;
	setupMenuInfo.framer.generic.x					= 376;
	setupMenuInfo.framer.generic.y					= 76;
	setupMenuInfo.framer.width  					= 256;
	setupMenuInfo.framer.height  					= 334;
*/
// END

	y = 134;
	setupMenuInfo.setupplayer.generic.type			= MTYPE_PTEXT;
	setupMenuInfo.setupplayer.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.setupplayer.generic.x				= 320;
	setupMenuInfo.setupplayer.generic.y				= y;
	setupMenuInfo.setupplayer.generic.id			= ID_CUSTOMIZEPLAYER;
	setupMenuInfo.setupplayer.generic.callback		= UI_SetupMenu_Event; 
	setupMenuInfo.setupplayer.string				= "PLAYER";
// BAGPUSS
//	setupMenuInfo.setupplayer.color					= color_red;
	setupMenuInfo.setupplayer.color					= text_color_normal;
// END
	setupMenuInfo.setupplayer.style					= UI_CENTER;

	y += SETUP_MENU_VERTICAL_SPACING;
	setupMenuInfo.setupcontrols.generic.type		= MTYPE_PTEXT;
	setupMenuInfo.setupcontrols.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.setupcontrols.generic.x			= 320;
	setupMenuInfo.setupcontrols.generic.y			= y;
	setupMenuInfo.setupcontrols.generic.id			= ID_CUSTOMIZECONTROLS;
	setupMenuInfo.setupcontrols.generic.callback	= UI_SetupMenu_Event; 
	setupMenuInfo.setupcontrols.string				= "CONTROLS";
// BAGPUSS
//	setupMenuInfo.setupcontrols.color				= color_red;
	setupMenuInfo.setupcontrols.color				= text_color_normal;
// END
	setupMenuInfo.setupcontrols.style				= UI_CENTER;

	y += SETUP_MENU_VERTICAL_SPACING;
	setupMenuInfo.setupsystem.generic.type			= MTYPE_PTEXT;
	setupMenuInfo.setupsystem.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.setupsystem.generic.x				= 320;
	setupMenuInfo.setupsystem.generic.y				= y;
	setupMenuInfo.setupsystem.generic.id			= ID_SYSTEMCONFIG;
	setupMenuInfo.setupsystem.generic.callback		= UI_SetupMenu_Event; 
	setupMenuInfo.setupsystem.string				= "SYSTEM";
// BAGPUSS
//	setupMenuInfo.setupsystem.color					= color_red;
	setupMenuInfo.setupsystem.color					= text_color_normal;
// END
	setupMenuInfo.setupsystem.style					= UI_CENTER;

	y += SETUP_MENU_VERTICAL_SPACING;
	setupMenuInfo.game.generic.type					= MTYPE_PTEXT;
	setupMenuInfo.game.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.game.generic.x					= 320;
	setupMenuInfo.game.generic.y					= y;
	setupMenuInfo.game.generic.id					= ID_GAME;
	setupMenuInfo.game.generic.callback				= UI_SetupMenu_Event; 
	setupMenuInfo.game.string						= "GAME OPTIONS";
// BAGPUSS
//	setupMenuInfo.game.color						= color_red;
	setupMenuInfo.game.color						= text_color_normal;
// END
	setupMenuInfo.game.style						= UI_CENTER;

// STONELANCE
	y += SETUP_MENU_VERTICAL_SPACING;
	setupMenuInfo.q3rOptions.generic.type				= MTYPE_PTEXT;
	setupMenuInfo.q3rOptions.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.q3rOptions.generic.x					= 320;
	setupMenuInfo.q3rOptions.generic.y					= y;
	setupMenuInfo.q3rOptions.generic.id					= ID_Q3ROPTIONS;
	setupMenuInfo.q3rOptions.generic.callback			= UI_SetupMenu_Event; 
	setupMenuInfo.q3rOptions.string						= "Q3R OPTIONS";
	setupMenuInfo.q3rOptions.color						= text_color_normal;
	setupMenuInfo.q3rOptions.style						= UI_CENTER;
// END

	y += SETUP_MENU_VERTICAL_SPACING;
	setupMenuInfo.cdkey.generic.type				= MTYPE_PTEXT;
	setupMenuInfo.cdkey.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.cdkey.generic.x					= 320;
	setupMenuInfo.cdkey.generic.y					= y;
	setupMenuInfo.cdkey.generic.id					= ID_CDKEY;
	setupMenuInfo.cdkey.generic.callback			= UI_SetupMenu_Event; 
	setupMenuInfo.cdkey.string						= "CD Key";
// BAGPUSS
//	setupMenuInfo.cdkey.color						= color_red;
	setupMenuInfo.cdkey.color						= text_color_normal;
// END
	setupMenuInfo.cdkey.style						= UI_CENTER;

	if( !trap_Cvar_VariableValue( "cl_paused" ) ) {
#if 0
		y += SETUP_MENU_VERTICAL_SPACING;
		setupMenuInfo.load.generic.type					= MTYPE_PTEXT;
		setupMenuInfo.load.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
		setupMenuInfo.load.generic.x					= 320;
		setupMenuInfo.load.generic.y					= y;
		setupMenuInfo.load.generic.id					= ID_LOAD;
		setupMenuInfo.load.generic.callback				= UI_SetupMenu_Event; 
		setupMenuInfo.load.string						= "LOAD";
		setupMenuInfo.load.color						= color_red;
		setupMenuInfo.load.style						= UI_CENTER;

		y += SETUP_MENU_VERTICAL_SPACING;
		setupMenuInfo.save.generic.type					= MTYPE_PTEXT;
		setupMenuInfo.save.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
		setupMenuInfo.save.generic.x					= 320;
		setupMenuInfo.save.generic.y					= y;
		setupMenuInfo.save.generic.id					= ID_SAVE;
		setupMenuInfo.save.generic.callback				= UI_SetupMenu_Event; 
		setupMenuInfo.save.string						= "SAVE";
		setupMenuInfo.save.color						= color_red;
		setupMenuInfo.save.style						= UI_CENTER;
#endif

		y += SETUP_MENU_VERTICAL_SPACING;
		setupMenuInfo.defaults.generic.type				= MTYPE_PTEXT;
		setupMenuInfo.defaults.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
		setupMenuInfo.defaults.generic.x				= 320;
		setupMenuInfo.defaults.generic.y				= y;
		setupMenuInfo.defaults.generic.id				= ID_DEFAULTS;
		setupMenuInfo.defaults.generic.callback			= UI_SetupMenu_Event; 
		setupMenuInfo.defaults.string					= "DEFAULTS";
// BAGPUSS
//		setupMenuInfo.defaults.color					= color_red;
		setupMenuInfo.defaults.color					= text_color_normal;
// END
		setupMenuInfo.defaults.style					= UI_CENTER;
	}

// BAGPUSS
/*
	setupMenuInfo.back.generic.type					= MTYPE_BITMAP;
	setupMenuInfo.back.generic.name					= ART_BACK0;
	setupMenuInfo.back.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.back.generic.id					= ID_BACK;
	setupMenuInfo.back.generic.callback				= UI_SetupMenu_Event;
	setupMenuInfo.back.generic.x					= 0;
	setupMenuInfo.back.generic.y					= 480-64;
	setupMenuInfo.back.width						= 128;
	setupMenuInfo.back.height						= 64;
	setupMenuInfo.back.focuspic						= ART_BACK1;
*/

	setupMenuInfo.back.generic.type			= MTYPE_PTEXT;
	setupMenuInfo.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.back.generic.x			= 20;
	setupMenuInfo.back.generic.y			= 480 - 50;
	setupMenuInfo.back.generic.id			= ID_BACK;
	setupMenuInfo.back.generic.callback		= UI_SetupMenu_Event; 
	setupMenuInfo.back.string				= "< BACK";
	setupMenuInfo.back.color				= text_color_normal;
	setupMenuInfo.back.style				= UI_LEFT | UI_SMALLFONT;
// END

	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.banner );
// BAGPUSS
/*
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.framel );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.framer );
*/
// END
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.setupplayer );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.setupcontrols );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.setupsystem );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.game );
// STONELANCE
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.q3rOptions );
// END
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.cdkey );
//	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.load );
//	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.save );
	if( !trap_Cvar_VariableValue( "cl_paused" ) ) {
		Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.defaults );
	}
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.back );

// STONELANCE
	uis.transitionIn = uis.realtime;
// END
}


/*
=================
UI_SetupMenu_Cache
=================
*/
void UI_SetupMenu_Cache( void ) {
// BAGPUSS
/*
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
*/
// END
}


/*
===============
UI_SetupMenu
===============
*/
void UI_SetupMenu( void ) {
	UI_SetupMenu_Init();
	UI_PushMenu( &setupMenuInfo.menu );
}
