/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2013 Darklegion Development
Copyright (C) 2015-2019 GrangerHub

This file is part of Tremulous.

Tremulous is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Tremulous is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Tremulous; if not, see <https://www.gnu.org/licenses/>

===========================================================================
*/

// cg_main.c -- initialization and primary entry point for cgame

#include "cg_local.h"
#include "ui/ui_shared.h"

// display context for new ui stuff
displayContextDef_t cgDC;

void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum );
void CG_Shutdown( void );
static char *CG_VoIPString( void );

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
Q_EXPORT intptr_t vmMain( int command, int arg0, int arg1, int arg2 )
{
  switch( command )
  {
    case CG_INIT:
      CG_Init( arg0, arg1, arg2 );
      return 0;

    case CG_SHUTDOWN:
      CG_Shutdown( );
      return 0;

    case CG_CONSOLE_COMMAND:
      return CG_ConsoleCommand( );

    case CG_CONSOLE_TEXT:
      CG_AddNotifyText( );
      return 0;

    case CG_DRAW_ACTIVE_FRAME:
      CG_DrawActiveFrame( arg0, arg1, arg2 );
      return 0;

    case CG_CROSSHAIR_PLAYER:
      return CG_CrosshairPlayer( );

    case CG_LAST_ATTACKER:
      return CG_LastAttacker( );

    case CG_KEY_EVENT:
      CG_KeyEvent( arg0, arg1 );
      return 0;

    case CG_MOUSE_EVENT:
      // cgame doesn't care where the cursor is
      return 0;

    case CG_EVENT_HANDLING:
      CG_EventHandling( arg0 );
      return 0;

#ifndef MODULE_INTERFACE_11
    case CG_VOIP_STRING:
      return (intptr_t)CG_VoIPString( );

    case CG_CONSOLE_COMPLETARGUMENT:
      return CG_Console_CompleteArgument( arg0 );
#endif

    default:
      CG_Error( "vmMain: unknown command %i", command );
      break;
  }

  return -1;
}


cg_t        cg;
cgs_t       cgs;
centity_t   cg_entities[ MAX_GENTITIES ];

weaponInfo_t    cg_weapons[ 32 ];
upgradeInfo_t   cg_upgrades[ 32 ];

buildableInfo_t cg_buildables[ BA_NUM_BUILDABLES ];

vmCvar_t  cg_teslaTrailTime;
vmCvar_t  cg_centertime;
vmCvar_t  cg_runpitch;
vmCvar_t  cg_runroll;
vmCvar_t  cg_swingSpeed;
vmCvar_t  cg_shadows;
vmCvar_t  cg_drawTimer;
vmCvar_t  cg_drawClock;
vmCvar_t  cg_drawFPS;
vmCvar_t  cg_drawDemoState;
vmCvar_t  cg_drawSnapshot;
vmCvar_t  cg_drawChargeBar;
vmCvar_t  cg_drawCrosshair;
vmCvar_t  cg_drawCrosshairNames;
vmCvar_t  cg_crosshairSize;
vmCvar_t  cg_draw2D;
vmCvar_t  cg_animSpeed;
vmCvar_t  cg_debugAnim;
vmCvar_t  cg_debugPosition;
vmCvar_t  cg_debugEvents;
vmCvar_t  cg_errorDecay;
vmCvar_t  cg_nopredict;
vmCvar_t  cg_debugMove;
vmCvar_t  cg_noPlayerAnims;
vmCvar_t  cg_showmiss;
vmCvar_t  cg_footsteps;
vmCvar_t  cg_addMarks;
vmCvar_t  cg_viewsize;
vmCvar_t  cg_drawGun;
vmCvar_t  cg_gun_frame;
vmCvar_t  cg_gun_x;
vmCvar_t  cg_gun_y;
vmCvar_t  cg_gun_z;
vmCvar_t  cg_tracerChance;
vmCvar_t  cg_tracerWidth;
vmCvar_t  cg_tracerLength;
vmCvar_t  cg_thirdPerson;
vmCvar_t  cg_thirdPersonAngle;
vmCvar_t  cg_thirdPersonShoulderViewMode;
vmCvar_t  cg_staticDeathCam;
vmCvar_t  cg_thirdPersonPitchFollow;
vmCvar_t  cg_thirdPersonRange;
vmCvar_t  cg_stereoSeparation;
vmCvar_t  cg_lagometer;
vmCvar_t  cg_drawSpeed;
vmCvar_t  cg_maxSpeedTimeWindow;
vmCvar_t  cg_synchronousClients;
vmCvar_t  cg_stats;
vmCvar_t  cg_paused;
vmCvar_t  cg_blood;
vmCvar_t  cg_teamChatsOnly;
vmCvar_t  cg_drawTeamOverlay;
vmCvar_t  cg_teamOverlaySortMode;
vmCvar_t  cg_teamOverlayMaxPlayers;
vmCvar_t  cg_teamOverlayUserinfo;
vmCvar_t  cg_noPrintDuplicate;
vmCvar_t  cg_noVoiceChats;
vmCvar_t  cg_noVoiceText;
vmCvar_t  cg_hudFiles;
vmCvar_t  cg_smoothClients;
vmCvar_t  pmove_fixed;
vmCvar_t  pmove_msec;
vmCvar_t  cg_cameraMode;
vmCvar_t  cg_timescaleFadeEnd;
vmCvar_t  cg_timescaleFadeSpeed;
vmCvar_t  cg_timescale;
vmCvar_t  cg_noTaunt;
vmCvar_t  cg_drawSurfNormal;
vmCvar_t  cg_drawBBOX;
vmCvar_t  cg_wwSmoothTime;
vmCvar_t  cg_disableBlueprintErrors;
vmCvar_t  cg_depthSortParticles;
vmCvar_t  cg_bounceParticles;
vmCvar_t  cg_consoleLatency;
vmCvar_t  cg_lightFlare;
vmCvar_t  cg_debugParticles;
vmCvar_t  cg_debugTrails;
vmCvar_t  cg_debugPVS;
vmCvar_t  cg_disableWarningDialogs;
vmCvar_t  cg_disableUpgradeDialogs;
vmCvar_t  cg_disableBuildDialogs;
vmCvar_t  cg_disableCommandDialogs;
vmCvar_t  cg_disableScannerPlane;
vmCvar_t  cg_tutorial;

vmCvar_t  cg_rangeMarkerDrawSurface;
vmCvar_t  cg_rangeMarkerDrawIntersection;
vmCvar_t  cg_rangeMarkerDrawFrontline;
vmCvar_t  cg_rangeMarkerSurfaceOpacity;
vmCvar_t  cg_rangeMarkerLineOpacity;
vmCvar_t  cg_rangeMarkerLineThickness;
vmCvar_t  cg_rangeMarkerForBlueprint;
vmCvar_t  cg_rangeMarkerBuildableTypes;
vmCvar_t  cg_binaryShaderScreenScale;

vmCvar_t  cg_animatedCreep;

vmCvar_t  cg_painBlendUpRate;
vmCvar_t  cg_painBlendDownRate;
vmCvar_t  cg_painBlendMax;
vmCvar_t  cg_painBlendScale;
vmCvar_t  cg_painBlendZoom;

vmCvar_t  cg_stickySpec;
vmCvar_t  cg_sprintToggle;
vmCvar_t  cg_unlagged;

vmCvar_t  cg_debugVoices;

vmCvar_t  ui_currentClass;
vmCvar_t  ui_carriage;
vmCvar_t  ui_credit;
vmCvar_t  ui_ammoFull;
vmCvar_t  ui_stages;
vmCvar_t  ui_alienStates;
vmCvar_t  ui_humanStates;
vmCvar_t  ui_dialog;
vmCvar_t  ui_voteActive;
vmCvar_t  ui_alienTeamVoteActive;
vmCvar_t  ui_humanTeamVoteActive;

vmCvar_t  cg_debugRandom;

vmCvar_t  cg_optimizePrediction;
vmCvar_t  cg_projectileNudge;

vmCvar_t  cg_voice;

vmCvar_t  cg_emoticons;

vmCvar_t  cg_chatTeamPrefix;

vmCvar_t  cg_killMsg;
vmCvar_t  cg_killMsgTime;
vmCvar_t  cg_killMsgHeight;

vmCvar_t  thz_radar;
vmCvar_t  thz_radarrange;

typedef struct
{
  vmCvar_t  *vmCvar;
  char      *cvarName;
  char      *defaultString;
  int       cvarFlags;
} cvarTable_t;

static cvarTable_t cvarTable[ ] =
{
  { &cg_drawGun, "cg_drawGun", "1", CVAR_ARCHIVE },
  { &cg_viewsize, "cg_viewsize", "100", CVAR_ARCHIVE },
  { &cg_stereoSeparation, "cg_stereoSeparation", "0.4", CVAR_ARCHIVE  },
  { &cg_shadows, "cg_shadows", "1", CVAR_ARCHIVE  },
  { &cg_draw2D, "cg_draw2D", "1", CVAR_ARCHIVE  },
  { &cg_drawTimer, "cg_drawTimer", "1", CVAR_ARCHIVE  },
  { &cg_drawClock, "cg_drawClock", "0", CVAR_ARCHIVE  },
  { &cg_drawFPS, "cg_drawFPS", "1", CVAR_ARCHIVE  },
  { &cg_drawDemoState, "cg_drawDemoState", "1", CVAR_ARCHIVE  },
  { &cg_drawSnapshot, "cg_drawSnapshot", "0", CVAR_ARCHIVE  },
  { &cg_drawChargeBar, "cg_drawChargeBar", "1", CVAR_ARCHIVE  },
  { &cg_drawCrosshair, "cg_drawCrosshair", "2", CVAR_ARCHIVE },
  { &cg_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE },
  { &cg_crosshairSize, "cg_crosshairSize", "1", CVAR_ARCHIVE },
  { &cg_addMarks, "cg_marks", "1", CVAR_ARCHIVE },
  { &cg_lagometer, "cg_lagometer", "0", CVAR_ARCHIVE },
  { &cg_drawSpeed, "cg_drawSpeed", "0", CVAR_ARCHIVE },
  { &cg_maxSpeedTimeWindow, "cg_maxSpeedTimeWindow", "2000", CVAR_ARCHIVE },
  { &cg_teslaTrailTime, "cg_teslaTrailTime", "250", CVAR_ARCHIVE  },
  { &cg_gun_x, "cg_gunX", "0", CVAR_CHEAT },
  { &cg_gun_y, "cg_gunY", "0", CVAR_CHEAT },
  { &cg_gun_z, "cg_gunZ", "0", CVAR_CHEAT },
  { &cg_centertime, "cg_centertime", "3", CVAR_CHEAT },
  { &cg_runpitch, "cg_runpitch", "0.002", CVAR_ARCHIVE},
  { &cg_runroll, "cg_runroll", "0.005", CVAR_ARCHIVE },
  { &cg_swingSpeed, "cg_swingSpeed", "0.3", CVAR_CHEAT },
  { &cg_animSpeed, "cg_animspeed", "1", CVAR_CHEAT },
  { &cg_debugAnim, "cg_debuganim", "0", CVAR_CHEAT },
  { &cg_debugPosition, "cg_debugposition", "0", CVAR_CHEAT },
  { &cg_debugEvents, "cg_debugevents", "0", CVAR_CHEAT },
  { &cg_errorDecay, "cg_errordecay", "100", 0 },
  { &cg_nopredict, "cg_nopredict", "0", 0 },
  { &cg_debugMove, "cg_debugMove", "0", 0 },
  { &cg_noPlayerAnims, "cg_noplayeranims", "0", CVAR_CHEAT },
  { &cg_showmiss, "cg_showmiss", "0", 0 },
  { &cg_footsteps, "cg_footsteps", "1", CVAR_CHEAT },
  { &cg_tracerChance, "cg_tracerchance", "0.4", CVAR_CHEAT },
  { &cg_tracerWidth, "cg_tracerwidth", "1", CVAR_CHEAT },
  { &cg_tracerLength, "cg_tracerlength", "100", CVAR_CHEAT },
  { &cg_thirdPersonRange, "cg_thirdPersonRange", "75", CVAR_ARCHIVE },
  { &cg_thirdPerson, "cg_thirdPerson", "0", CVAR_CHEAT },
  { &cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", CVAR_CHEAT },
  { &cg_thirdPersonPitchFollow, "cg_thirdPersonPitchFollow", "0", 0 },
  { &cg_thirdPersonShoulderViewMode, "cg_thirdPersonShoulderViewMode", "1", CVAR_ARCHIVE },
  { &cg_staticDeathCam, "cg_staticDeathCam", "0", CVAR_ARCHIVE },
  { &cg_stats, "cg_stats", "0", 0 },
  { &cg_drawTeamOverlay, "cg_drawTeamOverlay", "1", CVAR_ARCHIVE },
  { &cg_teamOverlaySortMode, "cg_teamOverlaySortMode", "1", CVAR_ARCHIVE },
  { &cg_teamOverlayMaxPlayers, "cg_teamOverlayMaxPlayers", "8", CVAR_ARCHIVE },
  { &cg_teamOverlayUserinfo, "teamoverlay", "1", CVAR_ARCHIVE|CVAR_USERINFO },
  { &cg_teamChatsOnly, "cg_teamChatsOnly", "0", CVAR_ARCHIVE },
  { &cg_noPrintDuplicate, "cg_noPrintDuplicate", "0", CVAR_ARCHIVE },
  { &cg_noVoiceChats, "cg_noVoiceChats", "0", CVAR_ARCHIVE },
  { &cg_noVoiceText, "cg_noVoiceText", "0", CVAR_ARCHIVE },
  { &cg_drawSurfNormal, "cg_drawSurfNormal", "0", CVAR_CHEAT },
  { &cg_drawBBOX, "cg_drawBBOX", "0", CVAR_CHEAT },
  { &cg_wwSmoothTime, "cg_wwSmoothTime", "225", CVAR_ARCHIVE },
  { NULL, "cg_wwFollow", "1", CVAR_ARCHIVE|CVAR_USERINFO },
  { NULL, "cg_wwToggle", "1", CVAR_ARCHIVE|CVAR_USERINFO },
  { NULL, "cg_disableBlueprintErrors", "0", CVAR_ARCHIVE|CVAR_USERINFO },
  { &cg_stickySpec, "cg_stickySpec", "1", CVAR_ARCHIVE|CVAR_USERINFO },
  { &cg_sprintToggle, "cg_sprintToggle", "0", CVAR_ARCHIVE|CVAR_USERINFO },
  { &cg_unlagged, "cg_unlagged", "1", CVAR_ARCHIVE|CVAR_USERINFO },
  { NULL, "cg_flySpeed", "600", CVAR_ARCHIVE|CVAR_USERINFO },
  { &cg_depthSortParticles, "cg_depthSortParticles", "1", CVAR_ARCHIVE },
  { &cg_bounceParticles, "cg_bounceParticles", "1", CVAR_ARCHIVE },
  { &cg_consoleLatency, "cg_consoleLatency", "3000", CVAR_ARCHIVE },
  { &cg_lightFlare, "cg_lightFlare", "3", CVAR_ARCHIVE },
  { &cg_debugParticles, "cg_debugParticles", "0", CVAR_CHEAT },
  { &cg_debugTrails, "cg_debugTrails", "0", CVAR_CHEAT },
  { &cg_debugPVS, "cg_debugPVS", "0", CVAR_CHEAT },
  { &cg_disableWarningDialogs, "cg_disableWarningDialogs", "0", CVAR_ARCHIVE },
  { &cg_disableUpgradeDialogs, "cg_disableUpgradeDialogs", "0", CVAR_ARCHIVE },
  { &cg_disableBuildDialogs, "cg_disableBuildDialogs", "0", CVAR_ARCHIVE },
  { &cg_disableCommandDialogs, "cg_disableCommandDialogs", "0", CVAR_ARCHIVE },
  { &cg_disableScannerPlane, "cg_disableScannerPlane", "1", CVAR_ARCHIVE },
  { &cg_tutorial, "cg_tutorial", "1", CVAR_ARCHIVE },

  { &cg_rangeMarkerDrawSurface, "cg_rangeMarkerDrawSurface", "1", CVAR_ARCHIVE },
  { &cg_rangeMarkerDrawIntersection, "cg_rangeMarkerDrawIntersection", "0", CVAR_ARCHIVE },
  { &cg_rangeMarkerDrawFrontline, "cg_rangeMarkerDrawFrontline", "0", CVAR_ARCHIVE },
  { &cg_rangeMarkerSurfaceOpacity, "cg_rangeMarkerSurfaceOpacity", "0.08", CVAR_ARCHIVE },
  { &cg_rangeMarkerLineOpacity, "cg_rangeMarkerLineOpacity", "0.4", CVAR_ARCHIVE },
  { &cg_rangeMarkerLineThickness, "cg_rangeMarkerLineThickness", "4.0", CVAR_ARCHIVE },
  { &cg_rangeMarkerForBlueprint, "cg_rangeMarkerForBlueprint", "1", CVAR_ARCHIVE },
  { &cg_rangeMarkerBuildableTypes, "cg_rangeMarkerBuildableTypes", "support", CVAR_ARCHIVE },
  { NULL, "cg_buildableRangeMarkerMask", "", CVAR_USERINFO },
  { &cg_binaryShaderScreenScale, "cg_binaryShaderScreenScale", "1.0", CVAR_ARCHIVE },

  { &cg_animatedCreep, "cg_animatedCreep", "2", CVAR_ARCHIVE },

  { &cg_hudFiles, "cg_hudFiles", "ui/hud.txt", CVAR_ARCHIVE},
  { NULL, "cg_alienConfig", "", CVAR_ARCHIVE },
  { NULL, "cg_humanConfig", "", CVAR_ARCHIVE },
  { NULL, "cg_spectatorConfig", "", CVAR_ARCHIVE },

  { &cg_painBlendUpRate, "cg_painBlendUpRate", "10.0", 0 },
  { &cg_painBlendDownRate, "cg_painBlendDownRate", "0.5", 0 },
  { &cg_painBlendMax, "cg_painBlendMax", "0.7", 0 },
  { &cg_painBlendScale, "cg_painBlendScale", "7.0", 0 },
  { &cg_painBlendZoom, "cg_painBlendZoom", "0.65", 0 },

  { &cg_debugVoices, "cg_debugVoices", "0", 0 },

  // communication cvars set by the cgame to be read by ui
  { &ui_currentClass, "ui_currentClass", "0", CVAR_ROM },
  { &ui_carriage, "ui_carriage", "", CVAR_ROM },
  { &ui_credit, "ui_credit", "0", CVAR_ROM },
  { &ui_ammoFull, "ui_ammoFull", "1", CVAR_ROM },
  { &ui_stages, "ui_stages", "0 0", CVAR_ROM },
  { &ui_alienStates, "ui_alienStates", "0 0 0 0 0", CVAR_ROM },
  { &ui_humanStates, "ui_humanStates", "0 0 0 0 0 0", CVAR_ROM },
  { &ui_dialog, "ui_dialog", "Text not set", CVAR_ROM },
  { &ui_voteActive, "ui_voteActive", "0", CVAR_ROM },
  { &ui_humanTeamVoteActive, "ui_humanTeamVoteActive", "0", CVAR_ROM },
  { &ui_alienTeamVoteActive, "ui_alienTeamVoteActive", "0", CVAR_ROM },

  { &cg_debugRandom, "cg_debugRandom", "0", 0 },

  { &cg_optimizePrediction, "cg_optimizePrediction", "1", CVAR_ARCHIVE },
  { &cg_projectileNudge, "cg_projectileNudge", "1", CVAR_ARCHIVE },

  // the following variables are created in other parts of the system,
  // but we also reference them here

  { &cg_paused, "cl_paused", "0", CVAR_ROM },
  { &cg_blood, "cg_blood", "1", CVAR_ARCHIVE },
  { &cg_synchronousClients, "g_synchronousClients", "0", 0 }, // communicated by systeminfo
  { &cg_timescaleFadeEnd, "cg_timescaleFadeEnd", "1", CVAR_CHEAT },
  { &cg_timescaleFadeSpeed, "cg_timescaleFadeSpeed", "0", CVAR_CHEAT },
  { &cg_timescale, "timescale", "1", 0},
  { &cg_smoothClients, "cg_smoothClients", "0", CVAR_USERINFO | CVAR_ARCHIVE},
  { &cg_cameraMode, "com_cameraMode", "0", CVAR_CHEAT},

  { &pmove_fixed, "pmove_fixed", "0", 0},
  { &pmove_msec, "pmove_msec", "8", 0},
  { &cg_noTaunt, "cg_noTaunt", "0", CVAR_ARCHIVE},

  { &cg_voice, "voice", "default", CVAR_USERINFO|CVAR_ARCHIVE},

  { &cg_emoticons, "cg_emoticons", "1", CVAR_LATCH|CVAR_ARCHIVE},

  { &cg_chatTeamPrefix, "cg_chatTeamPrefix", "1", CVAR_ARCHIVE},

  { &cg_killMsg, "cg_killMsg", "1", CVAR_ARCHIVE },
  { &cg_killMsgTime, "cg_killMsgTime", "4000", CVAR_ARCHIVE },
  { &cg_killMsgHeight, "cg_killMsgHeight", "7", CVAR_ARCHIVE },

  // Old school thz stuff
  { &thz_radar, "thz_radar", "0", CVAR_CHEAT},
  { &thz_radarrange, "thz_radarrange", "600", CVAR_ARCHIVE},

};

static size_t cvarTableSize = ARRAY_LEN( cvarTable );

/*
=================
CG_RegisterCvars
=================
*/
void CG_RegisterCvars( void )
{
  int         i;
  cvarTable_t *cv;
  char        var[ MAX_TOKEN_CHARS ];

  for( i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++ )
  {
    trap_Cvar_Register( cv->vmCvar, cv->cvarName,
      cv->defaultString, cv->cvarFlags );
  }

  // see if we are also running the server on this machine
  trap_Cvar_VariableStringBuffer( "sv_running", var, sizeof( var ) );
  cgs.localServer = atoi( var );
}

/*
===============
CG_DoNeedAmmo
===============
*/
static qboolean CG_DoNeedAmmo( playerState_t *ps )
{
  weapon_t weapon = ps->stats[ STAT_WEAPON ];

  if (weapon == WP_NONE)
    return (qfalse);
  if (BG_Weapon( weapon )->infiniteAmmo)
    return (qfalse);
  if (BG_WeaponIsFull( weapon, ps->stats, ps->ammo, ps->clips ))
    return (qfalse);
  return (qtrue);
}

/*
===============
CG_SetUIVars

Set some cvars used by the UI
===============
*/
static void CG_SetUIVars( void )
{
  int   i;
  char  carriageCvar[ MAX_TOKEN_CHARS ];
  int   credit;
  playerState_t *ps;
  alienStates_t *alienStates = &cgs.alienStates;
  humanStates_t *humanStates = &cgs.humanStates;

  if( !cg.snap )
    return;

  ps = &cg.snap->ps;
  *carriageCvar = 0;

  //determine what the player is carrying
  if( BG_Weapon( cg.snap->ps.stats[ STAT_WEAPON ] )->purchasable )
    strcat( carriageCvar, va( "W%d ", cg.snap->ps.stats[ STAT_WEAPON ] ) );

  for( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
  {
    if( BG_InventoryContainsUpgrade( i, cg.snap->ps.stats ) &&
        BG_Upgrade( i )->purchasable )
      strcat( carriageCvar, va( "U%d ", i ) );
  }
  strcat( carriageCvar, "$" );

  trap_Cvar_Set( "ui_carriage", carriageCvar );

  credit = ps->persistant[ PERS_CREDIT ];
  trap_Cvar_Set( "ui_credit", va( "%d", credit > -1 ? credit : 0 ) );

  trap_Cvar_Set( "ui_ammoFull", va( "%d", !CG_DoNeedAmmo(ps)));

  trap_Cvar_Set( "ui_stages", va( "%d %d", cgs.alienStage, cgs.humanStage ) );

  trap_Cvar_Set( "ui_alienStates", va( "%d %d %d %d %d", alienStates->omBuilding, alienStates->omHealth,
      alienStates->spawns, alienStates->builders, alienStates->boosters ) );

  trap_Cvar_Set( "ui_humanStates", va( "%d %d %d %d %d %d %d", humanStates->rcBuilding, humanStates->rcHealth,
      humanStates->spawns, humanStates->builders, humanStates->armourys, humanStates->medicals, humanStates->computers ) );
}

/*
=================
CG_SetPVars
=================
*/
static void CG_SetPVars( void )
{
    playerState_t *ps;

    if( !cg.snap ) return;
    ps = &cg.snap->ps;

    trap_Cvar_Set( "player_hp", va( "%d", ps->stats[ STAT_HEALTH ] ));
    trap_Cvar_Set( "player_maxhp",va( "%d", ps->stats[ STAT_MAX_HEALTH ] ));

    switch( ps->stats[ STAT_TEAM ] )
    {
    case TEAM_NONE:
    trap_Cvar_Set( "team_bp", "0" );
    trap_Cvar_Set( "team_kns", "0" );
    trap_Cvar_Set( "team_teamname", "spectator" );
    trap_Cvar_Set( "team_stage", "0" );
    break;

    case TEAM_ALIENS:
    //trap_Cvar_Set( "team_bp", va( "%d", cgs.alienBuildPoints ));
    trap_Cvar_Set( "team_kns", va("%d", cgs.alienNextStageThreshold) );
    trap_Cvar_Set( "team_teamname", "aliens" );
    trap_Cvar_Set( "team_stage", va( "%d", cgs.alienStage+1 ) );
    break;

    case TEAM_HUMANS:
    //trap_Cvar_Set( "team_bp", va("%d",cgs.humanBuildPoints) );
    trap_Cvar_Set( "team_kns", va("%d",cgs.humanNextStageThreshold) );
    trap_Cvar_Set( "team_teamname", "humans" );
    trap_Cvar_Set( "team_stage", va( "%d", cgs.humanStage+1 ) );
    break;
    }

    trap_Cvar_Set( "player_credits", va( "%d", cg.snap->ps.persistant[ PERS_CREDIT ] ) );
    trap_Cvar_Set( "player_score", va( "%d", cg.snap->ps.persistant[ PERS_SCORE ] ) );

    if ( CG_LastAttacker( ) != -1 )
        trap_Cvar_Set( "player_attackername", cgs.clientinfo[ CG_LastAttacker( ) ].name );
    else
        trap_Cvar_Set( "player_attackername", "" );

    if ( CG_CrosshairPlayer( ) != -1 )
        trap_Cvar_Set( "player_crosshairname", cgs.clientinfo[ CG_CrosshairPlayer( ) ].name );
    else
        trap_Cvar_Set( "player_crosshairname", "" );

}

/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars( void )
{
  int         i;
  cvarTable_t *cv;

  CG_SetPVars( );

  for( i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++ )
    if( cv->vmCvar )
      trap_Cvar_Update( cv->vmCvar );

  // check for modications here

  CG_SetUIVars( );
  CG_UpdateBuildableRangeMarkerMask();
}


int CG_CrosshairPlayer( void )
{
  if( cg.time > ( cg.crosshairClientTime + 1000 ) )
    return -1;

  return cg.crosshairClientNum;
}


int CG_LastAttacker( void )
{
  if( !cg.attackerTime )
    return -1;

  return cg.snap->ps.persistant[ PERS_ATTACKER ];
}


/*
=================
CG_RemoveNotifyLine
=================
*/
void CG_RemoveNotifyLine( void )
{
  int i, offset, totalLength;

  if( cg.numConsoleLines == 0 )
    return;

  offset = cg.consoleLines[ 0 ].length;
  totalLength = strlen( cg.consoleText ) - offset;

  //slide up consoleText
  for( i = 0; i <= totalLength; i++ )
    cg.consoleText[ i ] = cg.consoleText[ i + offset ];

  //pop up the first consoleLine
  cg.numConsoleLines--;
  for( i = 0; i < cg.numConsoleLines; i++ )
    cg.consoleLines[ i ] = cg.consoleLines[ i + 1 ];
}

/*
=================
CG_AddNotifyText
=================
*/
void CG_AddNotifyText( void )
{
  char buffer[ BIG_INFO_STRING ];
  int bufferLen, textLen;

  trap_LiteralArgs( buffer, BIG_INFO_STRING );

  if( !buffer[ 0 ] )
  {
    cg.consoleText[ 0 ] = '\0';
    cg.numConsoleLines = 0;
    return;
  }

  bufferLen = strlen( buffer );
  textLen = strlen( cg.consoleText );

  // Ignore console messages that were just printed
  if( cg_noPrintDuplicate.integer && textLen >= bufferLen &&
      !strcmp( cg.consoleText + textLen - bufferLen, buffer ) )
    return;

  if( cg.numConsoleLines == MAX_CONSOLE_LINES )
  {
    CG_RemoveNotifyLine( );
    textLen = strlen( cg.consoleText );
  }

  Q_strncpyz( cg.consoleText + textLen, buffer, MAX_CONSOLE_TEXT - textLen );
  cg.consoleLines[ cg.numConsoleLines ].time = cg.time;
  cg.consoleLines[ cg.numConsoleLines ].length =
    MIN( bufferLen, MAX_CONSOLE_TEXT - textLen - 1 );
  cg.numConsoleLines++;
}

void QDECL CG_Printf( const char *msg, ... )
{
  va_list argptr;
  char    text[ 1024 ];

  va_start( argptr, msg );
  Q_vsnprintf( text, sizeof( text ), msg, argptr );
  va_end( argptr );

  trap_Print( text );
}

void QDECL CG_Error( const char *msg, ... )
{
  va_list argptr;
  char    text[ 1024 ];

  va_start( argptr, msg );
  Q_vsnprintf( text, sizeof( text ), msg, argptr );
  va_end( argptr );

  trap_Error( text );
}

void QDECL Com_Error( int level, const char *error, ... )
{
  va_list argptr;
  char    text[1024];

  va_start( argptr, error );
  Q_vsnprintf( text, sizeof( text ), error, argptr );
  va_end( argptr );

  CG_Error( "%s", text );
}

void QDECL Com_Printf( const char *msg, ... ) {
  va_list   argptr;
  char    text[1024];

  va_start( argptr, msg );
  Q_vsnprintf( text, sizeof( text ), msg, argptr );
  va_end( argptr );

  CG_Printf ("%s", text);
}



/*
================
CG_Argv
================
*/
const char *CG_Argv( int arg )
{
  static char buffer[ MAX_STRING_CHARS ];

  trap_Argv( arg, buffer, sizeof( buffer ) );

  return buffer;
}


//========================================================================

/*
=================
CG_FileExists

Test if a specific file exists or not
=================
*/
qboolean CG_FileExists( const char *filename )
{
  return trap_FS_FOpenFile( filename, NULL, FS_READ );
}

/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
static void CG_RegisterSounds( void )
{
  int         i;
  char        name[ MAX_QPATH ];
  const char  *soundName;

  cgs.media.alienStageTransition  = trap_S_RegisterSound( "sound/announcements/overmindevolved.wav", qtrue );
  cgs.media.humanStageTransition  = trap_S_RegisterSound( "sound/announcements/reinforcement.wav", qtrue );

  cgs.media.alienOvermindAttack   = trap_S_RegisterSound( "sound/announcements/overmindattack.wav", qtrue );
  cgs.media.alienOvermindDying    = trap_S_RegisterSound( "sound/announcements/overminddying.wav", qtrue );
  cgs.media.alienOvermindSpawns   = trap_S_RegisterSound( "sound/announcements/overmindspawns.wav", qtrue );

  cgs.media.alienL1Grab           = trap_S_RegisterSound( "sound/player/level1/grab.wav", qtrue );
  cgs.media.alienL4ChargePrepare  = trap_S_RegisterSound( "sound/player/level4/charge_prepare.wav", qtrue );
  cgs.media.alienL4ChargeStart    = trap_S_RegisterSound( "sound/player/level4/charge_start.wav", qtrue );

  cgs.media.tracerSound           = trap_S_RegisterSound( "sound/weapons/tracer.wav", qfalse );
  cgs.media.selectSound           = trap_S_RegisterSound( "sound/weapons/change.wav", qfalse );
  cgs.media.turretSpinupSound     = trap_S_RegisterSound( "sound/buildables/mgturret/spinup.wav", qfalse );
  cgs.media.weaponEmptyClick      = trap_S_RegisterSound( "sound/weapons/click.wav", qfalse );

  cgs.media.talkSound             = trap_S_RegisterSound( "sound/misc/talk.wav", qfalse );
  cgs.media.alienTalkSound        = trap_S_RegisterSound( "sound/misc/alien_talk.wav", qfalse );
  cgs.media.humanTalkSound        = trap_S_RegisterSound( "sound/misc/human_talk.wav", qfalse );
  cgs.media.landSound             = trap_S_RegisterSound( "sound/player/land1.wav", qfalse );

  cgs.media.watrInSound           = trap_S_RegisterSound( "sound/player/watr_in.wav", qfalse );
  cgs.media.watrOutSound          = trap_S_RegisterSound( "sound/player/watr_out.wav", qfalse );
  cgs.media.watrUnSound           = trap_S_RegisterSound( "sound/player/watr_un.wav", qfalse );

  cgs.media.disconnectSound       = trap_S_RegisterSound( "sound/misc/disconnect.wav", qfalse );

  for( i = 0; i < 4; i++ )
  {
    Com_sprintf( name, sizeof( name ), "sound/player/footsteps/step%i.wav", i + 1 );
    cgs.media.footsteps[ FOOTSTEP_NORMAL ][ i ] = trap_S_RegisterSound( name, qfalse );

    Com_sprintf( name, sizeof( name ), "sound/player/footsteps/flesh%i.wav", i + 1 );
    cgs.media.footsteps[ FOOTSTEP_FLESH ][ i ] = trap_S_RegisterSound( name, qfalse );

    Com_sprintf( name, sizeof( name ), "sound/player/footsteps/splash%i.wav", i + 1 );
    cgs.media.footsteps[ FOOTSTEP_SPLASH ][ i ] = trap_S_RegisterSound( name, qfalse );

    Com_sprintf( name, sizeof( name ), "sound/player/footsteps/clank%i.wav", i + 1 );
    cgs.media.footsteps[ FOOTSTEP_METAL ][ i ] = trap_S_RegisterSound( name, qfalse );
  }

  for( i = 1 ; i < MAX_SOUNDS ; i++ )
  {
    soundName = CG_ConfigString( CS_SOUNDS + i );

    if( !soundName[ 0 ] )
      break;

    if( soundName[ 0 ] == '*' )
      continue; // custom sound

    cgs.gameSounds[ i ] = trap_S_RegisterSound( soundName, qfalse );
  }

  cgs.media.jetpackDescendSound     = trap_S_RegisterSound( "sound/upgrades/jetpack/low.wav", qfalse );
  cgs.media.jetpackIdleSound        = trap_S_RegisterSound( "sound/upgrades/jetpack/idle.wav", qfalse );
  cgs.media.jetpackAscendSound      = trap_S_RegisterSound( "sound/upgrades/jetpack/hi.wav", qfalse );

  cgs.media.medkitUseSound          = trap_S_RegisterSound( "sound/upgrades/medkit/medkit.wav", qfalse );

  cgs.media.alienEvolveSound        = trap_S_RegisterSound( "sound/player/alienevolve.wav", qfalse );

  cgs.media.alienBuildableExplosion = trap_S_RegisterSound( "sound/buildables/alien/explosion.wav", qfalse );
  cgs.media.alienBuildableDamage    = trap_S_RegisterSound( "sound/buildables/alien/damage.wav", qfalse );
  cgs.media.alienBuildablePrebuild  = trap_S_RegisterSound( "sound/buildables/alien/prebuild.wav", qfalse );

  cgs.media.humanBuildableExplosion = trap_S_RegisterSound( "sound/buildables/human/explosion.wav", qfalse );
  cgs.media.humanBuildablePrebuild  = trap_S_RegisterSound( "sound/buildables/human/prebuild.wav", qfalse );

  for( i = 0; i < 4; i++ )
    cgs.media.humanBuildableDamage[ i ] = trap_S_RegisterSound(
        va( "sound/buildables/human/damage%d.wav", i ), qfalse );

  cgs.media.hardBounceSound1        = trap_S_RegisterSound( "sound/misc/hard_bounce1.wav", qfalse );
  cgs.media.hardBounceSound2        = trap_S_RegisterSound( "sound/misc/hard_bounce2.wav", qfalse );

  cgs.media.repeaterUseSound        = trap_S_RegisterSound( "sound/buildables/repeater/use.wav", qfalse );

  cgs.media.buildableRepairSound    = trap_S_RegisterSound( "sound/buildables/human/repair.wav", qfalse );
  cgs.media.buildableRepairedSound  = trap_S_RegisterSound( "sound/buildables/human/repaired.wav", qfalse );

  cgs.media.lCannonWarningSound     = trap_S_RegisterSound( "models/weapons/lcannon/warning.wav", qfalse );
  cgs.media.lCannonWarningSound2    = trap_S_RegisterSound( "models/weapons/lcannon/warning2.wav", qfalse );
}


//===================================================================================


#define CREEP_FRAMES          550
/*
=================
CG_RegisterGraphics

This function may execute for a couple of minutes with a slow disk.
=================
*/
static void CG_RegisterGraphics( void )
{
  int         i;
  int         animatedCreep = cg_animatedCreep.integer;

  static char *sb_nums[ 11 ] =
  {
    "gfx/2d/numbers/zero_32b",
    "gfx/2d/numbers/one_32b",
    "gfx/2d/numbers/two_32b",
    "gfx/2d/numbers/three_32b",
    "gfx/2d/numbers/four_32b",
    "gfx/2d/numbers/five_32b",
    "gfx/2d/numbers/six_32b",
    "gfx/2d/numbers/seven_32b",
    "gfx/2d/numbers/eight_32b",
    "gfx/2d/numbers/nine_32b",
    "gfx/2d/numbers/minus_32b",
  };
  static char *buildWeaponTimerPieShaders[ 8 ] =
  {
    "ui/assets/neutral/1_5pie",
    "ui/assets/neutral/3_0pie",
    "ui/assets/neutral/4_5pie",
    "ui/assets/neutral/6_0pie",
    "ui/assets/neutral/7_5pie",
    "ui/assets/neutral/9_0pie",
    "ui/assets/neutral/10_5pie",
    "ui/assets/neutral/12_0pie",
  };

  // clear any references to old media
  memset( &cg.refdef, 0, sizeof( cg.refdef ) );
  trap_R_ClearScene( );

  trap_R_LoadWorldMap( cgs.mapname );
  CG_UpdateMediaFraction( 0.66f );

  for( i = 0; i < 11; i++ )
    cgs.media.numberShaders[ i ] = trap_R_RegisterShader( sb_nums[ i ] );

  cgs.media.viewBloodShader           = trap_R_RegisterShader( "gfx/damage/fullscreen_painblend" );

  cgs.media.connectionShader          = trap_R_RegisterShader( "gfx/2d/net" );

  cgs.media.creepShader               = trap_R_RegisterShader( "creep" );

  if (animatedCreep > 0)
  {
    CG_UpdateMediaFraction( 0.67f );
    for ( i = 0; i < CREEP_FRAMES; i++ )
    {
      if (i % animatedCreep == 0 || i == CREEP_FRAMES - 1)
        cgs.media.creepAnimationShader[ i ] = trap_R_RegisterShader( va( "creep%d", i ) );
      else
        cgs.media.creepAnimationShader[ i ] = cgs.media.creepAnimationShader[ (i / animatedCreep) * animatedCreep ];
    }
    CG_UpdateMediaFraction( 0.685f );
  }

  cgs.media.scannerBlipShader         = trap_R_RegisterShader( "gfx/2d/blip" );
  cgs.media.scannerLineShader         = trap_R_RegisterShader( "gfx/2d/stalk" );

  cgs.media.teamOverlayShader         = trap_R_RegisterShader( "gfx/2d/teamoverlay" );

  cgs.media.tracerShader              = trap_R_RegisterShader( "gfx/misc/tracer" );

  cgs.media.backTileShader            = trap_R_RegisterShader( "console" );


  // building shaders
  cgs.media.greenBuildShader          = trap_R_RegisterShader("gfx/misc/greenbuild" );
  cgs.media.redBuildShader            = trap_R_RegisterShader("gfx/misc/redbuild" );
  cgs.media.humanSpawningShader       = trap_R_RegisterShader("models/buildables/telenode/rep_cyl" );

  for( i = 0; i < 8; i++ )
    cgs.media.buildWeaponTimerPie[ i ] = trap_R_RegisterShader( buildWeaponTimerPieShaders[ i ] );

  // player health cross shaders
  cgs.media.healthCross               = trap_R_RegisterShader( "ui/assets/neutral/cross.tga" );
  cgs.media.healthCross2X             = trap_R_RegisterShader( "ui/assets/neutral/cross2.tga" );
  cgs.media.healthCross3X             = trap_R_RegisterShader( "ui/assets/neutral/cross3.tga" );
  cgs.media.healthCrossMedkit         = trap_R_RegisterShader( "ui/assets/neutral/cross_medkit.tga" );
  cgs.media.healthCrossPoisoned       = trap_R_RegisterShader( "ui/assets/neutral/cross_poison.tga" );

  cgs.media.upgradeClassIconShader    = trap_R_RegisterShader( "icons/icona_upgrade.tga" );

  cgs.media.balloonShader             = trap_R_RegisterShader( "gfx/sprites/chatballoon" );
  cgs.media.aliensBalloonShader       = trap_R_RegisterShader( "gfx/sprites/aliens_chatballoon" );
  cgs.media.humansBalloonShader       = trap_R_RegisterShader( "gfx/sprites/humans_chatballoon" );

  cgs.media.disconnectPS              = CG_RegisterParticleSystem( "disconnectPS" );

  CG_UpdateMediaFraction( 0.7f );

  memset( cg_weapons, 0, sizeof( cg_weapons ) );
  memset( cg_upgrades, 0, sizeof( cg_upgrades ) );

  cgs.media.shadowMarkShader          = trap_R_RegisterShader( "gfx/marks/shadow" );
  cgs.media.wakeMarkShader            = trap_R_RegisterShader( "gfx/marks/wake" );

  cgs.media.poisonCloudPS             = CG_RegisterParticleSystem( "firstPersonPoisonCloudPS" );
  cgs.media.poisonCloudedPS           = CG_RegisterParticleSystem( "poisonCloudedPS" );
  cgs.media.alienEvolvePS             = CG_RegisterParticleSystem( "alienEvolvePS" );
  cgs.media.alienAcidTubePS           = CG_RegisterParticleSystem( "alienAcidTubePS" );

  cgs.media.jetPackDescendPS          = CG_RegisterParticleSystem( "jetPackDescendPS" );
  cgs.media.jetPackHoverPS            = CG_RegisterParticleSystem( "jetPackHoverPS" );
  cgs.media.jetPackAscendPS           = CG_RegisterParticleSystem( "jetPackAscendPS" );

  cgs.media.humanBuildableDamagedPS   = CG_RegisterParticleSystem( "humanBuildableDamagedPS" );
  cgs.media.alienBuildableDamagedPS   = CG_RegisterParticleSystem( "alienBuildableDamagedPS" );
  cgs.media.humanBuildableDestroyedPS = CG_RegisterParticleSystem( "humanBuildableDestroyedPS" );
  cgs.media.alienBuildableDestroyedPS = CG_RegisterParticleSystem( "alienBuildableDestroyedPS" );

  cgs.media.humanBuildableBleedPS     = CG_RegisterParticleSystem( "humanBuildableBleedPS");
  cgs.media.alienBuildableBleedPS     = CG_RegisterParticleSystem( "alienBuildableBleedPS" );

  cgs.media.alienBleedPS              = CG_RegisterParticleSystem( "alienBleedPS" );
  cgs.media.humanBleedPS              = CG_RegisterParticleSystem( "humanBleedPS" );

  cgs.media.sphereModel               = trap_R_RegisterModel( "models/generic/sphere" );
  cgs.media.sphericalCone64Model      = trap_R_RegisterModel( "models/generic/sphericalCone64" );
  cgs.media.sphericalCone240Model     = trap_R_RegisterModel( "models/generic/sphericalCone240" );

  cgs.media.plainColorShader          = trap_R_RegisterShader( "gfx/plainColor" );
  cgs.media.binaryAlpha1Shader        = trap_R_RegisterShader( "gfx/binary/alpha1" );

  for( i = 0; i < NUM_BINARY_SHADERS; ++i )
  {
    cgs.media.binaryShaders[ i ].f1 = trap_R_RegisterShader( va( "gfx/binary/%03i_F1", i ) );
    cgs.media.binaryShaders[ i ].f2 = trap_R_RegisterShader( va( "gfx/binary/%03i_F2", i ) );
    cgs.media.binaryShaders[ i ].f3 = trap_R_RegisterShader( va( "gfx/binary/%03i_F3", i ) );
    cgs.media.binaryShaders[ i ].b1 = trap_R_RegisterShader( va( "gfx/binary/%03i_B1", i ) );
    cgs.media.binaryShaders[ i ].b2 = trap_R_RegisterShader( va( "gfx/binary/%03i_B2", i ) );
    cgs.media.binaryShaders[ i ].b3 = trap_R_RegisterShader( va( "gfx/binary/%03i_B3", i ) );
  }

  CG_BuildableStatusParse( "ui/assets/human/buildstat.cfg", &cgs.humanBuildStat );
  CG_BuildableStatusParse( "ui/assets/alien/buildstat.cfg", &cgs.alienBuildStat );

  // register the inline models
  cgs.numInlineModels = trap_CM_NumInlineModels( );

  for( i = 1; i < cgs.numInlineModels; i++ )
  {
    char    name[ 10 ];
    vec3_t  mins, maxs;
    int     j;

    Com_sprintf( name, sizeof( name ), "*%i", i );

    cgs.inlineDrawModel[ i ] = trap_R_RegisterModel( name );
    trap_R_ModelBounds( cgs.inlineDrawModel[ i ], mins, maxs );

    for( j = 0 ; j < 3 ; j++ )
      cgs.inlineModelMidpoints[ i ][ j ] = mins[ j ] + 0.5 * ( maxs[ j ] - mins[ j ] );
  }

  // register all the server specified models
  for( i = 1; i < MAX_MODELS; i++ )
  {
    const char *modelName;

    modelName = CG_ConfigString( CS_MODELS + i );

    if( !modelName[ 0 ] )
      break;

    cgs.gameModels[ i ] = trap_R_RegisterModel( modelName );
  }

  CG_UpdateMediaFraction( 0.8f );

  // register all the server specified shaders
  for( i = 1; i < MAX_GAME_SHADERS; i++ )
  {
    const char *shaderName;

    shaderName = CG_ConfigString( CS_SHADERS + i );

    if( !shaderName[ 0 ] )
      break;

    cgs.gameShaders[ i ] = trap_R_RegisterShader( shaderName );
  }

  CG_UpdateMediaFraction( 0.9f );

  // register all the server specified particle systems
  for( i = 1; i < MAX_GAME_PARTICLE_SYSTEMS; i++ )
  {
    const char *psName;

    psName = CG_ConfigString( CS_PARTICLE_SYSTEMS + i );

    if( !psName[ 0 ] )
      break;

    cgs.gameParticleSystems[ i ] = CG_RegisterParticleSystem( (char *)psName );
  }
}


/*
=======================
CG_BuildSpectatorString

=======================
*/
void CG_BuildSpectatorString( void )
{
  int i;

  cg.spectatorList[ 0 ] = 0;

  for( i = 0; i < MAX_CLIENTS; i++ )
  {
    if( cgs.clientinfo[ i ].infoValid && cgs.clientinfo[ i ].team == TEAM_NONE )
    {
      Q_strcat( cg.spectatorList, sizeof( cg.spectatorList ),
          va( S_COLOR_WHITE "%s     ", cgs.clientinfo[ i ].name ) );
    }
  }
}



/*
===================
CG_RegisterClients

===================
*/
static void CG_RegisterClients( void )
{
  int   i;

  cg.charModelFraction = 0.0f;

  //precache all the models/sounds/etc
  for( i = PCL_NONE + 1; i < PCL_NUM_CLASSES;  i++ )
  {
    CG_PrecacheClientInfo( i, BG_ClassConfig( i )->modelName,
                              BG_ClassConfig( i )->skinName );

    cg.charModelFraction = (float)i / (float)PCL_NUM_CLASSES;
    trap_UpdateScreen( );
  }

  cgs.media.larmourHeadSkin    = trap_R_RegisterSkin( "models/players/human_base/head_light.skin" );
  cgs.media.larmourLegsSkin    = trap_R_RegisterSkin( "models/players/human_base/lower_light.skin" );
  cgs.media.larmourTorsoSkin   = trap_R_RegisterSkin( "models/players/human_base/upper_light.skin" );

  cgs.media.jetpackModel       = trap_R_RegisterModel( "models/players/human_base/jetpack.md3" );
  cgs.media.jetpackFlashModel  = trap_R_RegisterModel( "models/players/human_base/jetpack_flash.md3" );
  cgs.media.battpackModel      = trap_R_RegisterModel( "models/players/human_base/battpack.md3" );

  cg.charModelFraction = 1.0f;
  trap_UpdateScreen( );

  //load all the clientinfos of clients already connected to the server
  for( i = 0; i < MAX_CLIENTS; i++ )
  {
    const char  *clientInfo;

    clientInfo = CG_ConfigString( CS_PLAYERS + i );
    if( !clientInfo[ 0 ] )
      continue;

    CG_NewClientInfo( i );
  }

  CG_BuildSpectatorString( );
}

//===========================================================================

/*
=================
CG_ConfigString
=================
*/
const char *CG_ConfigString( int index )
{
  if( index < 0 || index >= MAX_CONFIGSTRINGS )
    CG_Error( "CG_ConfigString: bad index: %i", index );

  return cgs.gameState.stringData + cgs.gameState.stringOffsets[ index ];
}

//==================================================================

/*
======================
CG_StartMusic

======================
*/
void CG_StartMusic( void )
{
  char  *s;
  char  parm1[ MAX_QPATH ], parm2[ MAX_QPATH ];

  // start the background music
  s = (char *)CG_ConfigString( CS_MUSIC );
  Q_strncpyz( parm1, COM_Parse( &s ), sizeof( parm1 ) );
  Q_strncpyz( parm2, COM_Parse( &s ), sizeof( parm2 ) );

  trap_S_StartBackgroundTrack( parm1, parm2 );
}

/*
======================
CG_PlayerCount
======================
*/
int CG_PlayerCount( void )
{
  int i, count = 0;

  CG_RequestScores( );

  for( i = 0; i < cg.numScores; i++ )
  {
    if( cg.scores[ i ].team == TEAM_ALIENS ||
        cg.scores[ i ].team == TEAM_HUMANS )
      count++;
  }

  return count;
}

//
// ==============================
// new hud stuff ( mission pack )
// ==============================
//
char *CG_GetMenuBuffer( const char *filename )
{
  int           len;
  fileHandle_t  f;
  static char   buf[ MAX_MENUFILE ];

  len = trap_FS_FOpenFile( filename, &f, FS_READ );

  if( !f )
  {
    trap_Print( va( S_COLOR_RED "menu file not found: %s, using default\n", filename ) );
    return NULL;
  }

  if( len >= MAX_MENUFILE )
  {
    trap_Print( va( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i",
                    filename, len, MAX_MENUFILE ) );
    trap_FS_FCloseFile( f );
    return NULL;
  }

  trap_FS_Read( buf, len, f );
  buf[len] = 0;
  trap_FS_FCloseFile( f );

  return buf;
}

qboolean CG_Asset_Parse( int handle )
{
  pc_token_t token;
  const char *tempStr;

  if( !trap_Parse_ReadToken( handle, &token ) )
    return qfalse;

  if( Q_stricmp( token.string, "{" ) != 0 )
    return qfalse;

  while( 1 )
  {
    if( !trap_Parse_ReadToken( handle, &token ) )
      return qfalse;

    if( Q_stricmp( token.string, "}" ) == 0 )
      return qtrue;

    // font
    if( Q_stricmp( token.string, "font" ) == 0 )
    {
      int pointSize;

      if( !PC_String_Parse( handle, &tempStr ) || !PC_Int_Parse( handle, &pointSize ) )
        return qfalse;

      cgDC.registerFont( tempStr, pointSize, &cgDC.Assets.textFont );
      continue;
    }

    // smallFont
    if( Q_stricmp( token.string, "smallFont" ) == 0 )
    {
      int pointSize;

      if( !PC_String_Parse( handle, &tempStr ) || !PC_Int_Parse( handle, &pointSize ) )
        return qfalse;

      cgDC.registerFont( tempStr, pointSize, &cgDC.Assets.smallFont );
      continue;
    }

    // font
    if( Q_stricmp( token.string, "bigfont" ) == 0 )
    {
      int pointSize;

      if( !PC_String_Parse( handle, &tempStr ) || !PC_Int_Parse( handle, &pointSize ) )
        return qfalse;

      cgDC.registerFont( tempStr, pointSize, &cgDC.Assets.bigFont );
      continue;
    }

    // gradientbar
    if( Q_stricmp( token.string, "gradientbar" ) == 0 )
    {
      if( !PC_String_Parse( handle, &tempStr ) )
        return qfalse;

      cgDC.Assets.gradientBar = trap_R_RegisterShaderNoMip( tempStr );
      continue;
    }

    // enterMenuSound
    if( Q_stricmp( token.string, "menuEnterSound" ) == 0 )
    {
      if( !PC_String_Parse( handle, &tempStr ) )
        return qfalse;

      cgDC.Assets.menuEnterSound = trap_S_RegisterSound( tempStr, qfalse );
      continue;
    }

    // exitMenuSound
    if( Q_stricmp( token.string, "menuExitSound" ) == 0 )
    {
      if( !PC_String_Parse( handle, &tempStr ) )
        return qfalse;

      cgDC.Assets.menuExitSound = trap_S_RegisterSound( tempStr, qfalse );
      continue;
    }

    // itemFocusSound
    if( Q_stricmp( token.string, "itemFocusSound" ) == 0 )
    {
      if( !PC_String_Parse( handle, &tempStr ) )
        return qfalse;

      cgDC.Assets.itemFocusSound = trap_S_RegisterSound( tempStr, qfalse );
      continue;
    }

    // menuBuzzSound
    if( Q_stricmp( token.string, "menuBuzzSound" ) == 0 )
    {
      if( !PC_String_Parse( handle, &tempStr ) )
        return qfalse;

      cgDC.Assets.menuBuzzSound = trap_S_RegisterSound( tempStr, qfalse );
      continue;
    }

    if( Q_stricmp( token.string, "cursor" ) == 0 )
    {
      if( !PC_String_Parse( handle, &cgDC.Assets.cursorStr ) )
        return qfalse;

      cgDC.Assets.cursor = trap_R_RegisterShaderNoMip( cgDC.Assets.cursorStr );
      continue;
    }

    if( Q_stricmp( token.string, "fadeClamp" ) == 0 )
    {
      if( !PC_Float_Parse( handle, &cgDC.Assets.fadeClamp ) )
        return qfalse;

      continue;
    }

    if( Q_stricmp( token.string, "fadeCycle" ) == 0 )
    {
      if( !PC_Int_Parse( handle, &cgDC.Assets.fadeCycle ) )
        return qfalse;

      continue;
    }

    if( Q_stricmp( token.string, "fadeAmount" ) == 0 )
    {
      if( !PC_Float_Parse( handle, &cgDC.Assets.fadeAmount ) )
        return qfalse;

      continue;
    }

    if( Q_stricmp( token.string, "shadowX" ) == 0 )
    {
      if( !PC_Float_Parse( handle, &cgDC.Assets.shadowX ) )
        return qfalse;

      continue;
    }

    if( Q_stricmp( token.string, "shadowY" ) == 0 )
    {
      if( !PC_Float_Parse( handle, &cgDC.Assets.shadowY ) )
        return qfalse;

      continue;
    }

    if( Q_stricmp( token.string, "shadowColor" ) == 0 )
    {
      if( !PC_Color_Parse( handle, &cgDC.Assets.shadowColor ) )
        return qfalse;

      cgDC.Assets.shadowFadeClamp = cgDC.Assets.shadowColor[ 3 ];
      continue;
    }
  }

  return qfalse;
}

void CG_ParseMenu( const char *menuFile )
{
  pc_token_t  token;
  int         handle;

  handle = trap_Parse_LoadSource( menuFile );

  if( !handle )
    handle = trap_Parse_LoadSource( "ui/testhud.menu" );

  if( !handle )
    return;

  while( 1 )
  {
    if( !trap_Parse_ReadToken( handle, &token ) )
      break;

    //if ( Q_stricmp( token, "{" ) ) {
    //  Com_Printf( "Missing { in menu file\n" );
    //  break;
    //}

    //if ( menuCount == MAX_MENUS ) {
    //  Com_Printf( "Too many menus!\n" );
    //  break;
    //}

    if( token.string[ 0 ] == '}' )
      break;

    if( Q_stricmp( token.string, "assetGlobalDef" ) == 0 )
    {
      if( CG_Asset_Parse( handle ) )
        continue;
      else
        break;
    }


    if( Q_stricmp( token.string, "menudef" ) == 0 )
    {
      // start a new menu
      Menu_New( handle );
    }
  }

  trap_Parse_FreeSource( handle );
}

qboolean CG_Load_Menu( char **p )
{
  char *token;

  token = COM_ParseExt( p, qtrue );

  if( token[ 0 ] != '{' )
    return qfalse;

  while( 1 )
  {
    token = COM_ParseExt( p, qtrue );

    if( Q_stricmp( token, "}" ) == 0 )
      return qtrue;

    if( !token || token[ 0 ] == 0 )
      return qfalse;

    CG_ParseMenu( token );
  }
  return qfalse;
}



void CG_LoadMenus( const char *menuFile )
{
  char          *token;
  char          *p;
  int           len, start;
  fileHandle_t  f;
  static char   buf[ MAX_MENUDEFFILE ];

  start = trap_Milliseconds( );

  len = trap_FS_FOpenFile( menuFile, &f, FS_READ );

  if( !f )
  {
    trap_Error( va( S_COLOR_YELLOW "menu file not found: %s, using default", menuFile ) );
    len = trap_FS_FOpenFile( "ui/hud.txt", &f, FS_READ );

    if( !f )
      trap_Error( va( S_COLOR_RED "default menu file not found: ui/hud.txt, unable to continue!" ) );
  }

  if( len >= MAX_MENUDEFFILE )
  {
    trap_Error( va( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i",
                menuFile, len, MAX_MENUDEFFILE ) );
    trap_FS_FCloseFile( f );
    return;
  }

  trap_FS_Read( buf, len, f );
  buf[ len ] = 0;
  trap_FS_FCloseFile( f );

  COM_Compress( buf );

  Menu_Reset( );

  p = buf;

  while( 1 )
  {
    token = COM_ParseExt( &p, qtrue );

    if( !token || token[ 0 ] == 0 || token[ 0 ] == '}' )
      break;

    if( Q_stricmp( token, "}" ) == 0 )
      break;

    if( Q_stricmp( token, "loadmenu" ) == 0 )
    {
      if( CG_Load_Menu( &p ) )
        continue;
      else
        break;
    }
  }

  Com_Printf( "UI menu load time = %d milli seconds\n", trap_Milliseconds( ) - start );
}



static qboolean CG_OwnerDrawHandleKey( int ownerDraw, int key )
{
  return qfalse;
}


static int CG_FeederCount( int feederID )
{
  int i, count = 0;

  if( feederID == FEEDER_ALIENTEAM_LIST )
  {
    for( i = 0; i < cg.numScores; i++ )
    {
      if( cg.scores[ i ].team == TEAM_ALIENS )
        count++;
    }
  }
  else if( feederID == FEEDER_HUMANTEAM_LIST )
  {
    for( i = 0; i < cg.numScores; i++ )
    {
      if( cg.scores[ i ].team == TEAM_HUMANS )
        count++;
    }
  }

  return count;
}


void CG_SetScoreSelection( void *p )
{
  menuDef_t     *menu = (menuDef_t*)p;
  playerState_t *ps = &cg.snap->ps;
  int           i, alien, human;
  int           feeder;

  alien = human = 0;

  for( i = 0; i < cg.numScores; i++ )
  {
    if( cg.scores[ i ].team == TEAM_ALIENS )
      alien++;
    else if( cg.scores[ i ].team == TEAM_HUMANS )
      human++;

    if( ps->clientNum == cg.scores[ i ].client )
      cg.selectedScore = i;
  }

  if( menu == NULL )
    // just interested in setting the selected score
    return;

  feeder = FEEDER_ALIENTEAM_LIST;
  i = alien;

  if( cg.scores[ cg.selectedScore ].team == TEAM_HUMANS )
  {
    feeder = FEEDER_HUMANTEAM_LIST;
    i = human;
  }

  Menu_SetFeederSelection(menu, feeder, i, NULL);
}

// FIXME: might need to cache this info
static clientInfo_t * CG_InfoFromScoreIndex( int index, int team, int *scoreIndex )
{
  int i, count;
  count = 0;

  for( i = 0; i < cg.numScores; i++ )
  {
    if( cg.scores[ i ].team == team )
    {
      if( count == index )
      {
        *scoreIndex = i;
        return &cgs.clientinfo[ cg.scores[ i ].client ];
      }
      count++;
    }
  }

  *scoreIndex = index;
  return &cgs.clientinfo[ cg.scores[ index ].client ];
}

qboolean CG_ClientIsReady( int clientNum )
{
  clientList_t ready;

  Com_ClientListParse( &ready, CG_ConfigString( CS_CLIENTS_READY ) );

  return Com_ClientListContains( &ready, clientNum );
}

static const char *CG_FeederItemText( int feederID, int index, int column, qhandle_t *handle )
{
  int           scoreIndex = 0;
  clientInfo_t  *info = NULL;
  int           team = -1;
  score_t       *sp = NULL;
  qboolean      showIcons = qfalse;

  *handle = -1;

  if( feederID == FEEDER_ALIENTEAM_LIST )
    team = TEAM_ALIENS;
  else if( feederID == FEEDER_HUMANTEAM_LIST )
    team = TEAM_HUMANS;

  info = CG_InfoFromScoreIndex( index, team, &scoreIndex );
  sp = &cg.scores[ scoreIndex ];

  if( cg.intermissionStarted && CG_ClientIsReady( sp->client ) )
    showIcons = qfalse;
  else if( cg.snap->ps.pm_type == PM_SPECTATOR ||
           cg.snap->ps.pm_type == PM_NOCLIP ||
           cg.snap->ps.pm_flags & PMF_FOLLOW ||
           team == cg.snap->ps.stats[ STAT_TEAM ] ||
           cg.intermissionStarted )
  {
    showIcons = qtrue;
  }

  if( info && info->infoValid )
  {
    switch( column )
    {
      case 0:
        if( showIcons )
        {
          if( sp->weapon != WP_NONE )
            *handle = cg_weapons[ sp->weapon ].weaponIcon;
        }
        break;

      case 1:
        if( showIcons )
        {
          if( sp->team == TEAM_HUMANS && sp->upgrade != UP_NONE )
            *handle = cg_upgrades[ sp->upgrade ].upgradeIcon;
          else if( sp->team == TEAM_ALIENS )
          {
            switch( sp->weapon )
            {
              case WP_ABUILD2:
              case WP_ALEVEL1_UPG:
              case WP_ALEVEL2_UPG:
              case WP_ALEVEL3_UPG:
                *handle = cgs.media.upgradeClassIconShader;
                break;

              default:
                break;
            }
          }
        }
        break;

      case 2:
        if( cg.intermissionStarted && CG_ClientIsReady( sp->client ) )
          return "Ready";
        break;

      case 3:
        return va( S_COLOR_WHITE "%s", info->name );
        break;

      case 4:
        return va( "%d", sp->score );
        break;

      case 5:
        return va( "%4d", sp->time );
        break;

      case 6:
        if( sp->ping == -1 )
          return "";

        return va( "%4d", sp->ping );
        break;
    }
  }

  return "";
}

static qhandle_t CG_FeederItemImage( int feederID, int index )
{
  return 0;
}

static void CG_FeederSelection( int feederID, int index )
{
  int i, count;
  int team = ( feederID == FEEDER_ALIENTEAM_LIST ) ? TEAM_ALIENS : TEAM_HUMANS;
  count = 0;

  for( i = 0; i < cg.numScores; i++ )
  {
    if( cg.scores[ i ].team == team )
    {
      if( index == count )
        cg.selectedScore = i;

      count++;
    }
  }
}

static float CG_Cvar_Get( const char *cvar )
{
  char buff[ 128 ];

  memset( buff, 0, sizeof( buff ) );
  trap_Cvar_VariableStringBuffer( cvar, buff, sizeof( buff ) );
  return atof( buff );
}

void CG_Text_PaintWithCursor( float x, float y, float scale, vec4_t color, const char *text,
                              int cursorPos, char cursor, int limit, int style )
{
  UI_Text_Paint( x, y, scale, color, text, 0, limit, style );
}

static int CG_OwnerDrawWidth( int ownerDraw, float scale )
{
  switch( ownerDraw )
  {
    case CG_KILLER:
      return UI_Text_Width( CG_GetKillerText( ), scale );
      break;
  }

  return 0;
}

static int CG_PlayCinematic( const char *name, float x, float y, float w, float h )
{
  return trap_CIN_PlayCinematic( name, x, y, w, h, CIN_loop );
}

static void CG_StopCinematic( int handle )
{
  trap_CIN_StopCinematic( handle );
}

static void CG_DrawCinematic( int handle, float x, float y, float w, float h )
{
  trap_CIN_SetExtents( handle, x, y, w, h );
  trap_CIN_DrawCinematic( handle );
}

static void CG_RunCinematicFrame( int handle )
{
  trap_CIN_RunCinematic( handle );
}

// hack to prevent warning
static qboolean CG_OwnerDrawVisible( int parameter )
{
  return qfalse;
}

/*
=================
CG_LoadHudMenu
=================
*/
void CG_LoadHudMenu( void )
{
  char        buff[ 1024 ];
  const char  *hudSet;

  cgDC.aspectScale = ( ( 640.0f * cgs.glconfig.vidHeight ) /
                       ( 480.0f * cgs.glconfig.vidWidth ) );
  cgDC.xscale = cgs.glconfig.vidWidth / 640.0f;
  cgDC.yscale = cgs.glconfig.vidHeight / 480.0f;

  cgDC.smallFontScale = CG_Cvar_Get( "ui_smallFont" );
  cgDC.bigFontScale = CG_Cvar_Get( "ui_bigFont" );

  cgDC.registerShaderNoMip  = &trap_R_RegisterShaderNoMip;
  cgDC.setColor             = &trap_R_SetColor;
  cgDC.drawHandlePic        = &CG_DrawPic;
  cgDC.drawStretchPic       = &trap_R_DrawStretchPic;
  cgDC.registerModel        = &trap_R_RegisterModel;
  cgDC.modelBounds          = &trap_R_ModelBounds;
  cgDC.fillRect             = &CG_FillRect;
  cgDC.fillRoundedRect      = &CG_FillRoundedRect;
  cgDC.drawRect             = &CG_DrawRect;
  cgDC.drawRoundedRect      = &CG_DrawRoundedRect;
  cgDC.drawSides            = &CG_DrawSides;
  cgDC.drawTopBottom        = &CG_DrawTopBottom;
  cgDC.clearScene           = &trap_R_ClearScene;
  cgDC.addRefEntityToScene  = &trap_R_AddRefEntityToScene;
  cgDC.renderScene          = &trap_R_RenderScene;
  cgDC.registerFont         = &trap_R_RegisterFont;
  cgDC.ownerDrawItem        = &CG_OwnerDraw;
  cgDC.getValue             = &CG_GetValue;
  cgDC.ownerDrawVisible     = &CG_OwnerDrawVisible;
  cgDC.runScript            = &CG_RunMenuScript;
  cgDC.setCVar              = trap_Cvar_Set;
  cgDC.getCVarString        = trap_Cvar_VariableStringBuffer;
  cgDC.getCVarValue         = CG_Cvar_Get;
  cgDC.setOverstrikeMode    = &trap_Key_SetOverstrikeMode;
  cgDC.getOverstrikeMode    = &trap_Key_GetOverstrikeMode;
  cgDC.startLocalSound      = &trap_S_StartLocalSound;
  cgDC.ownerDrawHandleKey   = &CG_OwnerDrawHandleKey;
  cgDC.feederCount          = &CG_FeederCount;
  cgDC.feederItemImage      = &CG_FeederItemImage;
  cgDC.feederItemText       = &CG_FeederItemText;
  cgDC.feederSelection      = &CG_FeederSelection;
  //cgDC.setBinding           = &trap_Key_SetBinding;
  //cgDC.getBindingBuf        = &trap_Key_GetBindingBuf;
  //cgDC.keynumToStringBuf    = &trap_Key_KeynumToStringBuf;
  //cgDC.executeText          = &trap_Cmd_ExecuteText;
  cgDC.Error                = &Com_Error;
  cgDC.Print                = &Com_Printf;
  cgDC.ownerDrawWidth       = &CG_OwnerDrawWidth;
  //cgDC.ownerDrawText        = &CG_OwnerDrawText;
  //cgDC.Pause                = &CG_Pause;
  cgDC.registerSound        = &trap_S_RegisterSound;
  cgDC.startBackgroundTrack = &trap_S_StartBackgroundTrack;
  cgDC.stopBackgroundTrack  = &trap_S_StopBackgroundTrack;
  cgDC.playCinematic        = &CG_PlayCinematic;
  cgDC.stopCinematic        = &CG_StopCinematic;
  cgDC.drawCinematic        = &CG_DrawCinematic;
  cgDC.runCinematicFrame    = &CG_RunCinematicFrame;
  cgDC.Bucket_Create_Bucket = BG_Bucket_Create_Bucket;
  cgDC.Bucket_Delete_Bucket = BG_Bucket_Delete_Bucket;
  cgDC.Bucket_Destroy_All_Buckets = BG_Bucket_Destroy_All_Buckets;
  cgDC.Bucket_Add_Item_To_Bucket = BG_Bucket_Add_Item_To_Bucket;
  cgDC.Bucket_Remove_Item_From_Bucket = BG_Bucket_Remove_Item_From_Bucket;
  cgDC.Bucket_Select_A_Random_Item = BG_Bucket_Select_A_Random_Item;
  cgDC.Bucket_Select_A_Specific_Item = BG_Bucket_Select_A_Specific_Item;
  cgDC.FS_GetFileList = trap_FS_GetFileList;

  Init_Display( &cgDC );

  Menu_Reset( );

  trap_Cvar_VariableStringBuffer( "cg_hudFiles", buff, sizeof( buff ) );
  hudSet = buff;

  if( hudSet[ 0 ] == '\0' )
    hudSet = "ui/hud.txt";

  CG_LoadMenus( hudSet );
}

void CG_AssetCache( void )
{
  int i;

  cgDC.Assets.gradientBar         = trap_R_RegisterShaderNoMip( ASSET_GRADIENTBAR );
  cgDC.Assets.scrollBar           = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR );
  cgDC.Assets.scrollBarArrowDown  = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWDOWN );
  cgDC.Assets.scrollBarArrowUp    = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWUP );
  cgDC.Assets.scrollBarArrowLeft  = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWLEFT );
  cgDC.Assets.scrollBarArrowRight = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWRIGHT );
  cgDC.Assets.scrollBarThumb      = trap_R_RegisterShaderNoMip( ASSET_SCROLL_THUMB );
  cgDC.Assets.showMoreArrow       = trap_R_RegisterShaderNoMip( ASSET_SHOWMORE_ARROW );
  cgDC.Assets.sliderBar           = trap_R_RegisterShaderNoMip( ASSET_SLIDER_BAR );
  cgDC.Assets.sliderThumb         = trap_R_RegisterShaderNoMip( ASSET_SLIDER_THUMB );

  cgDC.Assets.cornerIn[BORDER_SQUARE]         = trap_R_RegisterShaderNoMip( ASSET_CORNERIN_SQUARE );
  cgDC.Assets.cornerOut[BORDER_SQUARE]        = trap_R_RegisterShaderNoMip( ASSET_CORNEROUT_SQUARE );
  cgDC.Assets.cornerIn[BORDER_ROUNDED]        = trap_R_RegisterShaderNoMip( ASSET_CORNERIN_ROUNDED );
  cgDC.Assets.cornerOut[BORDER_ROUNDED]       = trap_R_RegisterShaderNoMip( ASSET_CORNEROUT_ROUNDED );
  cgDC.Assets.cornerIn[BORDER_FOLD]           = trap_R_RegisterShaderNoMip( ASSET_CORNERIN_FOLD );
  cgDC.Assets.cornerOut[BORDER_FOLD]          = trap_R_RegisterShaderNoMip( ASSET_CORNEROUT_FOLD );

  if( cg_emoticons.integer )
  {
    cgDC.Assets.emoticonCount = BG_LoadEmoticons( cgDC.Assets.emoticons,
      MAX_EMOTICONS );
  }
  else
    cgDC.Assets.emoticonCount = 0;

  for( i = 0; i < cgDC.Assets.emoticonCount; i++ )
  {
    cgDC.Assets.emoticons[ i ].shader = trap_R_RegisterShaderNoMip(
      va( "emoticons/%s_%dx1.tga", cgDC.Assets.emoticons[ i ].name,
          cgDC.Assets.emoticons[ i ].width ) );
  }
}

/*
=================
CG_Init

Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
=================
*/
void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum )
{
  const char  *s;

  // clear everything
  memset( &cgs, 0, sizeof( cgs ) );
  memset( &cg, 0, sizeof( cg ) );
  memset( cg_entities, 0, sizeof( cg_entities ) );

  cg.clientNum = clientNum;

  cgs.processedSnapshotNum = serverMessageNum;
  cgs.serverCommandSequence = serverCommandSequence;

  // get the rendering configuration from the client system
  trap_GetGlconfig( &cgs.glconfig );
  cgs.screenXScale = cgs.glconfig.vidWidth / 640.0f;
  cgs.screenYScale = cgs.glconfig.vidHeight / 480.0f;

  // load a few needed things before we do any screen updates
  cgs.media.whiteShader     = trap_R_RegisterShader( "white" );
  cgs.media.charsetShader   = trap_R_RegisterShader( "gfx/2d/bigchars" );
  cgs.media.outlineShader   = trap_R_RegisterShader( "outline" );

  // load overrides
  BG_InitClassConfigs( );
  BG_InitBuildableConfigs( );
  BG_InitAllowedGameElements( );

  // Dynamic memory
  BG_InitMemory( );

  CG_RegisterCvars( );

  CG_InitConsoleCommands( );

  String_Init( );

  CG_AssetCache( );
  CG_LoadHudMenu( );

  cg.weaponSelect = WP_NONE;

  // old servers

  // get the gamestate from the client system
  trap_GetGameState( &cgs.gameState );

  // copy vote display strings so they don't show up blank if we see
  // the same one directly after connecting
  Q_strncpyz( cgs.voteString[ TEAM_NONE ],
      CG_ConfigString( CS_VOTE_STRING + TEAM_NONE ),
      sizeof( cgs.voteString ) );
  Q_strncpyz( cgs.voteString[ TEAM_ALIENS ],
      CG_ConfigString( CS_VOTE_STRING + TEAM_ALIENS ),
      sizeof( cgs.voteString[ TEAM_ALIENS ] ) );
  Q_strncpyz( cgs.voteString[ TEAM_HUMANS ],
      CG_ConfigString( CS_VOTE_STRING + TEAM_ALIENS ),
      sizeof( cgs.voteString[ TEAM_HUMANS ] ) );

  // check version
  s = CG_ConfigString( CS_GAME_VERSION );

  if( strcmp( s, GAME_VERSION ) )
    CG_Error( "Client/Server game mismatch: %s/%s", GAME_VERSION, s );

  s = CG_ConfigString( CS_LEVEL_START_TIME );
  cgs.levelStartTime = atoi( s );

  CG_ParseServerinfo( );

  // load the new map
  trap_CM_LoadMap( cgs.mapname );

  cg.loading = qtrue;   // force players to load instead of defer

  CG_LoadTrailSystems( );
  CG_UpdateMediaFraction( 0.05f );

  CG_LoadParticleSystems( );
  CG_UpdateMediaFraction( 0.05f );

  CG_RegisterSounds( );
  CG_UpdateMediaFraction( 0.60f );

  CG_RegisterGraphics( );
  CG_UpdateMediaFraction( 0.90f );

  CG_InitWeapons( );
  CG_UpdateMediaFraction( 0.95f );

  CG_InitUpgrades( );
  CG_UpdateMediaFraction( 1.0f );

  CG_InitBuildables( );

  cgs.voices = BG_VoiceInit( );
  BG_PrintVoices( cgs.voices, cg_debugVoices.integer );

  CG_RegisterClients( );   // if low on memory, some clients will be deferred

  cg.loading = qfalse;  // future players will be deferred

  CG_InitMarkPolys( );

  // remove the last loading update
  cg.infoScreenText[ 0 ] = 0;

  // Make sure we have update values (scores)
  CG_SetConfigValues( );

  CG_StartMusic( );

  CG_ShaderStateChanged( );

  trap_S_ClearLoopingSounds( qtrue );
}

/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
void CG_Shutdown( void )
{
  BG_Bucket_Destroy_All_Buckets( );
  CG_UnregisterCommands( );
}

/*
================
CG_VoIPString
================
*/
static char *CG_VoIPString( void )
{
  // a generous overestimate of the space needed for 0,1,2...61,62,63
  static char voipString[ MAX_CLIENTS * 4 ];
  char voipSendTarget[ MAX_CVAR_VALUE_STRING ];

  trap_Cvar_VariableStringBuffer( "cl_voipSendTarget", voipSendTarget,
                                  sizeof( voipSendTarget ) );

  if( Q_stricmp( voipSendTarget, "team" ) == 0 )
  {
    int i, slen, nlen;
    for( slen = i = 0; i < cgs.maxclients; i++ )
    {
      if( !cgs.clientinfo[ i ].infoValid || i == cg.clientNum )
        continue;
      if( cgs.clientinfo[ i ].team != cgs.clientinfo[ cg.clientNum ].team )
        continue;

      nlen = Q_snprintf( &voipString[ slen ], sizeof( voipString ) - slen,
                         "%s%d", ( slen > 0 ) ? "," : "", i );
      if( slen + nlen + 1 >= sizeof( voipString ) )
      {
        CG_Printf( S_COLOR_YELLOW "WARNING: voipString overflowed\n" );
        break;
      }

      slen += nlen;
    }

    // Notice that if the snprintf was truncated, slen was not updated
    // so this will remove any trailing commas or partially-completed numbers
    voipString[ slen ] = '\0';
  }
  else if( Q_stricmp( voipSendTarget, "crosshair" ) == 0 )
    Com_sprintf( voipString, sizeof( voipString ), "%d",
                 CG_CrosshairPlayer( ) );
  else if( Q_stricmp( voipSendTarget, "attacker" ) == 0 )
    Com_sprintf( voipString, sizeof( voipString ), "%d",
                 CG_LastAttacker( ) );
  else
    return NULL;

  return voipString;
}

#ifdef MODULE_INTERFACE_11
int trap_S_SoundDuration( sfxHandle_t handle )
{
  return 1;
}

void trap_R_SetClipRegion( const float *region )
{
}

static qboolean keyOverstrikeMode = qfalse;

void trap_Key_SetOverstrikeMode( qboolean state )
{
  keyOverstrikeMode = state;
}

qboolean trap_Key_GetOverstrikeMode( void )
{
  return keyOverstrikeMode;
}
#endif
