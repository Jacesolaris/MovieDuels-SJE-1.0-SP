/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#include "cg_media.h"
#include "FxScheduler.h"

#include "../client/vmachine.h"
#include "g_local.h"

#include "../qcommon/sstring.h"
#include "qcommon/ojk_saved_game_helper.h"
#include "../game/wp_saber.h"

//NOTENOTE: Be sure to change the mirrored code in g_shared.h
using namePrecache_m = std::map<sstring_t, unsigned char>;
extern namePrecache_m* as_preCacheMap;
extern void CG_RegisterNPCCustomSounds(clientInfo_t* ci);

extern Vehicle_t* G_IsRidingVehicle(const gentity_t* p_ent);
extern int G_ParseAnimFileSet(const char* skeleton_name, const char* model_name = nullptr);
extern void CG_DrawDataPadInventorySelect();
vmCvar_t cg_com_kotor;

void CG_Init(int serverCommandSequence);
qboolean CG_ConsoleCommand();
void CG_Shutdown();
int CG_GetCameraPos(vec3_t camerapos);
int CG_GetCameraAng(vec3_t cameraang);
void UseItem(int item_num);
const char* CG_DisplayBoxedText(int iBoxX, int iBoxY, int iBoxWidth, int iBoxHeight,
	const char* psText, int iFontHandle, float fScale,
	const vec4_t v4Color);

constexpr auto NUM_CHUNKS = 6;
extern void G_StartNextItemEffect(gentity_t* ent, int me_flags = 0, int length = 1000, float time_scale = 0.0f,
	int spin_time = 0);
/*
Ghoul2 Insert Start
*/

void CG_ResizeG2Bolt(boltInfo_v* bolt, int newCount);
void CG_ResizeG2Surface(surfaceInfo_v* surface, int newCount);
void CG_ResizeG2Bone(boneInfo_v* bone, int newCount);
void CG_ResizeG2(CGhoul2Info_v* ghoul2, int newCount);
void CG_ResizeG2TempBone(mdxaBone_v* tempBone, int newCount);
/*
Ghoul2 Insert End
*/

void CG_LoadHudMenu();
int inv_icons[INV_MAX];
const char* inv_names[] =
{
	"ELECTROBINOCULARS",
	"BACTA CANISTER",
	"SEEKER",
	"LIGHT AMP GOGGLES",
	"ASSAULT SENTRY",
	"GOODIE KEY",
	"GOODIE KEY",
	"GOODIE KEY",
	"GOODIE KEY",
	"GOODIE KEY",
	"SECURITY KEY",
	"SECURITY KEY",
	"SECURITY KEY",
	"SECURITY KEY",
	"SECURITY KEY",
	"CLOAK",
	"BARRIER",
	"GRAPPLEHOOK",
};

int force_icons[NUM_FORCE_POWERS];

void CG_DrawDataPadHUD(const centity_t* cent);
void CG_DrawDataPadObjectives(const centity_t* cent);
void CG_DrawIconBackground();
void CG_DrawSJEIconBackground();
void CG_DrawDataPadIconBackground(int background_type);
void CG_DrawDataPadWeaponSelect();
void CG_DrawDataPadWeaponSelect_kotor();
void CG_DrawDataPadForceSelect();

/*
================
vmMain

This is the only way control passes into the cgame module.
This must be the very first function compiled into the .q3vm file
================
*/
extern "C" Q_EXPORT intptr_t QDECL vmMain(const intptr_t command, const intptr_t arg0, intptr_t arg1, intptr_t arg2,
	intptr_t arg3,
	intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7)
{
	centity_t* cent;

	switch (command)
	{
	case CG_INIT:
		CG_Init(arg0);
		return 0;
	case CG_SHUTDOWN:
		CG_Shutdown();
		return 0;
	case CG_CONSOLE_COMMAND:
		return CG_ConsoleCommand();
	case CG_DRAW_ACTIVE_FRAME:
		CG_DrawActiveFrame(arg0, static_cast<stereoFrame_t>(arg1));
		return 0;
	case CG_CROSSHAIR_PLAYER:
		return CG_CrosshairPlayer();
	case CG_CAMERA_POS:
		return CG_GetCameraPos(reinterpret_cast<float*>(arg0));
	case CG_CAMERA_ANG:
		return CG_GetCameraAng(reinterpret_cast<float*>(arg0));
		/*
		Ghoul2 Insert Start
		*/
	case CG_RESIZE_G2:
		CG_ResizeG2(reinterpret_cast<CGhoul2Info_v*>(arg0), arg1);
		return 0;
	case CG_RESIZE_G2_BOLT:
		CG_ResizeG2Bolt(reinterpret_cast<boltInfo_v*>(arg0), arg1);
		return 0;
	case CG_RESIZE_G2_BONE:
		CG_ResizeG2Bone(reinterpret_cast<boneInfo_v*>(arg0), arg1);
		return 0;
	case CG_RESIZE_G2_SURFACE:
		CG_ResizeG2Surface(reinterpret_cast<surfaceInfo_v*>(arg0), arg1);
		return 0;
	case CG_RESIZE_G2_TEMPBONE:
		CG_ResizeG2TempBone(reinterpret_cast<mdxaBone_v*>(arg0), arg1);
		return 0;

		/*
		Ghoul2 Insert End
		*/
	case CG_DRAW_DATAPAD_HUD:
		if (cg.snap)
		{
			cent = &cg_entities[cg.snap->ps.client_num];
			CG_DrawDataPadHUD(cent);
		}
		return 0;

	case CG_DRAW_DATAPAD_OBJECTIVES:
		if (cg.snap)
		{
			cent = &cg_entities[cg.snap->ps.client_num];
			CG_DrawDataPadObjectives(cent);
		}
		return 0;

	case CG_DRAW_DATAPAD_WEAPONS:
		if (cg.snap)
		{
			CG_DrawDataPadIconBackground(ICON_WEAPONS);
			if (cg_com_kotor.integer == 1) //playing kotor
			{
				CG_DrawDataPadWeaponSelect_kotor();
			}
			else
			{
				cent = &cg_entities[cg.snap->ps.client_num];
				if (cent->gent->friendlyfaction == FACTION_KOTOR)
				{
					CG_DrawDataPadWeaponSelect_kotor();
				}
				else
				{
					CG_DrawDataPadWeaponSelect();
				}
			}
		}
		return 0;
	case CG_DRAW_DATAPAD_INVENTORY:
		if (cg.snap)
		{
			CG_DrawDataPadIconBackground(ICON_INVENTORY);
			CG_DrawDataPadInventorySelect();
		}
		return 0;
	case CG_DRAW_DATAPAD_FORCEPOWERS:
		if (cg.snap)
		{
			CG_DrawDataPadIconBackground(ICON_FORCE);
			CG_DrawDataPadForceSelect();
		}
		return 0;
	default:;
	}
	return -1;
}

vmCvar_t r_ratiofix;

static void CG_Set2DRatio()
{
	if (r_ratiofix.integer)
	{
		cgs.widthRatioCoef = static_cast<float>(SCREEN_WIDTH * cgs.glconfig.vidHeight) / static_cast<float>(
			SCREEN_HEIGHT * cgs.glconfig.vidWidth);
	}
	else
	{
		cgs.widthRatioCoef = 1.0f;
	}
}

/*
Ghoul2 Insert Start
*/

void CG_ResizeG2Bolt(boltInfo_v* bolt, const int newCount)
{
	bolt->resize(newCount);
}

void CG_ResizeG2Surface(surfaceInfo_v* surface, const int newCount)
{
	surface->resize(newCount);
}

void CG_ResizeG2Bone(boneInfo_v* bone, const int newCount)
{
	bone->resize(newCount);
}

void CG_ResizeG2(CGhoul2Info_v* ghoul2, const int newCount)
{
	ghoul2->resize(newCount);
}

void CG_ResizeG2TempBone(mdxaBone_v* tempBone, const int newCount)
{
	tempBone->resize(newCount);
}

/*
Ghoul2 Insert End
*/

cg_t cg;
cgs_t cgs;
centity_t cg_entities[MAX_GENTITIES];

centity_t* cg_permanents[MAX_GENTITIES];
int cg_numpermanents = 0;

weaponInfo_t cg_weapons[MAX_WEAPONS];
itemInfo_t cg_items[MAX_ITEMS];

using inventoryInfo_t = struct
{
	qboolean registered; // Has the player picked it up
	qboolean active; // Is it the chosen inventory item
	int count; // Count of items.
	char description[128];
};

inventoryInfo_t cg_inventory[INV_MAX];

vmCvar_t cg_runpitch;
vmCvar_t cg_runroll;
vmCvar_t cg_bobup;
vmCvar_t cg_bobpitch;
vmCvar_t cg_bobroll;
vmCvar_t cg_shadows;
vmCvar_t cg_renderToTextureFX;
vmCvar_t cg_shadowCullDistance;
vmCvar_t cg_footsteps;
vmCvar_t cg_saberEntMarks;
vmCvar_t cg_paused;
vmCvar_t cg_drawTimer;
vmCvar_t cg_drawFPS;
vmCvar_t cg_drawSnapshot;
vmCvar_t cg_drawAmmoWarning;
vmCvar_t cg_drawCrosshair;
vmCvar_t cg_crosshairIdentifyTarget;
vmCvar_t cg_dynamicCrosshair;
vmCvar_t cg_drawCrosshairNames;
vmCvar_t cg_DrawCrosshairItem;
vmCvar_t cg_crosshairForceHint;
vmCvar_t cg_crosshairX;
vmCvar_t cg_crosshairY;
vmCvar_t cg_crosshairSize;
vmCvar_t cg_crosshairDualSize;
vmCvar_t cg_weaponcrosshairs;
vmCvar_t cg_draw2D;
vmCvar_t cg_drawStatus;
vmCvar_t cg_drawHUD;
vmCvar_t cg_debugAnim;
#ifndef FINAL_BUILD
vmCvar_t	cg_debugAnimTarget;
vmCvar_t	cg_gun_frame;
#endif
vmCvar_t cg_gun_x;
vmCvar_t cg_gun_y;
vmCvar_t cg_gun_z;
vmCvar_t cg_debugSaber;
vmCvar_t cg_debugEvents;
vmCvar_t cg_errorDecay;
vmCvar_t cg_addMarks;
vmCvar_t cg_drawGun;
vmCvar_t cg_autoswitch;
vmCvar_t cg_simpleItems;
vmCvar_t cg_fov;
vmCvar_t cg_fovAspectAdjust;
vmCvar_t cg_endcredits;
vmCvar_t cg_updatedDataPadForcePower1;
vmCvar_t cg_updatedDataPadForcePower2;
vmCvar_t cg_updatedDataPadForcePower3;
vmCvar_t cg_updatedDataPadObjective;
vmCvar_t cg_drawBreath;
vmCvar_t cg_roffdebug;
#ifndef FINAL_BUILD
vmCvar_t	cg_roffval1;
vmCvar_t	cg_roffval2;
vmCvar_t	cg_roffval3;
vmCvar_t	cg_roffval4;
#endif
vmCvar_t cg_thirdPerson;
vmCvar_t cg_thirdPersonRange;
vmCvar_t cg_thirdPersonMaxRange;
vmCvar_t cg_thirdPersonAngle;
vmCvar_t cg_thirdPersonPitchOffset;
vmCvar_t cg_thirdPersonVertOffset;
vmCvar_t cg_thirdPersonCameraDamp;
vmCvar_t cg_thirdPersonTargetDamp;
vmCvar_t cg_gunAutoFirst;

vmCvar_t cg_thirdPersonAlpha;
vmCvar_t cg_thirdPersonAutoAlpha;
vmCvar_t cg_thirdPersonHorzOffset;

vmCvar_t cg_stereoSeparation;
vmCvar_t cg_developer;
vmCvar_t cg_timescale;
vmCvar_t cg_skippingcin;

vmCvar_t cg_pano;
vmCvar_t cg_panoNumShots;

vmCvar_t fx_freeze;
vmCvar_t fx_debug;

vmCvar_t cg_missionInfoFlashTime;
vmCvar_t cg_hudFiles;

vmCvar_t cg_neverHearThatDumbBeepingSoundAgain;

vmCvar_t cg_VariantSoundCap; // 0 = no capping, else cap to (n) max (typically just 1, but allows more)
vmCvar_t cg_turnAnims;
vmCvar_t cg_motionBoneComp;
vmCvar_t cg_distributeMBCorrection;
vmCvar_t cg_reliableAnimEvents;

vmCvar_t cg_smoothPlayerPos;
vmCvar_t cg_smoothPlayerPlat;
vmCvar_t cg_smoothPlayerPlatAccel;
vmCvar_t cg_g2Marks;
vmCvar_t fx_expensivePhysics;
vmCvar_t cg_debugHealthBars;
vmCvar_t cg_debugBlockBars;
vmCvar_t cg_debugFatigueBars;
vmCvar_t cg_saberinfo;

vmCvar_t cg_ignitionSpeed;
vmCvar_t cg_ignitionSpeedstaff;

vmCvar_t cg_smoothCamera;
vmCvar_t cg_speedTrail;
vmCvar_t cg_fovViewmodel;
vmCvar_t cg_fovViewmodelAdjust;

vmCvar_t cg_scaleVehicleSensitivity;

vmCvar_t cg_SFXSabers;
vmCvar_t cg_SFXSabersGlowSize;
vmCvar_t cg_SFXSabersCoreSize;
vmCvar_t cg_SFXLerpOrigin;
//
vmCvar_t cg_SFXSabersGlowSizeEP1;
vmCvar_t cg_SFXSabersCoreSizeEP1;
vmCvar_t cg_SFXSabersGlowSizeEP2;
vmCvar_t cg_SFXSabersCoreSizeEP2;
vmCvar_t cg_SFXSabersGlowSizeEP3;
vmCvar_t cg_SFXSabersCoreSizeEP3;
vmCvar_t cg_SFXSabersGlowSizeSFX;
vmCvar_t cg_SFXSabersCoreSizeSFX;
vmCvar_t cg_SFXSabersGlowSizeOT;
vmCvar_t cg_SFXSabersCoreSizeOT;
vmCvar_t cg_SFXSabersGlowSizeROTJ;
vmCvar_t cg_SFXSabersCoreSizeROTJ;
vmCvar_t cg_SFXSabersGlowSizeTFA;
vmCvar_t cg_SFXSabersCoreSizeTFA;
vmCvar_t cg_SFXSabersGlowSizeUSB;
vmCvar_t cg_SFXSabersCoreSizeUSB;
//
vmCvar_t cg_SFXSabersGlowSizeRebels;
vmCvar_t cg_SFXSabersCoreSizeRebels;

vmCvar_t cg_SerenityJediEngineMode;
vmCvar_t cg_SerenityJediEngineHudMode;
vmCvar_t cg_SaberInnonblockableAttackWarning;
vmCvar_t cg_IsSaberDoingAttackDamage;
vmCvar_t cg_DebugSaberCombat;

vmCvar_t cg_drawRadar;

vmCvar_t cg_drawSelectionScrollBar;

vmCvar_t cg_trueguns;
vmCvar_t cg_fpls;

vmCvar_t cg_trueroll;
vmCvar_t cg_trueflip;
vmCvar_t cg_truespin;
vmCvar_t cg_truemoveroll;
vmCvar_t cg_truesaberonly;
vmCvar_t cg_trueeyeposition;
vmCvar_t cg_trueinvertsaber;
vmCvar_t cg_truefov;
vmCvar_t cg_truebobbing;

vmCvar_t cg_weaponBob;
vmCvar_t cg_fallingBob;

vmCvar_t cg_gunMomentumDamp;
vmCvar_t cg_gunMomentumFall;
vmCvar_t cg_gunMomentumEnable;
vmCvar_t cg_gunMomentumInterval;

vmCvar_t cg_setVaderBreath;
vmCvar_t cg_setVaderBreathdamaged;
vmCvar_t cg_com_outcast;
vmCvar_t g_update6firststartup;

vmCvar_t g_totgfirststartup;

vmCvar_t cg_drawwidescreenmode;

vmCvar_t cg_SpinningBarrels;

vmCvar_t cg_Bloodmist;

vmCvar_t cg_Weather;

vmCvar_t cg_jkoeffects;

vmCvar_t cg_hudRatio;

vmCvar_t cg_saberLockCinematicCamera;

vmCvar_t cg_allowcallout;

vmCvar_t cg_allowcalloutmarker;

vmCvar_t cg_jumpSounds;

vmCvar_t cg_rollSounds;

using cvarTable_t = struct
{
	vmCvar_t* vmCvar;
	const char* cvarName;
	const char* defaultString;
	int cvarFlags;
};

static cvarTable_t cvarTable[] = {
	{&cg_autoswitch, "cg_autoswitch", "1", CVAR_ARCHIVE},
	{&cg_drawGun, "cg_drawGun", "1", CVAR_ARCHIVE},
	{&cg_fov, "cg_fov", "80", CVAR_ARCHIVE},
	{&cg_fovAspectAdjust, "cg_fovAspectAdjust", "0", CVAR_ARCHIVE},
	{&cg_stereoSeparation, "cg_stereoSeparation", "0.4", CVAR_ARCHIVE},
	{&cg_shadows, "cg_shadows", "3", CVAR_ARCHIVE},
	{&cg_renderToTextureFX, "cg_renderToTextureFX", "1", CVAR_ARCHIVE},
	{&cg_shadowCullDistance, "r_shadowRange", "1000", CVAR_ARCHIVE},
	{&cg_footsteps, "cg_footsteps", "3", CVAR_ARCHIVE},
	//1 = sounds, 2 = sounds & effects, 3 = sounds, effects & marks, 4 = always
	{&cg_saberEntMarks, "cg_saberEntMarks", "1", CVAR_ARCHIVE},

	{&cg_draw2D, "cg_draw2D", "1", CVAR_ARCHIVE},
	{&cg_drawStatus, "cg_drawStatus", "1", CVAR_ARCHIVE},
	{&cg_drawHUD, "cg_drawHUD", "1", 0},
	{&cg_drawTimer, "cg_drawTimer", "0", CVAR_ARCHIVE},
	{&cg_drawFPS, "cg_drawFPS", "0", CVAR_ARCHIVE},
	{&cg_drawSnapshot, "cg_drawSnapshot", "0", CVAR_ARCHIVE},
	{&cg_drawAmmoWarning, "cg_drawAmmoWarning", "1", CVAR_ARCHIVE},
	{&cg_drawCrosshair, "cg_drawCrosshair", "1", CVAR_ARCHIVE},
	{&cg_dynamicCrosshair, "cg_dynamicCrosshair", "6", CVAR_ARCHIVE},
	{&cg_drawCrosshairNames, "cg_drawCrosshairNames", "0", CVAR_ARCHIVE},
	{&cg_DrawCrosshairItem, "cg_DrawCrosshairItem", "1", CVAR_ARCHIVE},
	// NOTE : I also create this in UI_Init()
	{&cg_crosshairIdentifyTarget, "cg_crosshairIdentifyTarget", "1", CVAR_ARCHIVE},
	{&cg_crosshairForceHint, "cg_crosshairForceHint", "1", CVAR_ARCHIVE | CVAR_SAVEGAME | CVAR_NORESTART},
	{&cg_endcredits, "cg_endcredits", "0", 0},
	{&cg_updatedDataPadForcePower1, "cg_updatedDataPadForcePower1", "0", 0},
	{&cg_updatedDataPadForcePower2, "cg_updatedDataPadForcePower2", "0", 0},
	{&cg_updatedDataPadForcePower3, "cg_updatedDataPadForcePower3", "0", 0},
	{&cg_updatedDataPadObjective, "cg_updatedDataPadObjective", "0", 0},

	{&cg_crosshairSize, "cg_crosshairSize", "20", CVAR_ARCHIVE},
	{&cg_crosshairDualSize, "cg_crosshairDualSize", "10", CVAR_ARCHIVE},
	{&cg_weaponcrosshairs, "cg_weaponcrosshairs", "0", CVAR_ARCHIVE},

	{&cg_crosshairX, "cg_crosshairX", "0", CVAR_ARCHIVE},
	{&cg_crosshairY, "cg_crosshairY", "0", CVAR_ARCHIVE},
	{&cg_simpleItems, "cg_simpleItems", "0", CVAR_ARCHIVE},
	// NOTE : I also create this in UI_Init()
	{&cg_addMarks, "cg_marks", "1", CVAR_ARCHIVE},
	// NOTE : I also create these weapon sway cvars in UI_Init()
	{&cg_runpitch, "cg_runpitch", "0.002", CVAR_ARCHIVE},
	{&cg_runroll, "cg_runroll", "0.005", CVAR_ARCHIVE},
	{&cg_bobup, "cg_bobup", "0.005", CVAR_ARCHIVE},
	{&cg_bobpitch, "cg_bobpitch", "0.002", CVAR_ARCHIVE},
	{&cg_bobroll, "cg_bobroll", "0.002", CVAR_ARCHIVE},

	{&cg_debugAnim, "cg_debuganim", "0", CVAR_CHEAT},
#ifndef FINAL_BUILD
	{ &cg_gun_frame, "gun_frame", "0", CVAR_CHEAT },
	{ &cg_debugAnimTarget, "cg_debugAnimTarget", "0", CVAR_CHEAT },
#endif
	{&cg_gun_x, "cg_gunX", "0", CVAR_ARCHIVE},
	{&cg_gun_y, "cg_gunY", "0", CVAR_ARCHIVE},
	{&cg_gun_z, "cg_gunZ", "0", CVAR_ARCHIVE},
	{&cg_debugSaber, "cg_debugsaber", "0", CVAR_CHEAT},
	{&cg_debugEvents, "cg_debugevents", "0", CVAR_CHEAT},
	{&cg_errorDecay, "cg_errordecay", "100", 0},

	{&cg_drawBreath, "cg_drawBreath", "0", CVAR_ARCHIVE}, // Added 11/07/02
	{&cg_roffdebug, "cg_roffdebug", "0"},
#ifndef FINAL_BUILD
	{ &cg_roffval1, "cg_roffval1", "0" },
	{ &cg_roffval2, "cg_roffval2", "0" },
	{ &cg_roffval3, "cg_roffval3", "0" },
	{ &cg_roffval4, "cg_roffval4", "0" },
#endif
	{&cg_thirdPerson, "cg_thirdPerson", "1", CVAR_SAVEGAME},
	{&cg_thirdPersonRange, "cg_thirdPersonRange", "80", CVAR_ARCHIVE},
	{&cg_thirdPersonMaxRange, "cg_thirdPersonMaxRange", "150", 0},
	{&cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", 0},
	{&cg_thirdPersonPitchOffset, "cg_thirdPersonPitchOffset", "0", 0},
	{&cg_thirdPersonVertOffset, "cg_thirdPersonVertOffset", "16", 0},
	{&cg_thirdPersonCameraDamp, "cg_thirdPersonCameraDamp", "0.3", 0},
	{&cg_thirdPersonTargetDamp, "cg_thirdPersonTargetDamp", "0.5", 0},

	{&cg_thirdPersonHorzOffset, "cg_thirdPersonHorzOffset", "0", 0},
	{&cg_thirdPersonAlpha, "cg_thirdPersonAlpha", "1.0", CVAR_ARCHIVE},
	{&cg_thirdPersonAutoAlpha, "cg_thirdPersonAutoAlpha", "0", 0},
	// NOTE: also declare this in UI_Init
	{&cg_gunAutoFirst, "cg_gunAutoFirst", "1", CVAR_ARCHIVE},

	{&cg_pano, "pano", "0", 0},
	{&cg_panoNumShots, "panoNumShots", "10", 0},

	{&fx_freeze, "fx_freeze", "0", 0},
	{&fx_debug, "fx_debug", "0", 0},
	// the following variables are created in other parts of the system,
	// but we also reference them here

	{&cg_paused, "cl_paused", "0", CVAR_ROM},
	{&cg_developer, "developer", "", 0},
	{&cg_timescale, "timescale", "1", 0},
	{&cg_skippingcin, "skippingCinematic", "0", CVAR_ROM},
	{&cg_missionInfoFlashTime, "cg_missionInfoFlashTime", "10000", 0},
	{&cg_hudFiles, "cg_hudFiles", "ui/jahud.txt", CVAR_ARCHIVE},

	{&cg_VariantSoundCap, "cg_VariantSoundCap", "0", 0},
	{&cg_turnAnims, "cg_turnAnims", "0", 0},
	{&cg_motionBoneComp, "cg_motionBoneComp", "2", 0},
	{&cg_distributeMBCorrection, "cg_distributeMBCorrection", "1", 0},
	{&cg_reliableAnimEvents, "cg_reliableAnimEvents", "1", CVAR_ARCHIVE},
	{&cg_smoothPlayerPos, "cg_smoothPlayerPos", "0.5", 0},
	{&cg_smoothPlayerPlat, "cg_smoothPlayerPlat", "0.75", 0},
	{&cg_smoothPlayerPlatAccel, "cg_smoothPlayerPlatAccel", "3.25", 0},
	{&cg_g2Marks, "cg_g2Marks", "1", CVAR_ARCHIVE},
	{&fx_expensivePhysics, "fx_expensivePhysics", "1", CVAR_ARCHIVE},
	{&cg_debugHealthBars, "cg_debugHealthBars", "0", CVAR_ARCHIVE},
	{&cg_debugBlockBars, "cg_drawblockpointbar", "0", CVAR_ARCHIVE},
	{&cg_debugFatigueBars, "cg_drawfatiguepointbar", "0", CVAR_ARCHIVE},
	{&cg_saberinfo, "d_saberinfo", "0", CVAR_ARCHIVE},

	{&cg_smoothCamera, "cg_smoothCamera", "1", CVAR_ARCHIVE},
	{&cg_speedTrail, "cg_speedTrail", "1", CVAR_ARCHIVE},
	{&cg_fovViewmodel, "cg_fovViewmodel", "0", CVAR_ARCHIVE},
	{&cg_fovViewmodelAdjust, "cg_fovViewmodelAdjust", "1", CVAR_ARCHIVE},

	{&cg_scaleVehicleSensitivity, "cg_scaleVehicleSensitivity", "1", CVAR_ARCHIVE},

	{&cg_SFXSabers, "cg_SFXSabers", "4", CVAR_ARCHIVE},
	{&cg_SFXSabersGlowSize, "cg_SFXSabersGlowSize", "1.0", CVAR_ARCHIVE},
	{&cg_SFXSabersCoreSize, "cg_SFXSabersCoreSize", "1.0", CVAR_ARCHIVE},
	{&cg_SFXLerpOrigin, "cg_SFXLerpOrigin", "0", CVAR_ARCHIVE},

	{&cg_SFXSabersGlowSizeEP1, "cg_SFXSabersGlowSizeEP1", "1.0", CVAR_ARCHIVE},
	{&cg_SFXSabersCoreSizeEP1, "cg_SFXSabersCoreSizeEP1", "1.0", CVAR_ARCHIVE},
	{&cg_SFXSabersGlowSizeEP2, "cg_SFXSabersGlowSizeEP2", "1.0", CVAR_ARCHIVE},
	{&cg_SFXSabersCoreSizeEP2, "cg_SFXSabersCoreSizeEP2", "1.0", CVAR_ARCHIVE},
	{&cg_SFXSabersGlowSizeEP3, "cg_SFXSabersGlowSizeEP3", "1.0", CVAR_ARCHIVE},
	{&cg_SFXSabersCoreSizeEP3, "cg_SFXSabersCoreSizeEP3", "1.0", CVAR_ARCHIVE},
	{&cg_SFXSabersGlowSizeSFX, "cg_SFXSabersGlowSizeSFX", "1.0", CVAR_ARCHIVE},
	{&cg_SFXSabersCoreSizeSFX, "cg_SFXSabersCoreSizeSFX", "1.0", CVAR_ARCHIVE},
	{&cg_SFXSabersGlowSizeOT, "cg_SFXSabersGlowSizeOT", "1.0", CVAR_ARCHIVE},
	{&cg_SFXSabersCoreSizeOT, "cg_SFXSabersCoreSizeOT", "1.0", CVAR_ARCHIVE},
	{&cg_SFXSabersGlowSizeROTJ, "cg_SFXSabersGlowSizeROTJ", "1.0", CVAR_ARCHIVE},
	{&cg_SFXSabersCoreSizeROTJ, "cg_SFXSabersCoreSizeROTJ", "1.0", CVAR_ARCHIVE},
	{&cg_SFXSabersGlowSizeTFA, "cg_SFXSabersGlowSizeTFA", "1.0", CVAR_ARCHIVE},
	{&cg_SFXSabersCoreSizeTFA, "cg_SFXSabersCoreSizeTFA", "1.0", CVAR_ARCHIVE},
	{&cg_SFXSabersGlowSizeUSB, "cg_SFXSabersGlowSizeUSB", "1.0", CVAR_ARCHIVE},
	{&cg_SFXSabersCoreSizeUSB, "cg_SFXSabersCoreSizeUSB", "1.0", CVAR_ARCHIVE},
	//

	{&cg_SFXSabersGlowSizeRebels, "cg_SFXSabersGlowSizeRebels", "0.6", CVAR_ARCHIVE},
	{&cg_SFXSabersCoreSizeRebels, "cg_SFXSabersCoreSizeRebels", "0.6", CVAR_ARCHIVE},

	{&cg_ignitionSpeed, "cg_ignitionSpeed", "1.0", CVAR_ARCHIVE},
	{&cg_ignitionSpeedstaff, "cg_ignitionSpeedstaff", "1.0", CVAR_ARCHIVE},

	{&cg_SerenityJediEngineMode, "g_SerenityJediEngineMode", "1", CVAR_ARCHIVE},
	{&cg_SerenityJediEngineHudMode, "g_SerenityJediEngineHudMode", "4", CVAR_ARCHIVE},
	{&cg_SaberInnonblockableAttackWarning, "g_SaberInnonblockableAttackWarning", "0", CVAR_ARCHIVE},
	{&cg_IsSaberDoingAttackDamage, "g_IsSaberDoingAttackDamage", "0", CVAR_ARCHIVE},

	{&cg_drawRadar, "cg_drawRadar", "1", CVAR_ARCHIVE},

	{&cg_drawSelectionScrollBar, "cg_drawSelectionScrollBar", "0", CVAR_ARCHIVE},

	{&cg_trueguns, "cg_trueguns", "1", CVAR_ARCHIVE},
	{&cg_fpls, "cg_fpls", "0", CVAR_ARCHIVE},
	{&cg_trueroll, "cg_trueroll", "1", CVAR_ARCHIVE},
	{&cg_trueflip, "cg_trueflip", "1", CVAR_ARCHIVE},
	{&cg_truespin, "cg_truespin", "1", CVAR_ARCHIVE},
	{&cg_truemoveroll, "cg_truemoveroll", "0", CVAR_ARCHIVE},
	{&cg_truesaberonly, "cg_truesaberonly", "0", CVAR_ARCHIVE},
	{&cg_trueeyeposition, "cg_trueeyeposition", "0.0", 0},
	{&cg_trueinvertsaber, "cg_trueinvertsaber", "1", CVAR_ARCHIVE},
	{&cg_truefov, "cg_truefov", "80", CVAR_ARCHIVE},
	{&cg_truebobbing, "cg_truebobbing", "1", CVAR_ARCHIVE},

	{&cg_weaponBob, "cg_weaponBob", "1", CVAR_ARCHIVE},
	{&cg_fallingBob, "cg_fallingBob", "1", CVAR_ARCHIVE},

	{&cg_gunMomentumDamp, "cg_gunMomentumDamp", "0.001", CVAR_ARCHIVE},
	{&cg_gunMomentumFall, "cg_gunMomentumFall", "0.5", CVAR_ARCHIVE},
	{&cg_gunMomentumEnable, "cg_gunMomentumEnable", "0", CVAR_ARCHIVE},
	{&cg_gunMomentumInterval, "cg_gunMomentumInterval", "75", CVAR_ARCHIVE},

	{&cg_setVaderBreath, "cg_setVaderBreath", "0", CVAR_ARCHIVE | CVAR_SAVEGAME | CVAR_NORESTART},
	{&cg_setVaderBreathdamaged, "cg_setVaderBreathdamaged", "0", CVAR_ARCHIVE | CVAR_SAVEGAME | CVAR_NORESTART},
	{&cg_com_outcast, "com_outcast", "0", CVAR_ARCHIVE | CVAR_SAVEGAME | CVAR_NORESTART},
	{&g_update6firststartup, "g_update6firststartup", "1", 0},

	{&g_totgfirststartup, "g_totgfirststartup", "1", 0},

	{&cg_drawwidescreenmode, "cg_drawwidescreenmode", "0", CVAR_ARCHIVE | CVAR_SAVEGAME | CVAR_NORESTART},

	{&cg_SpinningBarrels, "cg_SpinningBarrels", "0", CVAR_ARCHIVE},

	{&cg_Bloodmist, "g_Bloodmist", "1", CVAR_ARCHIVE},

	{&cg_Weather, "r_weather", "0", CVAR_ARCHIVE},

	{&cg_jkoeffects, "cg_outcastpusheffect", "1", CVAR_ARCHIVE},

	{&r_ratiofix, "r_ratiofix", "0", CVAR_ARCHIVE | CVAR_SAVEGAME | CVAR_NORESTART},
	{&cg_hudRatio, "cg_hudRatio", "1", CVAR_ARCHIVE | CVAR_SAVEGAME | CVAR_NORESTART},
	{&cg_DebugSaberCombat, "g_DebugSaberCombat", "0", CVAR_ARCHIVE},

	{&cg_allowcallout, "g_allowattackorder", "1", CVAR_ARCHIVE},

	{&cg_allowcalloutmarker, "g_allowattackordermarker", "1", CVAR_ARCHIVE},

	{&cg_com_kotor, "com_kotor", "0", CVAR_ARCHIVE | CVAR_SAVEGAME | CVAR_NORESTART},

	{&cg_saberLockCinematicCamera, "g_saberLockCinematicCamera", "0", CVAR_ARCHIVE},

	{&cg_jumpSounds, "cg_jumpSounds", "1", CVAR_ARCHIVE | CVAR_SAVEGAME | CVAR_NORESTART},

	{&cg_rollSounds, "cg_rollSounds", "2", CVAR_ARCHIVE | CVAR_SAVEGAME | CVAR_NORESTART},
};

static constexpr size_t cvarTableSize = std::size(cvarTable);

/*
=================
CG_RegisterCvars
=================
*/
void CG_RegisterCvars()
{
	size_t i;
	cvarTable_t* cv;

	for (i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++)
	{
		cgi_Cvar_Register(cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags);
		if (cv->vmCvar == &r_ratiofix)
		{
			CG_Set2DRatio();
		}
	}
}

/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars()
{
	size_t i;
	cvarTable_t* cv;

	for (i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++)
	{
		if (cv->vmCvar)
		{
			const int modCount = cv->vmCvar->modificationCount;
			cgi_Cvar_Update(cv->vmCvar);
			if (cv->vmCvar->modificationCount > modCount && cv->vmCvar == &r_ratiofix)
			{
				CG_Set2DRatio();
			}
		}
	}
}

int CG_CrosshairPlayer()
{
	if (cg.time > cg.crosshairClientTime + 1000)
	{
		return -1;
	}
	return cg.crosshairclient_num;
}

int CG_GetCameraPos(vec3_t camerapos)
{
	if (in_camera)
	{
		VectorCopy(client_camera.origin, camerapos);
		return 1;
	}
	if (cg_entities[0].gent && cg_entities[0].gent->client && cg_entities[0].gent->client->ps.viewEntity > 0 &&
		cg_entities[0].gent->client->ps.viewEntity < ENTITYNUM_WORLD)
		//else if ( cg.snap && cg.snap->ps.viewEntity > 0 && cg.snap->ps.viewEntity < ENTITYNUM_WORLD )
	{
		//in an entity camera view
		if (g_entities[cg_entities[0].gent->client->ps.viewEntity].client && cg.renderingThirdPerson)
		{
			VectorCopy(g_entities[cg_entities[0].gent->client->ps.viewEntity].client->renderInfo.eyePoint,
				camerapos);
		}
		else
		{
			VectorCopy(g_entities[cg_entities[0].gent->client->ps.viewEntity].currentOrigin, camerapos);
		}
		//VectorCopy( cg_entities[cg_entities[0].gent->client->ps.viewEntity].lerpOrigin, camerapos );
		/*
		if ( g_entities[cg.snap->ps.viewEntity].client && cg.renderingThirdPerson )
		{
			VectorCopy( g_entities[cg.snap->ps.viewEntity].client->renderInfo.eyePoint, camerapos );
		}
		else
		{//use the g_ent because it may not have gotten over to the client yet...
			VectorCopy( g_entities[cg.snap->ps.viewEntity].currentOrigin, camerapos );
		}
		*/
		return 1;
	}
	if (cg.renderingThirdPerson)
	{
		//in third person
		//FIXME: what about hacks that render in third person regardless of this value?
		VectorCopy(cg.refdef.vieworg, camerapos);
		return 1;
	}
	if (cg.snap && (cg.snap->ps.weapon == WP_SABER || cg.snap->ps.weapon == WP_MELEE))
		//implied: !cg.renderingThirdPerson
	{
		//first person saber hack
		VectorCopy(cg.refdef.vieworg, camerapos);
		return 1;
	}
	if (cg_trueguns.integer && !cg.zoomMode)
	{
		VectorCopy(cg.refdef.vieworg, camerapos);
		return 1;
	}
	return 0;
}

int CG_GetCameraAng(vec3_t cameraang)
{
	if (in_camera)
	{
		VectorCopy(client_camera.angles, cameraang);
		return 1;
	}
	VectorCopy(cg.refdefViewAngles, cameraang);
	return 1;
}

void CG_Printf(const char* msg, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof text, msg, argptr);
	va_end(argptr);

	cgi_Printf(text);
}

NORETURN void CG_Error(const char* msg, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof text, msg, argptr);
	va_end(argptr);

	cgi_Error(text);
}

/*
================
CG_Argv
================
*/
const char* CG_Argv(const int arg)
{
	static char buffer[MAX_STRING_CHARS];

	cgi_Argv(arg, buffer, sizeof buffer);

	return buffer;
}

//========================================================================

/*
=================
CG_RegisterItemSounds

The server says this item is used on this level
=================
*/
void CG_RegisterItemSounds(const int itemNum)
{
	char data[MAX_QPATH];

	const gitem_t* item = &bg_itemlist[itemNum];

	if (item->pickup_sound)
	{
		cgi_S_RegisterSound(item->pickup_sound);
	}

	// parse the space seperated precache string for other media
	const char* s = item->sounds;
	if (!s || !s[0])
		return;

	while (*s)
	{
		const char* start = s;
		while (*s && *s != ' ')
		{
			s++;
		}

		const int len = s - start;
		if (len >= MAX_QPATH || len < 5)
		{
			CG_Error("PrecacheItem: %s has bad precache string",
				item->classname);
		}
		memcpy(data, start, len);
		data[len] = 0;
		if (*s)
		{
			s++;
		}

		if (strcmp(data + len - 3, "wav") == 0)
		{
			cgi_S_RegisterSound(data);
		}
	}
}

/*
======================
CG_LoadingString

======================
*/
void CG_LoadingString(const char* s)
{
	Q_strncpyz(cg.infoScreenText, s, sizeof cg.infoScreenText);
	cgi_UpdateScreen();
}

static void CG_AS_Register()
{
	CG_LoadingString("ambient sound sets");

	assert(as_preCacheMap);

	//Load the ambient sets

	cgi_AS_AddPrecacheEntry("#clear"); // ;-)
	//FIXME: Don't ask... I had to get around a really nasty MS error in the templates with this...
	namePrecache_m::iterator pi;
	STL_ITERATE(pi, (*as_preCacheMap))
	{
		cgi_AS_AddPrecacheEntry((*pi).first.c_str());
	}

	cgi_AS_ParseSets();
}

/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
static void CG_RegisterSounds()
{
	int i;

	CG_AS_Register();

	CG_LoadingString("general sounds");

	//FIXME: add to cg.media?
	cgi_S_RegisterSound("sound/player/fallsplat.wav");

	cgs.media.selectSound = cgi_S_RegisterSound("sound/weapons/change.wav");
	cgs.media.selectSound2 = cgi_S_RegisterSound("sound/interface/sub_select.wav");
	//	cgs.media.useNothingSound = cgi_S_RegisterSound( "sound/items/use_nothing.wav" );

	cgs.media.noAmmoSound = cgi_S_RegisterSound("sound/weapons/noammo.wav");
	//	cgs.media.talkSound = 	cgi_S_RegisterSound( "sound/interface/communicator.wav" );
	cgs.media.landSound = cgi_S_RegisterSound("sound/player/land1.wav");
	cgs.media.rollSound = cgi_S_RegisterSound("sound/player/roll1.wav");

	cgs.media.teleInSound = cgi_S_RegisterSound("sound/player/telein.wav");
	cgs.media.teleOutSound = cgi_S_RegisterSound("sound/player/teleout.wav");
	cgs.media.respawnSound = cgi_S_RegisterSound("sound/items/respawn1.wav");

	theFxScheduler.RegisterEffect("env/slide_dust");

	cgs.media.overchargeFastSound = cgi_S_RegisterSound("sound/weapons/overchargeFast.wav");
	cgs.media.overchargeSlowSound = cgi_S_RegisterSound("sound/weapons/overchargeSlow.wav");
	cgs.media.overchargeLoopSound = cgi_S_RegisterSound("sound/weapons/overchargeLoop.wav");
	cgs.media.overchargeEndSound = cgi_S_RegisterSound("sound/weapons/overchargeEnd.wav");

	cgs.media.batteryChargeSound = cgi_S_RegisterSound("sound/interface/pickup_battery.wav");

	cgs.media.messageLitSound = cgi_S_RegisterSound("sound/interface/update");

	cgs.media.overload = cgi_S_RegisterSound("sound/effects/woosh1");
	cgs.media.overheatgun = cgi_S_RegisterSound("sound/effects/air_burst");
	cgs.media.noforceSound = cgi_S_RegisterSound("sound/weapons/force/noforce");

	cgs.media.watrInSound = cgi_S_RegisterSound("sound/player/watr_in.wav");
	cgs.media.watrOutSound = cgi_S_RegisterSound("sound/player/watr_out.wav");
	cgs.media.watrUnSound = cgi_S_RegisterSound("sound/player/watr_un.wav");

	if (gi.totalMapContents() & CONTENTS_LAVA)
	{
		cgs.media.lavaInSound = cgi_S_RegisterSound("sound/player/inlava.wav");
		cgs.media.lavaOutSound = cgi_S_RegisterSound("sound/player/watr_out.wav");
		cgs.media.lavaUnSound = cgi_S_RegisterSound("sound/player/muckexit.wav");
	}
	// Zoom
	cgs.media.zoomStart = cgi_S_RegisterSound("sound/interface/zoomstart.wav");
	cgs.media.zoomLoop = cgi_S_RegisterSound("sound/interface/zoomloop.wav");
	cgs.media.zoomEnd = cgi_S_RegisterSound("sound/interface/zoomend.wav");

	cgi_S_RegisterSound("sound/chars/turret/startup.wav");
	cgi_S_RegisterSound("sound/chars/turret/shutdown.wav");
	cgi_S_RegisterSound("sound/chars/turret/ping.wav");
	cgi_S_RegisterSound("sound/chars/turret/move.wav");
	cgi_S_RegisterSound("sound/player/use_sentry");
	cgi_R_RegisterModel("models/items/psgun.glm");
	theFxScheduler.RegisterEffect("turret/explode");
	theFxScheduler.RegisterEffect("sparks/spark_exp_nosnd");

	for (i = 0; i < 4; i++)
	{
		char name[MAX_QPATH];
		Com_sprintf(name, sizeof name, "sound/player/footsteps/stone_step%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_STONEWALK][i] = cgi_S_RegisterSound(name);
		Com_sprintf(name, sizeof name, "sound/player/footsteps/stone_run%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_STONERUN][i] = cgi_S_RegisterSound(name);

		Com_sprintf(name, sizeof name, "sound/player/footsteps/metal_step%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_METALWALK][i] = cgi_S_RegisterSound(name);
		Com_sprintf(name, sizeof name, "sound/player/footsteps/metal_run%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_METALRUN][i] = cgi_S_RegisterSound(name);

		Com_sprintf(name, sizeof name, "sound/player/footsteps/pipe_step%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_PIPEWALK][i] = cgi_S_RegisterSound(name);
		Com_sprintf(name, sizeof name, "sound/player/footsteps/pipe_run%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_PIPERUN][i] = cgi_S_RegisterSound(name);

		Com_sprintf(name, sizeof name, "sound/player/footsteps/water_run%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_SPLASH][i] = cgi_S_RegisterSound(name);

		Com_sprintf(name, sizeof name, "sound/player/footsteps/water_walk%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_WADE][i] = cgi_S_RegisterSound(name);

		Com_sprintf(name, sizeof name, "sound/player/footsteps/water_wade_0%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_SWIM][i] = cgi_S_RegisterSound(name);

		Com_sprintf(name, sizeof name, "sound/player/footsteps/snow_step%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_SNOWWALK][i] = cgi_S_RegisterSound(name);
		Com_sprintf(name, sizeof name, "sound/player/footsteps/snow_run%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_SNOWRUN][i] = cgi_S_RegisterSound(name);

		Com_sprintf(name, sizeof name, "sound/player/footsteps/sand_walk%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_SANDWALK][i] = cgi_S_RegisterSound(name);
		Com_sprintf(name, sizeof name, "sound/player/footsteps/sand_run%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_SANDRUN][i] = cgi_S_RegisterSound(name);

		Com_sprintf(name, sizeof name, "sound/player/footsteps/grass_step%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_GRASSWALK][i] = cgi_S_RegisterSound(name);
		Com_sprintf(name, sizeof name, "sound/player/footsteps/grass_run%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_GRASSRUN][i] = cgi_S_RegisterSound(name);

		Com_sprintf(name, sizeof name, "sound/player/footsteps/dirt_step%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_DIRTWALK][i] = cgi_S_RegisterSound(name);
		Com_sprintf(name, sizeof name, "sound/player/footsteps/dirt_run%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_DIRTRUN][i] = cgi_S_RegisterSound(name);

		Com_sprintf(name, sizeof name, "sound/player/footsteps/mud_walk%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_MUDWALK][i] = cgi_S_RegisterSound(name);
		Com_sprintf(name, sizeof name, "sound/player/footsteps/mud_run%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_MUDRUN][i] = cgi_S_RegisterSound(name);

		Com_sprintf(name, sizeof name, "sound/player/footsteps/gravel_walk%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_GRAVELWALK][i] = cgi_S_RegisterSound(name);
		Com_sprintf(name, sizeof name, "sound/player/footsteps/gravel_run%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_GRAVELRUN][i] = cgi_S_RegisterSound(name);

		Com_sprintf(name, sizeof name, "sound/player/footsteps/rug_step%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_RUGWALK][i] = cgi_S_RegisterSound(name);
		Com_sprintf(name, sizeof name, "sound/player/footsteps/rug_run%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_RUGRUN][i] = cgi_S_RegisterSound(name);

		Com_sprintf(name, sizeof name, "sound/player/footsteps/wood_walk%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_WOODWALK][i] = cgi_S_RegisterSound(name);
		Com_sprintf(name, sizeof name, "sound/player/footsteps/wood_run%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_WOODRUN][i] = cgi_S_RegisterSound(name);
	}

	cg.loadLCARSStage = 1;
	CG_LoadingString("item sounds");

	// only register the items that the server says we need
	char items[MAX_ITEMS + 1];
	//Raz: Fixed buffer overflow
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof items);

	for (i = 1; i < bg_numItems; i++)
	{
		if (items[i] == '1') //even with sound pooling, don't clutter it for low end machines
		{
			CG_RegisterItemSounds(i);
		}
	}

	cg.loadLCARSStage = 2;
	CG_LoadingString("preregistered sounds");

	for (i = 1; i < MAX_SOUNDS; i++)
	{
		const char* soundName = CG_ConfigString(CS_SOUNDS + i);
		if (!soundName[0])
		{
			break;
		}
		if (soundName[0] == '*')
		{
			continue; // custom sound
		}
		if (!(i & 7))
		{
			CG_LoadingString(soundName);
		}
		cgs.sound_precache[i] = cgi_S_RegisterSound(soundName);
	}
}

/*
=============================================================================

CLIENT INFO

=============================================================================
*/

/*
==========================
CG_RegisterClientSkin
==========================
*/
qboolean CG_RegisterClientSkin(clientInfo_t* ci,
	const char* headModelName, const char* headSkinName,
	const char* torsoModelName, const char* torsoSkinName,
	const char* legsModelName, const char* legsSkinName)
{
	char lfilename[MAX_QPATH];

	Com_sprintf(lfilename, sizeof lfilename, "models/players/%s/lower_%s.skin", legsModelName, legsSkinName);
	ci->legsSkin = cgi_R_RegisterSkin(lfilename);

	if (!ci->legsSkin)
	{
		//		Com_Printf( "Failed to load skin file: %s : %s\n", legsModelName, legsSkinName );
		//return qfalse;
	}

	if (torsoModelName && torsoSkinName && torsoModelName[0] && torsoSkinName[0])
	{
		char tfilename[MAX_QPATH];
		Com_sprintf(tfilename, sizeof tfilename, "models/players/%s/upper_%s.skin", torsoModelName, torsoSkinName);
		ci->torsoSkin = cgi_R_RegisterSkin(tfilename);

		if (!ci->torsoSkin)
		{
			Com_Printf("Failed to load skin file: %s : %s\n", torsoModelName, torsoSkinName);
			return qfalse;
		}
	}

	if (headModelName && headSkinName && headModelName[0] && headSkinName[0])
	{
		char hfilename[MAX_QPATH];
		Com_sprintf(hfilename, sizeof hfilename, "models/players/%s/head_%s.skin", headModelName, headSkinName);
		ci->headSkin = cgi_R_RegisterSkin(hfilename);

		if (!ci->headSkin)
		{
			Com_Printf("Failed to load skin file: %s : %s\n", headModelName, headSkinName);
			return qfalse;
		}
	}

	return qtrue;
}

/*
==========================
CG_RegisterClientModelname
==========================
*/
qboolean CG_RegisterClientModelname(clientInfo_t* ci,
	const char* headModelName, const char* headSkinName,
	const char* torsoModelName, const char* torsoSkinName,
	const char* legsModelName, const char* legsSkinName)
{
	/*
	Ghoul2 Insert Start
	*/
#if 1
	char filename[MAX_QPATH];

	if (!legsModelName || !legsModelName[0])
	{
		return qtrue;
	}
	Com_sprintf(filename, sizeof filename, "models/players/%s/lower.mdr", legsModelName);
	ci->legsModel = cgi_R_RegisterModel(filename);
	if (!ci->legsModel)
	{
		//he's not skeletal, try the old way
		Com_sprintf(filename, sizeof filename, "models/players/%s/lower.md3", legsModelName);
		ci->legsModel = cgi_R_RegisterModel(filename);
		if (!ci->legsModel)
		{
			Com_Printf(S_COLOR_RED"Failed to load model file %s\n", filename);
			return qfalse;
		}
	}

	if (torsoModelName && torsoModelName[0])
	{
		//You are trying to set one
		Com_sprintf(filename, sizeof filename, "models/players/%s/upper.mdr", torsoModelName);
		ci->torsoModel = cgi_R_RegisterModel(filename);
		if (!ci->torsoModel)
		{
			//he's not skeletal, try the old way
			Com_sprintf(filename, sizeof filename, "models/players/%s/upper.md3", torsoModelName);
			ci->torsoModel = cgi_R_RegisterModel(filename);
			if (!ci->torsoModel)
			{
				Com_Printf(S_COLOR_RED"Failed to load model file %s\n", filename);
				return qfalse;
			}
		}
	}
	else
	{
		ci->torsoModel = 0;
	}

	if (headModelName && headModelName[0])
	{
		//You are trying to set one
		Com_sprintf(filename, sizeof filename, "models/players/%s/head.md3", headModelName);
		ci->headModel = cgi_R_RegisterModel(filename);
		if (!ci->headModel)
		{
			Com_Printf(S_COLOR_RED"Failed to load model file %s\n", filename);
			return qfalse;
		}
	}
	else
	{
		ci->headModel = 0;
	}

	// if any skins failed to load, return failure
	if (!CG_RegisterClientSkin(ci, headModelName, headSkinName, torsoModelName, torsoSkinName, legsModelName,
		legsSkinName))
	{
		//Com_Printf( "Failed to load skin file: %s : %s/%s : %s/%s : %s\n", headModelName, headSkinName, torsoModelName, torsoSkinName, legsModelName, legsSkinName );
		return qfalse;
	}

	//FIXME: for now, uses the legs model dir for anim cfg, but should we set this in some sort of NPCs.cfg?
	// load the animation file set
	ci->animFileIndex = G_ParseAnimFileSet(legsModelName);
	if (ci->animFileIndex < 0)
	{
		Com_Printf(S_COLOR_RED"Failed to load animation file set models/players/%s\n", legsModelName);
		return qfalse;
	}
#endif
	/*
	Ghoul2 Insert End
	*/
	return qtrue;
}

void CG_RegisterClientRenderInfo(clientInfo_t* ci, const renderInfo_t* ri)
{
	char headModelName[MAX_QPATH];
	char torsoModelName[MAX_QPATH];
	char legsModelName[MAX_QPATH];
	char headSkinName[MAX_QPATH];
	char torsoSkinName[MAX_QPATH];
	char legsSkinName[MAX_QPATH];

	if (!ri->legsModelName[0])
	{
		//Must have at LEAST a legs model
		return;
	}

	Q_strncpyz(legsModelName, ri->legsModelName, sizeof legsModelName);
	//Legs skin
	char* slash = strchr(legsModelName, '/');
	if (!slash)
	{
		// modelName didn not include a skin name
		Q_strncpyz(legsSkinName, "default", sizeof legsSkinName);
	}
	else
	{
		Q_strncpyz(legsSkinName, slash + 1, sizeof legsSkinName);
		// truncate modelName
		*slash = 0;
	}

	if (ri->torsoModelName[0])
	{
		Q_strncpyz(torsoModelName, ri->torsoModelName, sizeof torsoModelName);
		//Torso skin
		slash = strchr(torsoModelName, '/');
		if (!slash)
		{
			// modelName didn't include a skin name
			Q_strncpyz(torsoSkinName, "default", sizeof torsoSkinName);
		}
		else
		{
			Q_strncpyz(torsoSkinName, slash + 1, sizeof torsoSkinName);
			// truncate modelName
			*slash = 0;
		}
	}
	else
	{
		torsoModelName[0] = 0;
	}

	//Head
	if (ri->headModelName[0])
	{
		Q_strncpyz(headModelName, ri->headModelName, sizeof headModelName);
		//Head skin
		slash = strchr(headModelName, '/');
		if (!slash)
		{
			// modelName didn not include a skin name
			Q_strncpyz(headSkinName, "default", sizeof headSkinName);
		}
		else
		{
			Q_strncpyz(headSkinName, slash + 1, sizeof headSkinName);
			// truncate modelName
			*slash = 0;
		}
	}
	else
	{
		headModelName[0] = 0;
	}

	if (!CG_RegisterClientModelname(ci, headModelName, headSkinName, torsoModelName, torsoSkinName, legsModelName,
		legsSkinName))
	{
		if (!CG_RegisterClientModelname(ci, DEFAULT_HEADMODEL, "default", DEFAULT_TORSOMODEL, "default",
			DEFAULT_LEGSMODEL, "default"))
		{
			CG_Error("DEFAULT_MODELS failed to register");
		}
	}
}

//-------------------------------------
// CG_RegisterEffects
//
// Handles precaching all effect files
//	and any shader, model, or sound
//	files an effect may use.
//-------------------------------------
extern void CG_InitGlass();
extern void cgi_RE_WorldEffectCommand(const char* command);

extern cvar_t* g_delayedShutdown;

static void CG_RegisterEffects()
{
	char* effectName;
	int i, numFailed = 0;

	// Register external effects
	for (i = 1; i < MAX_FX; i++)
	{
		effectName = const_cast<char*>(CG_ConfigString(CS_EFFECTS + i));

		if (!effectName[0])
		{
			break;
		}

		if (!theFxScheduler.RegisterEffect(effectName))
		{
			//assert(0);
			numFailed++;
		}
	}
	if (numFailed && g_delayedShutdown->integer)
	{
		//assert(0);
		//CG_Error( "CG_RegisterEffects: %i Effects failed to load.  Please fix, or ask Aurelio.", numFailed );
	}

	// Start world effects
	for (i = 1; i < MAX_WORLD_FX; i++)
	{
		effectName = const_cast<char*>(CG_ConfigString(CS_WORLD_FX + i));

		if (!effectName[0])
		{
			break;
		}

		cgi_RE_WorldEffectCommand(effectName);
	}

	// Set up the glass effects mini-system.
	CG_InitGlass();

	cgs.effects.mHyperspaceStars = theFxScheduler.RegisterEffect("ships/hyperspace_stars");
	cgs.effects.mShipDestBurning = theFxScheduler.RegisterEffect("ships/dest_burning");

	//footstep effects
	cgs.effects.footstepMud = theFxScheduler.RegisterEffect("materials/mud");
	cgs.effects.footstepSand = theFxScheduler.RegisterEffect("materials/sand");
	cgs.effects.footstepSnow = theFxScheduler.RegisterEffect("materials/snow");
	cgs.effects.footstepGravel = theFxScheduler.RegisterEffect("materials/gravel");
	//landing effects
	cgs.effects.landingMud = theFxScheduler.RegisterEffect("materials/mud_large");
	cgs.effects.landingSand = theFxScheduler.RegisterEffect("materials/sand_large");
	cgs.effects.landingDirt = theFxScheduler.RegisterEffect("materials/dirt_large");
	cgs.effects.landingSnow = theFxScheduler.RegisterEffect("materials/snow_large");
	cgs.effects.landingGravel = theFxScheduler.RegisterEffect("materials/gravel_large");
	cgs.effects.landingLava = theFxScheduler.RegisterEffect("materials/dirt_large");
	cgs.effects.mSaberFriction = theFxScheduler.RegisterEffect("saber/saber_friction");

	cgs.effects.mSaberLock = theFxScheduler.RegisterEffect("saber/saber_lock");

	//splashes
	if (gi.totalMapContents() & CONTENTS_WATER)
	{
		theFxScheduler.RegisterEffect("env/water_impact");
		theFxScheduler.RegisterEffect("misc/waterbreath");
	}
	if (gi.totalMapContents() & CONTENTS_LAVA)
	{
		theFxScheduler.RegisterEffect("env/lava_splash");
	}
	if (gi.totalMapContents() & CONTENTS_SLIME)
	{
		theFxScheduler.RegisterEffect("env/acid_splash");
	}
	theFxScheduler.RegisterEffect("misc/breath");

	theFxScheduler.RegisterEffect("force/strike_flare");

	theFxScheduler.RegisterEffect("force/yellowstrike");

	theFxScheduler.RegisterEffect("env/yellow_lightning");
}

/*
void CG_RegisterClientModels (int entity_num)

Only call if clientInfo->infoValid is not true

For players and NPCs to register their models
*/
void CG_RegisterClientModels(const int entity_num)
{
	if (entity_num < 0 || entity_num > ENTITYNUM_WORLD)
	{
		return;
	}

	const gentity_t* ent = &g_entities[entity_num];

	if (!ent->client)
	{
		return;
	}

	ent->client->clientInfo.infoValid = qtrue;

	if (ent->playerModel != -1 && ent->ghoul2.size())
	{
		return;
	}

	CG_RegisterClientRenderInfo(&ent->client->clientInfo, &ent->client->renderInfo);

	if (entity_num < MAX_CLIENTS)
	{
		memcpy(&cgs.clientinfo[entity_num], &ent->client->clientInfo, sizeof(clientInfo_t));
	}
}

//===================================================================================

forceTicPos_t forceTicPos[] =
{
	{11, 41, 20, 10, "gfx/hud/DataPadforce_tick1", NULL_HANDLE}, // Left Top
	{12, 45, 20, 10, "gfx/hud/DataPadforce_tick2", NULL_HANDLE},
	{14, 49, 20, 10, "gfx/hud/DataPadforce_tick3", NULL_HANDLE},
	{17, 52, 20, 10, "gfx/hud/DataPadforce_tick4", NULL_HANDLE},
	{22, 55, 10, 10, "gfx/hud/DataPadforce_tick5", NULL_HANDLE},
	{28, 57, 10, 20, "gfx/hud/DataPadforce_tick6", NULL_HANDLE},
	{34, 59, 10, 10, "gfx/hud/DataPadforce_tick7", NULL_HANDLE}, // Left bottom

	{46, 59, -10, 10, "gfx/hud/DataPadforce_tick7", NULL_HANDLE}, // Right bottom
	{52, 57, -10, 20, "gfx/hud/DataPadforce_tick6", NULL_HANDLE},
	{58, 55, -10, 10, "gfx/hud/DataPadforce_tick5", NULL_HANDLE},
	{63, 52, -20, 10, "gfx/hud/DataPadforce_tick4", NULL_HANDLE},
	{66, 49, -20, 10, "gfx/hud/DataPadforce_tick3", NULL_HANDLE},
	{68, 45, -20, 10, "gfx/hud/DataPadforce_tick2", NULL_HANDLE},
	{69, 41, -20, 10, "gfx/hud/DataPadforce_tick1", NULL_HANDLE}, // Right top
};

forceTicPos_t JK2forceTicPos[] =
{
	{11, 41, 20, 10, "gfx/hud/JK2force_tick1", NULL_HANDLE}, // Left Top
	{12, 45, 20, 10, "gfx/hud/JK2force_tick2", NULL_HANDLE},
	{14, 49, 20, 10, "gfx/hud/JK2force_tick3", NULL_HANDLE},
	{17, 52, 20, 10, "gfx/hud/JK2force_tick4", NULL_HANDLE},
	{22, 55, 10, 10, "gfx/hud/JK2force_tick5", NULL_HANDLE},
	{28, 57, 10, 20, "gfx/hud/JK2force_tick6", NULL_HANDLE},
	{34, 59, 10, 10, "gfx/hud/JK2force_tick7", NULL_HANDLE}, // Left bottom

	{46, 59, -10, 10, "gfx/hud/JK2force_tick7", NULL_HANDLE}, // Right bottom
	{52, 57, -10, 20, "gfx/hud/JK2force_tick6", NULL_HANDLE},
	{58, 55, -10, 10, "gfx/hud/JK2force_tick5", NULL_HANDLE},
	{63, 52, -20, 10, "gfx/hud/JK2force_tick4", NULL_HANDLE},
	{66, 49, -20, 10, "gfx/hud/JK2force_tick3", NULL_HANDLE},
	{68, 45, -20, 10, "gfx/hud/JK2force_tick2", NULL_HANDLE},
	{69, 41, -20, 10, "gfx/hud/JK2force_tick1", NULL_HANDLE}, // Right top
};

forceTicPos_t ammoTicPos[] =
{
	{12, 34, 10, 10, "gfx/hud/DataPadammo_tick7-l", NULL_HANDLE}, // Bottom
	{13, 28, 10, 10, "gfx/hud/DataPadammo_tick6-l", NULL_HANDLE},
	{15, 23, 10, 10, "gfx/hud/DataPadammo_tick5-l", NULL_HANDLE},
	{19, 19, 10, 10, "gfx/hud/DataPadammo_tick4-l", NULL_HANDLE},
	{23, 15, 10, 10, "gfx/hud/DataPadammo_tick3-l", NULL_HANDLE},
	{29, 12, 10, 10, "gfx/hud/DataPadammo_tick2-l", NULL_HANDLE},
	{34, 11, 10, 10, "gfx/hud/DataPadammo_tick1-l", NULL_HANDLE},

	{47, 11, -10, 10, "gfx/hud/DataPadammo_tick1-r", NULL_HANDLE},
	{52, 12, -10, 10, "gfx/hud/DataPadammo_tick2-r", NULL_HANDLE},
	{58, 15, -10, 10, "gfx/hud/DataPadammo_tick3-r", NULL_HANDLE},
	{62, 19, -10, 10, "gfx/hud/DataPadammo_tick4-r", NULL_HANDLE},
	{66, 23, -10, 10, "gfx/hud/DataPadammo_tick5-r", NULL_HANDLE},
	{68, 28, -10, 10, "gfx/hud/DataPadammo_tick6-r", NULL_HANDLE},
	{69, 34, -10, 10, "gfx/hud/DataPadammo_tick7-r", NULL_HANDLE},
};

forceTicPos_t JK2ammoTicPos[] =
{
	{12, 34, 10, 10, "gfx/hud/JK2ammo_tick7-l", NULL_HANDLE}, // Bottom
	{13, 28, 10, 10, "gfx/hud/JK2ammo_tick6-l", NULL_HANDLE},
	{15, 23, 10, 10, "gfx/hud/JK2ammo_tick5-l", NULL_HANDLE},
	{19, 19, 10, 10, "gfx/hud/JK2ammo_tick4-l", NULL_HANDLE},
	{23, 15, 10, 10, "gfx/hud/JK2ammo_tick3-l", NULL_HANDLE},
	{29, 12, 10, 10, "gfx/hud/JK2ammo_tick2-l", NULL_HANDLE},
	{34, 11, 10, 10, "gfx/hud/JK2ammo_tick1-l", NULL_HANDLE},

	{47, 11, -10, 10, "gfx/hud/JK2ammo_tick1-r", NULL_HANDLE},
	{52, 12, -10, 10, "gfx/hud/JK2ammo_tick2-r", NULL_HANDLE},
	{58, 15, -10, 10, "gfx/hud/JK2ammo_tick3-r", NULL_HANDLE},
	{62, 19, -10, 10, "gfx/hud/JK2ammo_tick4-r", NULL_HANDLE},
	{66, 23, -10, 10, "gfx/hud/JK2ammo_tick5-r", NULL_HANDLE},
	{68, 28, -10, 10, "gfx/hud/JK2ammo_tick6-r", NULL_HANDLE},
	{69, 34, -10, 10, "gfx/hud/JK2ammo_tick7-r", NULL_HANDLE},
};

HUDMenuItem_t forceTics[] =
{
	{"rightHUD", "force_tic1", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "force_tic2", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "force_tic3", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "force_tic4", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
};

HUDMenuItem_t forceTics_md[] =
{
	{"rightHUD", "force_tic1SP", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "force_tic2SP", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "force_tic3SP", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "force_tic4SP", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
};

HUDMenuItem_t blockpoint_tics[] =
{
	{"leftHUD", "block_tic1", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"leftHUD", "block_tic2", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, //
	{"leftHUD", "block_tic3", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, //
	{"leftHUD", "block_tic4", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Bottom
};

HUDMenuItem_t ammoTics[] =
{
	{"rightHUD", "ammo_tic1", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "ammo_tic2", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "ammo_tic3", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "ammo_tic4", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
};

HUDMenuItem_t ammoTics_md[] =
{
	{"rightHUD", "ammo_tic1SP", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "ammo_tic2SP", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "ammo_tic3SP", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "ammo_tic4SP", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
};

HUDMenuItem_t armorTics[] =
{
	{"leftHUD", "armor_tic1", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"leftHUD", "armor_tic2", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "armor_tic3", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "armor_tic4", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
};

HUDMenuItem_t armorTics_md[] =
{
	{"leftHUD", "armor_tic1SP", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"leftHUD", "armor_tic2SP", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "armor_tic3SP", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "armor_tic4SP", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
};

HUDMenuItem_t healthTics[] =
{
	{"leftHUD", "health_tic1", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"leftHUD", "health_tic2", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, //
	{"leftHUD", "health_tic3", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, //
	{"leftHUD", "health_tic4", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Bottom
};
// new df hud
HUDMenuItem_t df_health_tics[] =
{
	{"leftHUD", "DF-Health-Tic1", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"leftHUD", "DF-Health-Tic2", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, //
	{"leftHUD", "DF-Health-Tic3", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, //
	{"leftHUD", "DF-Health-Tic4", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Bottom
	{"leftHUD", "DF-Health-Tic5", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, //
	{"leftHUD", "DF-Health-Tic6", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, //
	{"leftHUD", "DF-Health-Tic7", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Bottom
	{"leftHUD", "DF-Health-Tic8", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, //
	{"leftHUD", "DF-Health-Tic9", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, //
	{"leftHUD", "DF-Health-Tic10", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Bottom
	{"leftHUD", "DF-Health-Tic11", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, //
	{"leftHUD", "DF-Health-Tic12", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, //
	{"leftHUD", "DF-Health-Tic13", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Bottom
	{"leftHUD", "DF-Health-Tic14", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, //
	{"leftHUD", "DF-Health-Tic15", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, //
	{"leftHUD", "DF-Health-Tic16", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Bottom
};

HUDMenuItem_t df_armorTics[] =
{
	{"leftHUD", "DF-armor_tic1", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"leftHUD", "DF-armor_tic2", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-armor_tic3", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-armor_tic4", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-armor_tic5", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"leftHUD", "DF-armor_tic6", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-armor_tic7", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-armor_tic8", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-armor_tic9", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"leftHUD", "DF-armor_tic10", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-armor_tic11", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-armor_tic12", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-armor_tic13", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"leftHUD", "DF-armor_tic14", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-armor_tic15", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-armor_tic16", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
};

HUDMenuItem_t df_staminaTics[] =
{
	{"leftHUD", "DF-stamina_tic1", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"leftHUD", "DF-stamina_tic2", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-stamina_tic3", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-stamina_tic4", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-stamina_tic5", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"leftHUD", "DF-stamina_tic6", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-stamina_tic7", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-stamina_tic8", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-stamina_tic9", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"leftHUD", "DF-stamina_tic10", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-stamina_tic11", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-stamina_tic12", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-stamina_tic13", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"leftHUD", "DF-stamina_tic14", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-stamina_tic15", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-stamina_tic16", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
};

HUDMenuItem_t df_fuelTics[] =
{
	{"leftHUD", "DF-fuel_tic1", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"leftHUD", "DF-fuel_tic2", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-fuel_tic3", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-fuel_tic4", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-fuel_tic5", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"leftHUD", "DF-fuel_tic6", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-fuel_tic7", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-fuel_tic8", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-fuel_tic9", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"leftHUD", "DF-fuel_tic10", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-fuel_tic11", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-fuel_tic12", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-fuel_tic13", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"leftHUD", "DF-fuel_tic14", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-fuel_tic15", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-fuel_tic16", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
};

HUDMenuItem_t df_cloakTics[] =
{
	{"leftHUD", "DF-cloak_tic1", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"leftHUD", "DF-cloak_tic2", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-cloak_tic3", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-cloak_tic4", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-cloak_tic5", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"leftHUD", "DF-cloak_tic6", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-cloak_tic7", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-cloak_tic8", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-cloak_tic9", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"leftHUD", "DF-cloak_tic10", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-cloak_tic11", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-cloak_tic12", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-cloak_tic13", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"leftHUD", "DF-cloak_tic14", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-cloak_tic15", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-cloak_tic16", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
};

HUDMenuItem_t df_barrierTics[] =
{
	{"leftHUD", "DF-barrier_tic1", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"leftHUD", "DF-barrier_tic2", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-barrier_tic3", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-barrier_tic4", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-barrier_tic5", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"leftHUD", "DF-barrier_tic6", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-barrier_tic7", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-barrier_tic8", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-barrier_tic9", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"leftHUD", "DF-barrier_tic10", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-barrier_tic11", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-barrier_tic12", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-barrier_tic13", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"leftHUD", "DF-barrier_tic14", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-barrier_tic15", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"leftHUD", "DF-barrier_tic16", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
};

HUDMenuItem_t df_forceTics[] =
{
	{"rightHUD", "DF-force_tic1", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "DF-force_tic2", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-force_tic3", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-force_tic4", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-force_tic5", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "DF-force_tic6", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-force_tic7", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-force_tic8", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-force_tic9", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "DF-force_tic10", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-force_tic11", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-force_tic12", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-force_tic13", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "DF-force_tic14", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-force_tic15", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-force_tic16", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
};

HUDMenuItem_t df_blockTics[] =
{
	{"rightHUD", "DF-block_tic1", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "DF-block_tic2", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-block_tic3", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-block_tic4", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-block_tic5", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "DF-block_tic6", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-block_tic7", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-block_tic8", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-block_tic9", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "DF-block_tic10", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-block_tic11", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-block_tic12", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-block_tic13", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "DF-block_tic14", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-block_tic15", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-block_tic16", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
};

HUDMenuItem_t df_ammoTics[] =
{
	{"rightHUD", "DF-ammo_tic1", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "DF-ammo_tic2", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-ammo_tic3", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-ammo_tic4", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-ammo_tic5", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "DF-ammo_tic6", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-ammo_tic7", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-ammo_tic8", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-ammo_tic9", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "DF-ammo_tic10", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-ammo_tic11", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-ammo_tic12", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-ammo_tic13", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "DF-ammo_tic14", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-ammo_tic15", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	{"rightHUD", "DF-ammo_tic16", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
};

HUDMenuItem_t healthTics_md[] =
{
	{"leftHUD", "health_tic1SP", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"leftHUD", "health_tic2SP", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, //
	{"leftHUD", "health_tic3SP", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, //
	{"leftHUD", "health_tic4SP", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Bottom
};

HUDMenuItem_t mishapTics[] =
{
	{"rightHUD", "mishap_tic1", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "mishap_tic2", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "mishap_tic3", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "mishap_tic4", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
};

HUDMenuItem_t classicmishapTics[] =
{
	{"rightHUD", "classicsabfat_tic1", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "classicsabfat_tic4", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "classicsabfat_tic8", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "classicsabfat_tic14", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
};

HUDMenuItem_t sabfatticS[] =
{
	{"rightHUD", "sabfat_tic1", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "sabfat_tic2", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "sabfat_tic3", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "sabfat_tic4", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "sabfat_tic5", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "sabfat_tic6", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "sabfat_tic7", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "sabfat_tic8", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "sabfat_tic9", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "sabfat_tic10", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "sabfat_tic11", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "sabfat_tic12", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "sabfat_tic13", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "sabfat_tic14", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "sabfat_tic15", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
};

HUDMenuItem_t classicsabfatticS[] =
{
	{"rightHUD", "classicsabfat_tic1", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "classicsabfat_tic2", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "classicsabfat_tic3", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "classicsabfat_tic4", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "classicsabfat_tic5", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "classicsabfat_tic6", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "classicsabfat_tic7", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "classicsabfat_tic8", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "classicsabfat_tic9", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "classicsabfat_tic10", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "classicsabfat_tic11", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "classicsabfat_tic12", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "classicsabfat_tic13", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "classicsabfat_tic14", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "classicsabfat_tic15", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
};

HUDMenuItem_t newsabfatticS[] =
{
	{"rightHUD", "newsabfat_tic1", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "newsabfat_tic2", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "newsabfat_tic3", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "newsabfat_tic4", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "newsabfat_tic5", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "newsabfat_tic6", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "newsabfat_tic7", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "newsabfat_tic8", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "newsabfat_tic9", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "newsabfat_tic10", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "newsabfat_tic11", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "newsabfat_tic12", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "newsabfat_tic13", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "newsabfat_tic14", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
	{"rightHUD", "newsabfat_tic15", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // Top
};

HUDMenuItem_t otherHUDBits[] =
{
	{"lefthud", "healthamount", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_HEALTHAMOUNT
	{"lefthud", "healthamount_md", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_HEALTHAMOUNT_MD
	{"lefthud", "healthamount_df", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_HEALTHAMOUNT_DF
	{"lefthud", "healthamountalt", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_HEALTHAMOUNTALT
	{"lefthud", "blockpointamount", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_BLOCKAMOUNT
	{"righthud", "blockpointamount_df", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_BLOCKAMOUNT_DF
	{"lefthud", "JK2blockpointamount", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_JK2BLOCKAMOUNT
	{"lefthud", "Classicblockpointamount", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	// OHB_CLASSICBLOCKAMOUNT
	{"lefthud", "armoramount", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_ARMORAMOUNT
	{"lefthud", "armoramount_md", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_ARMORAMOUNT_MD
	{"lefthud", "armoramount_df", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_ARMORAMOUNT_DF
	{"righthud", "forceamount", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_FORCEAMOUNT
	{"righthud", "forceamount_md", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_FORCEAMOUNT_MD
	{"righthud", "forceamount_df", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_FORCEAMOUNT_DF
	{"righthud", "JK2forceamount", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_JK2FORCEAMOUNT
	{"righthud", "ammoamount", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_AMMOAMOUNT
	{"righthud", "ammoamount_md", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_AMMOAMOUNT_MD
	{"righthud", "ammoamount_df", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_AMMOAMOUNT_DF

	{"righthud", "saberstyle_strong", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_SABERSTYLE_STRONG
	{"righthud", "saberstyle_strongSP", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	// OHB_SABERSTYLE_STRONG_MD
	{"righthud", "saberstyle_strongDF", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	// OHB_SABERSTYLE_STRONG_DF

	{"righthud", "saberstyle_medium", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_SABERSTYLE_MEDIUM
	{"righthud", "saberstyle_mediumSP", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	// OHB_SABERSTYLE_MEDIUM_MD
	{"righthud", "saberstyle_mediumDF", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	// OHB_SABERSTYLE_MEDIUM_DF

	{"righthud", "saberstyle_fast", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_SABERSTYLE_FAST
	{"righthud", "saberstyle_fastSP", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_SABERSTYLE_FAST_MD
	{"righthud", "saberstyle_fastDF", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_SABERSTYLE_FAST_DF

	{"righthud", "saberstyle_tavion", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_SABERSTYLE_TAVION
	{"righthud", "saberstyle_tavionSP", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	// OHB_SABERSTYLE_TAVION_MD
	{"righthud", "saberstyle_tavionDF", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	// OHB_SABERSTYLE_TAVION_DF

	{"righthud", "saberstyle_desann", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_SABERSTYLE_DESANN
	{"righthud", "saberstyle_desannSP", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	// OHB_SABERSTYLE_DESANN_MD
	{"righthud", "saberstyle_desannDF", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	// OHB_SABERSTYLE_DESANN_DF

	{"righthud", "saberstyle_staff", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_SABERSTYLE_STAFF
	{"righthud", "saberstyle_staffSP", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	// OHB_SABERSTYLE_STAFF_MD
	{"righthud", "saberstyle_staffDF", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	// OHB_SABERSTYLE_STAFF_DF

	{"righthud", "saberstyle_dual", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_SABERSTYLE_DUAL
	{"righthud", "saberstyle_dualSP", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_SABERSTYLE_DUAL_MD
	{"righthud", "saberstyle_dualDF", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_SABERSTYLE_DUAL_DF

	{"righthud", "blasterstyle_DF", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_BLASTERSTYLE_DF

	{"righthud", "weapontype_stun_baton", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_STUN_BATON
	{"righthud", "weapontype_briar_pistol", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_BRIAR_PISTOL
	{"righthud", "weapontype_blaster_pistol", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	// OHB_BLASTER_PISTOL
	{"righthud", "weapontype_blaster", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_BLASTER
	{"righthud", "weapontype_disruptor", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_DISRUPTOR
	{"righthud", "weapontype_bowcaster", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_BOWCASTER
	{"righthud", "weapontype_repeater", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_REPEATER
	{"righthud", "weapontype_demp2", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_DEMP2
	{"righthud", "weapontype_flachette", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_FLACHETTE
	{"righthud", "weapontype_rocket", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_ROCKET
	{"righthud", "weapontype_thermal", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_THERMAL
	{"righthud", "weapontype_tripmine", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_TRIPMINE
	{"righthud", "weapontype_detpack", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_DETPACK
	{"righthud", "weapontype_concussion", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_CONCUSSION
	{"righthud", "weapontype_melee", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_MELEE
	{"righthud", "weapontype_battledroid", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_BATTLEDROID
	{"righthud", "weapontype_thefirstorder", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	// OHB_THEFIRSTORDER
	{"righthud", "weapontype_clonecarbine", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_CLONECARBINE
	{"righthud", "weapontype_rebelblaster", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_REBELBLASTER
	{"righthud", "weapontype_clonerifle", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_CLONERIFLE
	{"righthud", "weapontype_clonecommando", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE},
	// OHB_CLONECOMMANDO
	{"righthud", "weapontype_rebelrifle", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_REBELRIFLE
	{"righthud", "weapontype_rey", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_REY
	{"righthud", "weapontype_jango", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_JANGO
	{"righthud", "weapontype_boba", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_BOBA
	{"righthud", "weapontype_clonepistol", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_CLONEPISTOL
	{"righthud", "weapontype_wrist", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_WRIST
	{"righthud", "weapontype_jangodual", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_DUAL
	{"righthud", "weapontype_droideka", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_DEKA
	{"righthud", "weapontype_nogri", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_NOGRI
	{"righthud", "weapontype_tusken", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_TUSKEN
	// OHB_KOTOR
	{"righthud", "weapontype_kotor_bowcaster2", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_KOTOR_BOWCASTER
	{"righthud", "weapontype_kotor_bpistol2", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_KOTOR_BPISTOL2
	{"righthud", "weapontype_kotor_bpistol3", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_KOTOR_BPISTOL3
	{"righthud", "weapontype_kotor_brifle1", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_KOTOR_BRIFLE1
	{"righthud", "weapontype_kotor_brifle2", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_KOTOR_BRIFLE2
	{"righthud", "weapontype_kotor_brifle3", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_KOTOR_BRIFLE3
	{"righthud", "weapontype_kotor_carbine", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_KOTOR_CARBINE
	{"righthud", "weapontype_kotor_drifle", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_KOTOR_DRIFLE
	{"righthud", "weapontype_kotor_hobpistol", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_KOTOR_HOBPISTOL
	{"righthud", "weapontype_kotor_hpistol", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_KOTOR_HPISTOL
	{"righthud", "weapontype_kotor_ionrifle", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_KOTOR_IONRIFLE
	{"righthud", "weapontype_kotor_pistol1", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_KOTOR_PISTOL1
	{"righthud", "weapontype_kotor_repeater", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_KOTOR_REPEATER
	{"righthud", "weapontype_kotor_reprifle", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_KOTOR_REPRIFLE
	/////////////
	{"lefthud", "scanline", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_SCANLINE_LEFT
	{"righthud", "scanline", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_SCANLINE_RIGHT
	{"lefthud", "frame", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_FRAME_LEFT
	{"righthud", "frame", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_FRAME_RIGHT
	{"lefthud", "frame_classic", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_CLASSIC_FRAME_LEFT
	{"righthud", "frame_classic", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_CLASSIC_FRAME_RIGHT
	{"lefthud", "frame_md", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_FRAME_LEFT_MD
	{"righthud", "frame_md", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_FRAME_RIGHT_MD
	{"righthud", "MBlockingMode", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_MBLOCKINGMODE
	{"righthud", "BlockingMode", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_BLOCKINGMODE
	{"righthud", "NotBlocking", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_NOTBLOCKING
	{"righthud", "JK2MBlockingMode", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_JK2MBLOCKINGMODE
	{"righthud", "JK2BlockingMode", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_JK2BLOCKINGMODE
	{"righthud", "JK2NotBlocking", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_JK2NOTBLOCKING

	{"lefthud", "left_frame_df", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_LEFT_FRAME_DF
	{"lefthud", "left_frame_df2", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_LEFT_FRAME_DF2
	{"lefthud", "left_center_df", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_LEFT_CENTER_DF
	{"lefthud", "left_ring_df", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_LEFT_RING_DF
	{"righthud", "right_frame_df", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_RIGHT_FRAME_DF
	{"righthud", "right_frame_df2", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_RIGHT_FRAME_DF2
	{"righthud", "right_center_df", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_RIGHT_CENTER_DF
	{"righthud", "right_ring_df", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_RIGHT_RING_DF
	{"righthud", "hilt_df", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_HILT_DF
	{"righthud", "block_df", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_BLOCK_DF
	{"righthud", "mblock_df", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_MBLOCK_DF
	{"lefthud", "health_icon_df", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_HEALTH_ICON_DF
	{"lefthud", "armor_icon_df", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_ARMOR_ICON_DF
	{"lefthud", "stamina_icon_df", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_STAMINA_ICON_DF
	{"lefthud", "fuel_icon_df", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_FUEL_ICON_DF
	{"lefthud", "cloak_icon_df", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_CLOAK_ICON_DF
	{"lefthud", "barrier_icon_df", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_BARRIER_ICON_DF
	{"righthud", "force_icon_df", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_FORCE_ICON_DF
	{"righthud", "block_icon_df", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_BLOCK_ICON_DF
	{"righthud", "ammo_icon_df", 0, 0, 0, 0, {0.0f, 0.0f, 0.0f, 0.0f}, NULL_HANDLE}, // OHB_AMMO_ICON_DF
};

extern void CG_NPC_Precache(gentity_t* spawner);
qboolean NPCsPrecached = qfalse;

/*
=================
CG_IsWeaponUsablePlayer

These weapons are not really used by the player, so let's not preregister them.
Some weapons like the noghri stick can be used by the player
but those are in special circumstances.
=================
*/
static qboolean CG_IsWeaponUsablePlayer(const int weaponNum)
{
	if (weaponNum == WP_ATST_MAIN || weaponNum == WP_ATST_SIDE || weaponNum == WP_EMPLACED_GUN
		|| weaponNum == WP_BOT_LASER || weaponNum == WP_TURRET || weaponNum == WP_TIE_FIGHTER
		|| weaponNum == WP_RAPID_FIRE_CONC || weaponNum == WP_JAWA || weaponNum == WP_TUSKEN_RIFLE
		|| weaponNum == WP_TUSKEN_STAFF || weaponNum == WP_SCEPTER || weaponNum == WP_NOGHRI_STICK)
	{
		return qfalse;
	}

	return qtrue;
}

/*
=================
CG_PrepRefresh

Call before entering a new level, or after changing renderers
This function may execute for a couple of minutes with a slow disk.
=================
*/
void CG_CreateMiscEnts();

static void CG_RegisterGraphics()
{
	int i;
	char items[MAX_ITEMS + 1];
	const char* sb_nums[11] = {
		"gfx/2d/numbers/zero",
		"gfx/2d/numbers/one",
		"gfx/2d/numbers/two",
		"gfx/2d/numbers/three",
		"gfx/2d/numbers/four",
		"gfx/2d/numbers/five",
		"gfx/2d/numbers/six",
		"gfx/2d/numbers/seven",
		"gfx/2d/numbers/eight",
		"gfx/2d/numbers/nine",
		"gfx/2d/numbers/minus",
	};

	const char* sb_t_nums[11] = {
		"gfx/2d/numbers/t_zero",
		"gfx/2d/numbers/t_one",
		"gfx/2d/numbers/t_two",
		"gfx/2d/numbers/t_three",
		"gfx/2d/numbers/t_four",
		"gfx/2d/numbers/t_five",
		"gfx/2d/numbers/t_six",
		"gfx/2d/numbers/t_seven",
		"gfx/2d/numbers/t_eight",
		"gfx/2d/numbers/t_nine",
		"gfx/2d/numbers/t_minus",
	};

	const char* sb_c_nums[11] = {
		"gfx/2d/numbers/c_zero",
		"gfx/2d/numbers/c_one",
		"gfx/2d/numbers/c_two",
		"gfx/2d/numbers/c_three",
		"gfx/2d/numbers/c_four",
		"gfx/2d/numbers/c_five",
		"gfx/2d/numbers/c_six",
		"gfx/2d/numbers/c_seven",
		"gfx/2d/numbers/c_eight",
		"gfx/2d/numbers/c_nine",
		"gfx/2d/numbers/t_minus", //?????
	};

	// Clean, then register...rinse...repeat...
	CG_LoadingString("effects");
	FX_Init();
	CG_RegisterEffects();

	// clear any references to old media
	memset(&cg.refdef, 0, sizeof cg.refdef);
	cgi_R_ClearScene();

	cg.loadLCARSStage = 3;
	CG_LoadingString(cgs.mapname);

	cgi_R_LoadWorldMap(cgs.mapname);

	cg.loadLCARSStage = 4;
	CG_LoadingString("game media shaders");

	for (i = 0; i < 11; i++)
	{
		cgs.media.numberShaders[i] = cgi_R_RegisterShaderNoMip(sb_nums[i]);
		cgs.media.smallnumberShaders[i] = cgi_R_RegisterShaderNoMip(sb_t_nums[i]);
		cgs.media.chunkyNumberShaders[i] = cgi_R_RegisterShaderNoMip(sb_c_nums[i]);
	}

	cgs.media.radarShader = cgi_R_RegisterShaderNoMip("gfx/menus/radar/radar.png");
	cgs.media.radarScanShader = cgi_R_RegisterShaderNoMip("gfx/hud/Radarscanline");
	cgs.media.siegeItemShader = cgi_R_RegisterShaderNoMip("gfx/menus/radar/goalitem");
	cgs.media.mAutomapPlayerIcon = cgi_R_RegisterShaderNoMip("gfx/menus/radar/arrow_w");
	cgs.media.mAutomapRocketIcon = cgi_R_RegisterShaderNoMip("gfx/menus/radar/rocket");

	// FIXME: conditionally do this??  Something must be wrong with inventory item caching..?
	cgi_R_RegisterModel("models/items/remote.md3");

	cgs.media.explosionModel = cgi_R_RegisterModel("models/weaphits/explosion.md3");
	cgs.media.surfaceExplosionShader = cgi_R_RegisterShader("surfaceExplosion");

	cgs.media.halfShieldModel = cgi_R_RegisterModel("models/weaphits/testboom.md3");
	cgs.media.halfShieldShader = cgi_R_RegisterModel("halfShieldShell");

	cgs.media.solidWhiteShader = cgi_R_RegisterShader("gfx/effects/solidWhite");
	cgs.media.refractShader = cgi_R_RegisterShader("effects/refraction");

	//on players
	cgs.media.personalShieldShader = cgi_R_RegisterShader("gfx/misc/personalshield");
	cgs.media.cloakedShader = cgi_R_RegisterShader("gfx/effects/cloakedShader");
	cgi_R_RegisterShader("gfx/misc/ion_shield");

	cgs.media.boltShader = cgi_R_RegisterShader("gfx/misc/blueLine");

	cgs.media.shockShader = cgi_R_RegisterShader("gfx/misc/shockLine");

	// FIXME: do these conditionally
	cgi_R_RegisterShader("gfx/2d/workingCamera");
	cgi_R_RegisterShader("gfx/2d/brokenCamera");
	cgi_R_RegisterShader("gfx/effects/irid_shield");
	// for galak, but he doesn't have his own weapon so I can't register the shader there.
	cgi_R_RegisterShader("gfx/effects/barrier_shield");

	cgi_R_RegisterShader("gfx/effects/yellowflash");
	cgi_R_RegisterShader("gfx/effects/yellowline");

	//interface
	for (i = 0; i < NUM_CROSSHAIRS; i++)
	{
		cgs.media.crosshairShader[i] = cgi_R_RegisterShaderNoMip(va("gfx/2d/crosshair%c", 'a' + i));
	}
	cgs.media.backTileShader = cgi_R_RegisterShader("gfx/2d/backtile");

	//cgs.media.jedispawn = cgi_R_RegisterShaderNoMip("mp/spawn");

	cgs.media.weaponIconBackground = cgi_R_RegisterShaderNoMip("gfx/hud/background");
	cgs.media.weaponProngsOn = cgi_R_RegisterShaderNoMip("gfx/hud/prong_on_w");
	cgs.media.weaponProngsOff = cgi_R_RegisterShaderNoMip("gfx/hud/prong_off");
	cgs.media.forceProngsOn = cgi_R_RegisterShaderNoMip("gfx/hud/prong_on_f");
	cgs.media.forceIconBackground = cgi_R_RegisterShaderNoMip("gfx/hud/background_f");
	cgs.media.inventoryIconBackground = cgi_R_RegisterShaderNoMip("gfx/hud/background_i");
	cgs.media.inventoryProngsOn = cgi_R_RegisterShaderNoMip("gfx/hud/prong_on_i");
	cgs.media.dataPadFrame = cgi_R_RegisterShaderNoMip("gfx/hud/datapad2");
	//cgs.media.dataPadFrame			= cgi_R_RegisterShaderNoMip( "gfx/menus/datapad");

	//gore decal shaders -rww
	cgs.media.bdecal_bodyburn1 = cgi_R_RegisterShader("gfx/damage/bodyburnmark1");
	cgs.media.bdecal_burnmark1 = cgi_R_RegisterShader("gfx/damage/burnmark1");
	cgs.media.bdecal_saberglowmark = cgi_R_RegisterShader("gfx/damage/saberglowmark");
	cgs.media.bdecal_saberglow = cgi_R_RegisterShader("gfx/effects/saberDamageGlow");

	cg.loadLCARSStage = 5;
	CG_LoadingString("game media models");

	// Chunk models
	//FIXME: jfm:? bother to conditionally load these if an ent has this material type?
	for (i = 0; i < NUM_CHUNK_MODELS; i++)
	{
		cgs.media.chunkModels[CHUNK_METAL2][i] =
			cgi_R_RegisterModel(va("models/chunks/metal/metal1_%i.md3", i + 1));
		//_ /switched\ _
		cgs.media.chunkModels[CHUNK_METAL1][i] =
			cgi_R_RegisterModel(va("models/chunks/metal/metal2_%i.md3", i + 1));
		//  \switched/
		cgs.media.chunkModels[CHUNK_ROCK1][i] = cgi_R_RegisterModel(va("models/chunks/rock/rock1_%i.md3", i + 1));
		cgs.media.chunkModels[CHUNK_ROCK2][i] = cgi_R_RegisterModel(va("models/chunks/rock/rock2_%i.md3", i + 1));
		cgs.media.chunkModels[CHUNK_ROCK3][i] = cgi_R_RegisterModel(va("models/chunks/rock/rock3_%i.md3", i + 1));
		cgs.media.chunkModels[CHUNK_CRATE1][i] =
			cgi_R_RegisterModel(va("models/chunks/crate/crate1_%i.md3", i + 1));
		cgs.media.chunkModels[CHUNK_CRATE2][i] =
			cgi_R_RegisterModel(va("models/chunks/crate/crate2_%i.md3", i + 1));
		cgs.media.chunkModels[CHUNK_WHITE_METAL][i] = cgi_R_RegisterModel(
			va("models/chunks/metal/wmetal1_%i.md3", i + 1));
	}

	cgs.media.chunkSound = cgi_S_RegisterSound("sound/weapons/explosions/glasslcar");
	cgs.media.grateSound = cgi_S_RegisterSound("sound/effects/grate_destroy");
	cgs.media.rockBreakSound = cgi_S_RegisterSound("sound/effects/wall_smash");
	cgs.media.rockBounceSound[0] = cgi_S_RegisterSound("sound/effects/stone_bounce");
	cgs.media.rockBounceSound[1] = cgi_S_RegisterSound("sound/effects/stone_bounce2");
	cgs.media.metalBounceSound[0] = cgi_S_RegisterSound("sound/effects/metal_bounce");
	cgs.media.metalBounceSound[1] = cgi_S_RegisterSound("sound/effects/metal_bounce2");
	cgs.media.glassChunkSound = cgi_S_RegisterSound("sound/weapons/explosions/glassbreak1");
	cgs.media.crateBreakSound[0] = cgi_S_RegisterSound("sound/weapons/explosions/crateBust1");
	cgs.media.crateBreakSound[1] = cgi_S_RegisterSound("sound/weapons/explosions/crateBust2");

	cgs.media.weaponbox = cgi_R_RegisterShaderNoMip("gfx/interface/weapon_box");

	//Models & Shaders
	cgs.media.damageBlendBlobShader = cgi_R_RegisterShader("gfx/misc/borgeyeflare");

	cgs.media.HUDblockpoint1 = cgi_R_RegisterShaderNoMip("gfx/hud/blockpoint1");
	cgs.media.HUDblockpoint2 = cgi_R_RegisterShaderNoMip("gfx/hud/blockpoint2");

	cgs.media.HUDblockpointMB1 = cgi_R_RegisterShaderNoMip("gfx/hud/blockpointMB1");
	cgs.media.HUDblockpointMB2 = cgi_R_RegisterShaderNoMip("gfx/hud/blockpointMB2");

	// Datapad stuff from JKO
	cgs.media.HUDRightFrame = cgi_R_RegisterShaderNoMip("gfx/hud/hudrightframe");
	cgs.media.HUDInnerRight = cgi_R_RegisterShaderNoMip("gfx/hud/hudright_innerframe");
	cgs.media.HUDLeftFrame = cgi_R_RegisterShaderNoMip("gfx/hud/hudleftframe");
	cgs.media.HUDInnerLeft = cgi_R_RegisterShaderNoMip("gfx/hud/hudleft_innerframe");
	cgs.media.HUDHealth = cgi_R_RegisterShaderNoMip("gfx/hud/Datapadhealth");
	cgs.media.HUDHealthTic = cgi_R_RegisterShaderNoMip("gfx/hud/DataPadhealth_tic");
	cgs.media.HUDArmor1 = cgi_R_RegisterShaderNoMip("gfx/hud/Datapadarmor1");
	cgs.media.HUDArmor2 = cgi_R_RegisterShaderNoMip("gfx/hud/Datapadarmor2");
	cgs.media.HUDSaberStyleFast = cgi_R_RegisterShader("gfx/hud/DataPadsaber_stylesfast");
	cgs.media.HUDSaberStyleMed = cgi_R_RegisterShader("gfx/hud/DataPadsaber_stylesmed");
	cgs.media.HUDSaberStyleStrong = cgi_R_RegisterShader("gfx/hud/DataPadsaber_stylesstrong");
	cgs.media.HUDSaberStyleDesann = cgi_R_RegisterShader("gfx/hud/DataPadsaber_stylesDesann");
	cgs.media.HUDSaberStyleTavion = cgi_R_RegisterShader("gfx/hud/DataPadsaber_styletavion");
	cgs.media.HUDSaberStyleStaff = cgi_R_RegisterShader("gfx/hud/DataPadsaber_stylesStaff");
	cgs.media.HUDSaberStyleDuels = cgi_R_RegisterShader("gfx/hud/DataPadsaber_stylesDuels");

	//JK2 HUD ONLY

	cgs.media.JK2HUDArmor1 = cgi_R_RegisterShaderNoMip("gfx/hud/JK2armor1");
	cgs.media.JK2HUDArmor2 = cgi_R_RegisterShaderNoMip("gfx/hud/JK2armor2");

	cgs.media.JK2HUDHealth = cgi_R_RegisterShaderNoMip("gfx/hud/JK2health");
	cgs.media.JK2HUDHealthTic = cgi_R_RegisterShaderNoMip("gfx/hud/JK2health_tic");
	cgs.media.JK2HUDInnerLeft = cgi_R_RegisterShaderNoMip("gfx/hud/JK2hudleft_innerframe");
	cgs.media.JK2HUDLeftFrame = cgi_R_RegisterShaderNoMip("gfx/hud/JK2static_test");

	cgs.media.JK2HUDSaberStyleFast = cgi_R_RegisterShader("gfx/hud/JK2saber_stylesFast");
	cgs.media.JK2HUDSaberStyleMed = cgi_R_RegisterShader("gfx/hud/JK2saber_stylesMed");
	cgs.media.JK2HUDSaberStyleStrong = cgi_R_RegisterShader("gfx/hud/JK2saber_stylesStrong");
	cgs.media.JK2HUDSaberStyleDesann = cgi_R_RegisterShader("gfx/hud/JK2saber_stylesDesann");
	cgs.media.JK2HUDSaberStyleTavion = cgi_R_RegisterShader("gfx/hud/JK2saber_stylesTavion");
	cgs.media.JK2HUDSaberStyleStaff = cgi_R_RegisterShader("gfx/hud/JK2saber_stylesStaff");
	cgs.media.JK2HUDSaberStyleDuels = cgi_R_RegisterShader("gfx/hud/JK2saber_stylesDuals");

	cgs.media.JK2HUDRightFrame = cgi_R_RegisterShaderNoMip("gfx/hud/JK2hudrightframe");
	cgs.media.JK2HUDInnerRight = cgi_R_RegisterShaderNoMip("gfx/hud/JK2hudright_innerframe");

	cgs.media.SJEHUDRightFrame = cgi_R_RegisterShaderNoMip("gfx/hud/SJEHUDRightFrame");
	cgs.media.SJEHUDLeftFrame = cgi_R_RegisterShaderNoMip("gfx/hud/SJEHUDLeftFrame");

	for (i = 0; i < MAX_DATAPADTICS; i++)
	{
		forceTicPos[i].tic = cgi_R_RegisterShaderNoMip(forceTicPos[i].file);
		ammoTicPos[i].tic = cgi_R_RegisterShaderNoMip(ammoTicPos[i].file);
		JK2forceTicPos[i].tic = cgi_R_RegisterShaderNoMip(JK2forceTicPos[i].file);
		JK2ammoTicPos[i].tic = cgi_R_RegisterShaderNoMip(JK2ammoTicPos[i].file);
	}

	cg.loadLCARSStage = 6;

	cgs.media.messageLitOn = cgi_R_RegisterShaderNoMip("gfx/hud/message_on");
	cgs.media.messageLitOff = cgi_R_RegisterShaderNoMip("gfx/hud/message_off");
	cgs.media.messageObjCircle = cgi_R_RegisterShaderNoMip("gfx/hud/objective_circle");

	cgs.media.DPForcePowerOverlay = cgi_R_RegisterShader("gfx/hud/force_swirl");

	//NOTE: we should only cache this if there is a vehicle or emplaced gun somewhere on the map
	cgs.media.emplacedHealthBarShader = cgi_R_RegisterShaderNoMip("gfx/hud/health_frame");

	// battery charge shader when using a gonk
	cgs.media.batteryChargeShader = cgi_R_RegisterShader("gfx/2d/battery");
	cgi_R_RegisterShader("gfx/2d/droid_view");
	cgs.media.useableHint = cgi_R_RegisterShader("gfx/hud/useableHint");

	// Load up other HUD bits
	for (i = 0; i < OHB_MAX; i++)
	{
		cgi_UI_GetMenuItemInfo(
			otherHUDBits[i].menuName,
			otherHUDBits[i].itemName,
			&otherHUDBits[i].xPos,
			&otherHUDBits[i].yPos,
			&otherHUDBits[i].width,
			&otherHUDBits[i].height,
			otherHUDBits[i].color,
			&otherHUDBits[i].background);
	}

	// Get all the info for each HUD piece
	for (i = 0; i < MAX_HUD_TICS; i++)
	{
		cgi_UI_GetMenuItemInfo(
			healthTics[i].menuName,
			healthTics[i].itemName,
			&healthTics[i].xPos,
			&healthTics[i].yPos,
			&healthTics[i].width,
			&healthTics[i].height,
			healthTics[i].color,
			&healthTics[i].background);

		cgi_UI_GetMenuItemInfo(
			healthTics_md[i].menuName,
			healthTics_md[i].itemName,
			&healthTics_md[i].xPos,
			&healthTics_md[i].yPos,
			&healthTics_md[i].width,
			&healthTics_md[i].height,
			healthTics_md[i].color,
			&healthTics_md[i].background);

		cgi_UI_GetMenuItemInfo(
			armorTics[i].menuName,
			armorTics[i].itemName,
			&armorTics[i].xPos,
			&armorTics[i].yPos,
			&armorTics[i].width,
			&armorTics[i].height,
			armorTics[i].color,
			&armorTics[i].background);

		cgi_UI_GetMenuItemInfo(
			armorTics_md[i].menuName,
			armorTics_md[i].itemName,
			&armorTics_md[i].xPos,
			&armorTics_md[i].yPos,
			&armorTics_md[i].width,
			&armorTics_md[i].height,
			armorTics_md[i].color,
			&armorTics_md[i].background);

		cgi_UI_GetMenuItemInfo(
			forceTics[i].menuName,
			forceTics[i].itemName,
			&forceTics[i].xPos,
			&forceTics[i].yPos,
			&forceTics[i].width,
			&forceTics[i].height,
			forceTics[i].color,
			&forceTics[i].background);

		cgi_UI_GetMenuItemInfo(
			forceTics_md[i].menuName,
			forceTics_md[i].itemName,
			&forceTics_md[i].xPos,
			&forceTics_md[i].yPos,
			&forceTics_md[i].width,
			&forceTics_md[i].height,
			forceTics_md[i].color,
			&forceTics_md[i].background);

		cgi_UI_GetMenuItemInfo(
			blockpoint_tics[i].menuName,
			blockpoint_tics[i].itemName,
			&blockpoint_tics[i].xPos,
			&blockpoint_tics[i].yPos,
			&blockpoint_tics[i].width,
			&blockpoint_tics[i].height,
			blockpoint_tics[i].color,
			&blockpoint_tics[i].background);

		cgi_UI_GetMenuItemInfo(
			ammoTics[i].menuName,
			ammoTics[i].itemName,
			&ammoTics[i].xPos,
			&ammoTics[i].yPos,
			&ammoTics[i].width,
			&ammoTics[i].height,
			ammoTics[i].color,
			&ammoTics[i].background);

		cgi_UI_GetMenuItemInfo(
			ammoTics_md[i].menuName,
			ammoTics_md[i].itemName,
			&ammoTics_md[i].xPos,
			&ammoTics_md[i].yPos,
			&ammoTics_md[i].width,
			&ammoTics_md[i].height,
			ammoTics_md[i].color,
			&ammoTics_md[i].background);

		cgi_UI_GetMenuItemInfo(
			mishapTics[i].menuName,
			mishapTics[i].itemName,
			&mishapTics[i].xPos,
			&mishapTics[i].yPos,
			&mishapTics[i].width,
			&mishapTics[i].height,
			mishapTics[i].color,
			&mishapTics[i].background);

		cgi_UI_GetMenuItemInfo(
			classicmishapTics[i].menuName,
			classicmishapTics[i].itemName,
			&classicmishapTics[i].xPos,
			&classicmishapTics[i].yPos,
			&classicmishapTics[i].width,
			&classicmishapTics[i].height,
			classicmishapTics[i].color,
			&classicmishapTics[i].background);
	}

	for (i = 0; i < MAX_DFHUDTICS; i++)
	{
		// new df hud
		cgi_UI_GetMenuItemInfo(
			df_health_tics[i].menuName,
			df_health_tics[i].itemName,
			&df_health_tics[i].xPos,
			&df_health_tics[i].yPos,
			&df_health_tics[i].width,
			&df_health_tics[i].height,
			df_health_tics[i].color,
			&df_health_tics[i].background);

		// new df hud
		cgi_UI_GetMenuItemInfo(
			df_armorTics[i].menuName,
			df_armorTics[i].itemName,
			&df_armorTics[i].xPos,
			&df_armorTics[i].yPos,
			&df_armorTics[i].width,
			&df_armorTics[i].height,
			df_armorTics[i].color,
			&df_armorTics[i].background);

		// new df hud
		cgi_UI_GetMenuItemInfo(
			df_forceTics[i].menuName,
			df_forceTics[i].itemName,
			&df_forceTics[i].xPos,
			&df_forceTics[i].yPos,
			&df_forceTics[i].width,
			&df_forceTics[i].height,
			df_forceTics[i].color,
			&df_forceTics[i].background);

		// new df hud
		cgi_UI_GetMenuItemInfo(
			df_blockTics[i].menuName,
			df_blockTics[i].itemName,
			&df_blockTics[i].xPos,
			&df_blockTics[i].yPos,
			&df_blockTics[i].width,
			&df_blockTics[i].height,
			df_blockTics[i].color,
			&df_blockTics[i].background);

		// new df hud
		cgi_UI_GetMenuItemInfo(
			df_staminaTics[i].menuName,
			df_staminaTics[i].itemName,
			&df_staminaTics[i].xPos,
			&df_staminaTics[i].yPos,
			&df_staminaTics[i].width,
			&df_staminaTics[i].height,
			df_staminaTics[i].color,
			&df_staminaTics[i].background);

		cgi_UI_GetMenuItemInfo(
			df_fuelTics[i].menuName,
			df_fuelTics[i].itemName,
			&df_fuelTics[i].xPos,
			&df_fuelTics[i].yPos,
			&df_fuelTics[i].width,
			&df_fuelTics[i].height,
			df_fuelTics[i].color,
			&df_fuelTics[i].background);

		cgi_UI_GetMenuItemInfo(
			df_cloakTics[i].menuName,
			df_cloakTics[i].itemName,
			&df_cloakTics[i].xPos,
			&df_cloakTics[i].yPos,
			&df_cloakTics[i].width,
			&df_cloakTics[i].height,
			df_cloakTics[i].color,
			&df_cloakTics[i].background);

		cgi_UI_GetMenuItemInfo(
			df_barrierTics[i].menuName,
			df_barrierTics[i].itemName,
			&df_barrierTics[i].xPos,
			&df_barrierTics[i].yPos,
			&df_barrierTics[i].width,
			&df_barrierTics[i].height,
			df_barrierTics[i].color,
			&df_barrierTics[i].background);

		// new df hud
		cgi_UI_GetMenuItemInfo(
			df_ammoTics[i].menuName,
			df_ammoTics[i].itemName,
			&df_ammoTics[i].xPos,
			&df_ammoTics[i].yPos,
			&df_ammoTics[i].width,
			&df_ammoTics[i].height,
			df_ammoTics[i].color,
			&df_ammoTics[i].background);
	}

	for (i = 0; i < MAX_SJEHUD_TICS; i++)
	{
		cgi_UI_GetMenuItemInfo(
			sabfatticS[i].menuName,
			sabfatticS[i].itemName,
			&sabfatticS[i].xPos,
			&sabfatticS[i].yPos,
			&sabfatticS[i].width,
			&sabfatticS[i].height,
			sabfatticS[i].color,
			&sabfatticS[i].background);

		cgi_UI_GetMenuItemInfo(
			classicsabfatticS[i].menuName,
			classicsabfatticS[i].itemName,
			&classicsabfatticS[i].xPos,
			&classicsabfatticS[i].yPos,
			&classicsabfatticS[i].width,
			&classicsabfatticS[i].height,
			classicsabfatticS[i].color,
			&classicsabfatticS[i].background);

		cgi_UI_GetMenuItemInfo(
			newsabfatticS[i].menuName,
			newsabfatticS[i].itemName,
			&newsabfatticS[i].xPos,
			&newsabfatticS[i].yPos,
			&newsabfatticS[i].width,
			&newsabfatticS[i].height,
			newsabfatticS[i].color,
			&newsabfatticS[i].background);
	}

	memset(cg_items, 0, sizeof cg_items);
	memset(cg_weapons, 0, sizeof cg_weapons);

	// only register the items that the server says we need
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof items);

	for (i = 1; i < bg_numItems; i++)
	{
		if (items[i] == '1')
		{
			if (bg_itemlist[i].classname)
			{
				CG_LoadingString(bg_itemlist[i].classname);
				CG_RegisterItemVisuals(i);
			}
		}
		if (bg_itemlist[i].giType == IT_HOLDABLE)
		{
			if (bg_itemlist[i].giTag < INV_MAX)
			{
				inv_icons[bg_itemlist[i].giTag] = cgi_R_RegisterShaderNoMip(bg_itemlist[i].icon);
			}
		}
	}

	cgs.media.rageRecShader = cgi_R_RegisterShaderNoMip("gfx/mp/f_icon_ragerec");
	//etc.
	cgi_R_RegisterShader("gfx/misc/test_crackle"); //CG_DoGlassQuad

	// wall marks
	cgs.media.scavMarkShader = cgi_R_RegisterShader("gfx/damage/burnmark4");
	cgs.media.rivetMarkShader = cgi_R_RegisterShader("gfx/damage/rivetmark");

	// doing one shader just makes it look like a shell.  By using two shaders with different bulge offsets and different texture scales, it has a much more chaotic look
	cgs.media.electricBodyShader = cgi_R_RegisterShader("gfx/misc/electric");
	cgs.media.electricBody2Shader = cgi_R_RegisterShader("gfx/misc/fullbodyelectric2");

	cgs.media.shockBodyShader = cgi_R_RegisterShader("gfx/misc/shockbody");
	cgs.media.shockBody2Shader = cgi_R_RegisterShader("gfx/misc/fullshockbody");

	cgs.media.shadowMarkShader = cgi_R_RegisterShader("markShadow");
	cgs.media.wakeMarkShader = cgi_R_RegisterShader("wake");
	cgs.media.fsrMarkShader = cgi_R_RegisterShader("footstep_r");
	cgs.media.fslMarkShader = cgi_R_RegisterShader("footstep_l");
	cgs.media.fshrMarkShader = cgi_R_RegisterShader("footstep_heavy_r");
	cgs.media.fshlMarkShader = cgi_R_RegisterShader("footstep_heavy_l");
	cgi_S_RegisterSound("sound/effects/energy_crackle.wav");

	CG_LoadingString("map brushes");
	// register the inline models
	int breakPoint = cgs.numInlineModels = cgi_CM_NumInlineModels();

	assert(static_cast<size_t>(cgs.numInlineModels) < sizeof cgs.inlineDrawModel / sizeof cgs.inlineDrawModel[0]);

	for (i = 1; i < cgs.numInlineModels; i++)
	{
		char name[10];
		vec3_t mins, maxs;

		Com_sprintf(name, sizeof name, "*%i", i);
		cgs.inlineDrawModel[i] = cgi_R_RegisterModel(name);

		if (!cgs.inlineDrawModel[i])
		{
			breakPoint = i;
			break;
		}

		cgi_R_ModelBounds(cgs.inlineDrawModel[i], mins, maxs);
		for (int j = 0; j < 3; j++)
		{
			cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5 * (maxs[j] - mins[j]);
		}
	}

	cg.loadLCARSStage = 7;
	CG_LoadingString("map models");
	// register all the server specified models
	for (i = 1; i < MAX_MODELS; i++)
	{
		const char* modelName = CG_ConfigString(CS_MODELS + i);
		if (!modelName[0])
		{
			break;
		}
		cgs.model_draw[i] = cgi_R_RegisterModel(modelName);
		//		OutputDebugString(va("### CG_RegisterGraphics(): cgs.model_draw[%d] = \"%s\"\n",i,modelName));
	}

	cg.loadLCARSStage = 8;

	/*
	Ghoul2 Insert Start
	*/
	CG_LoadingString("skins");
	// register all the server specified models
	for (i = 1; i < MAX_CHARSKINS; i++)
	{
		const char* modelName = CG_ConfigString(CS_CHARSKINS + i);
		if (!modelName[0])
		{
			break;
		}
		cgs.skins[i] = cgi_R_RegisterSkin(modelName);
	}

	/*
	Ghoul2 Insert End
	*/

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		//feedback( va("client %i", i ) );
		CG_NewClientinfo(i);
	}

	for (i = 0; i < ENTITYNUM_WORLD; i++)
	{
		if (g_entities[i].inuse)
		{
			if (g_entities[i].client)
			{
				//if(!g_entities[i].client->clientInfo.infoValid)
				//We presume this
				{
					CG_LoadingString(va("client %s", g_entities[i].client->clientInfo.name));
					CG_RegisterClientModels(i);
					if (i != 0)
					{
						//Client weapons already precached
						CG_RegisterWeapon(g_entities[i].client->ps.weapon);
						if (g_entities[i].client->ps.saber[0].g2MarksShader[0])
						{
							cgi_R_RegisterShader(g_entities[i].client->ps.saber[0].g2MarksShader);
						}
						if (g_entities[i].client->ps.saber[0].g2MarksShader2[0])
						{
							cgi_R_RegisterShader(g_entities[i].client->ps.saber[0].g2MarksShader2);
						}
						if (g_entities[i].client->ps.saber[0].g2WeaponMarkShader[0])
						{
							cgi_R_RegisterShader(g_entities[i].client->ps.saber[0].g2WeaponMarkShader);
						}
						if (g_entities[i].client->ps.saber[0].g2WeaponMarkShader2[0])
						{
							cgi_R_RegisterShader(g_entities[i].client->ps.saber[0].g2WeaponMarkShader2);
						}
						if (g_entities[i].client->ps.saber[1].g2MarksShader[0])
						{
							cgi_R_RegisterShader(g_entities[i].client->ps.saber[1].g2MarksShader);
						}
						if (g_entities[i].client->ps.saber[1].g2MarksShader2[0])
						{
							cgi_R_RegisterShader(g_entities[i].client->ps.saber[1].g2MarksShader2);
						}
						if (g_entities[i].client->ps.saber[1].g2WeaponMarkShader[0])
						{
							cgi_R_RegisterShader(g_entities[i].client->ps.saber[1].g2WeaponMarkShader);
						}
						if (g_entities[i].client->ps.saber[1].g2WeaponMarkShader2[0])
						{
							cgi_R_RegisterShader(g_entities[i].client->ps.saber[1].g2WeaponMarkShader2);
						}
						CG_RegisterNPCCustomSounds(&g_entities[i].client->clientInfo);
					}
				}
			}
			else if (g_entities[i].svFlags & SVF_NPC_PRECACHE && g_entities[i].NPC_type && g_entities[i].NPC_type[
				0])
			{
				//Precache the NPC_type
				//FIXME: make sure we didn't precache this NPC_type already
				CG_LoadingString(va("NPC %s", g_entities[i].NPC_type));
				/*
				if (g_entities[i].classname && g_entities[i].classname[0] && Q_stricmp( g_entities[i].classname, "NPC_Vehicle" ) == 0)
				{
					// Get The Index, And Make Sure To Register All The Skins
					int iVehIndex = BG_VehicleGetIndex( g_entities[i].NPC_type );
				}
				else
				*/
				{
					CG_NPC_Precache(&g_entities[i]);
				}
			}
		}
	}

	// Preregister all of the weapons that were not already
	// registered to avoid lag when using cheats like "give all".
	for (i = 0; i < WP_NUM_WEAPONS; i++)
	{
		if (CG_IsWeaponUsablePlayer(i))
		{
			CG_RegisterWeapon(i);
		}
	}

	CG_LoadingString("static models");
	CG_CreateMiscEnts();

	cg.loadLCARSStage = 9;

	NPCsPrecached = qtrue;

	extern cvar_t* com_buildScript;

	if (com_buildScript->integer)
	{
		cgi_R_RegisterShader("gfx/misc/nav_cpoint");
		cgi_R_RegisterShader("gfx/misc/nav_line");
		cgi_R_RegisterShader("gfx/misc/nav_arrow");
		cgi_R_RegisterShader("gfx/misc/nav_node");
	}

	for (i = 1; i < MAX_SUB_BSP; i++)
	{
		vec3_t mins, maxs;
		int j;

		const char* bspName = CG_ConfigString(CS_BSP_MODELS + i);
		if (!bspName[0])
		{
			break;
		}
		CG_LoadingString("BSP instances");

		cgi_CM_LoadMap(bspName, qtrue);
		cgs.inlineDrawModel[breakPoint] = cgi_R_RegisterModel(bspName);
		cgi_R_ModelBounds(cgs.inlineDrawModel[breakPoint], mins, maxs);
		for (j = 0; j < 3; j++)
		{
			cgs.inlineModelMidpoints[breakPoint][j] = mins[j] + 0.5 * (maxs[j] - mins[j]);
		}
		breakPoint++;
		for (int sub = 1; sub < MAX_MODELS; sub++)
		{
			char temp[MAX_QPATH];
			Com_sprintf(temp, MAX_QPATH, "*%d-%d", i, sub);
			cgs.inlineDrawModel[breakPoint] = cgi_R_RegisterModel(temp);
			if (!cgs.inlineDrawModel[breakPoint])
			{
				break;
			}
			cgi_R_ModelBounds(cgs.inlineDrawModel[breakPoint], mins, maxs);
			for (j = 0; j < 3; j++)
			{
				cgs.inlineModelMidpoints[breakPoint][j] = mins[j] + 0.5 * (maxs[j] - mins[j]);
			}
			breakPoint++;
		}
	}

	for (i = 1; i < MAX_TERRAINS; i++)
	{
		const char* terrainInfo = CG_ConfigString(CS_TERRAINS + i);
		if (!terrainInfo[0])
		{
			break;
		}
		CG_LoadingString("Creating terrain");

		const int terrainID = cgi_CM_RegisterTerrain(terrainInfo);

		cgi_RMG_Init(terrainID, terrainInfo);

		// Send off the terrainInfo to the renderer
		cgi_RE_InitRendererTerrain(terrainInfo);
	}

	for (i = 1; i < MAX_ICONS; i++)
	{
		const char* iconName = const_cast<char*>(CG_ConfigString(CS_ICONS + i));

		if (!iconName[0])
		{
			break;
		}

		cgs.media.radarIcons[i] = cgi_R_RegisterShaderNoMip(iconName);
	}
}

//===========================================================================

/*
=================
CG_ConfigString
=================
*/
const char* CG_ConfigString(const int index)
{
	if (index < 0 || index >= MAX_CONFIGSTRINGS)
	{
		CG_Error("CG_ConfigString: bad index: %i", index);
	}
	return cgs.gameState.stringData + cgs.gameState.stringOffsets[index];
}

//==================================================================

void CG_LinkCentsToGents()
{
	for (int i = 0; i < MAX_GENTITIES; i++)
	{
		cg_entities[i].gent = &g_entities[i];
	}
}

/*
======================
CG_StartMusic

======================
*/
void CG_StartMusic(const qboolean bForceStart)
{
	const char* s;
	char parm1[MAX_QPATH], parm2[MAX_QPATH];

	// start the background music
	s = const_cast<char*>(CG_ConfigString(CS_MUSIC));
	COM_BeginParseSession();
	Q_strncpyz(parm1, COM_Parse(&s), sizeof parm1);
	Q_strncpyz(parm2, COM_Parse(&s), sizeof parm2);
	COM_EndParseSession();

	cgi_S_StartBackgroundTrack(parm1, parm2, static_cast<qboolean>(!bForceStart));
}

/*
======================
CG_GameStateReceived

Displays the info screen while loading media
======================
*/

int iCGResetCount = 0;
qboolean qbVidRestartOccured = qfalse;

//===================
qboolean gbUseTheseValuesFromLoadSave = qfalse; // MUST default to this
int gi_cg_forcepowerSelect;
int gi_cg_inventorySelect;
//===================

static void CG_GameStateReceived()
{
	// clear everything

	extern void CG_ClearAnimEvtCache();
	CG_ClearAnimEvtCache(); // else sound handles wrong after vid_restart

	qbVidRestartOccured = qtrue;
	iCGResetCount++;
	if (iCGResetCount == 1) // this will only equal 1 first time, after each vid_restart it just gets higher.
	{
		//	This non-clear is so the user can vid_restart during scrolling text without losing it.
		qbVidRestartOccured = qfalse;
	}

	if (!qbVidRestartOccured)
	{
		CG_Init_CG();
		cg.weaponSelect = WP_NONE;
		cg.forcepowerSelect = FP_HEAL;
	}

	memset(cg_weapons, 0, sizeof cg_weapons);
	memset(cg_items, 0, sizeof cg_items);

	CG_LinkCentsToGents();

	if (gbUseTheseValuesFromLoadSave)
	{
		gbUseTheseValuesFromLoadSave = qfalse; // ack
		cg.forcepowerSelect = gi_cg_forcepowerSelect;
		cg.inventorySelect = gi_cg_inventorySelect;
	}

	// get the rendering configuration from the client system
	cgi_GetGlconfig(&cgs.glconfig);

	// get the gamestate from the client system
	cgi_GetGameState(&cgs.gameState);

	CG_ParseServerinfo();

	// load the new map
	cgs.media.levelLoad = cgi_R_RegisterShaderNoMip("gfx/hud/mp_levelload");
	CG_LoadingString("collision map");

	cgi_CM_LoadMap(cgs.mapname, qfalse);

	CG_RegisterSounds();

	CG_RegisterGraphics();

	//jfm: moved down to preinit
	//	CG_InitLocalEntities();
	//	CG_InitMarkPolys();

	CG_LoadingString("music");
	CG_StartMusic(qfalse);

	// remove the last loading update
	cg.infoScreenText[0] = 0;

	CGCam_Init();

	CG_ClearLightStyles();

	// Okay so this doesn't exactly belong here
	cg.messageLitActive = qfalse;
	cg.forceHUDActive = qtrue;
	cg.forceHUDTotalFlashTime = 0;
	cg.forceHUDNextFlashTime = 0;

	cg.mishapHUDActive = qtrue;
	cg.mishapHUDTotalFlashTime = 0;
	cg.mishapHUDNextFlashTime = 0;

	cg.blockHUDActive = qtrue;
	cg.blockHUDTotalFlashTime = 0;
	cg.blockHUDNextFlashTime = 0;
}

void CG_WriteTheEvilCGHackStuff()
{
	ojk::SavedGameHelper saved_game(
		gi.saved_game);

	saved_game.write_chunk<int32_t>(
		INT_ID('F', 'P', 'S', 'L'),
		cg.forcepowerSelect);

	saved_game.write_chunk<int32_t>(
		INT_ID('I', 'V', 'S', 'L'),
		cg.inventorySelect);
}

void CG_ReadTheEvilCGHackStuff()
{
	ojk::SavedGameHelper saved_game(
		gi.saved_game);

	saved_game.read_chunk<int32_t>(
		INT_ID('F', 'P', 'S', 'L'),
		gi_cg_forcepowerSelect);

	saved_game.read_chunk<int32_t>(
		INT_ID('I', 'V', 'S', 'L'),
		gi_cg_inventorySelect);

	gbUseTheseValuesFromLoadSave = qtrue;
}

/*
Ghoul2 Insert Start
*/

// initialise the cg_entities structure
void CG_Init_CG()
{
	memset(&cg, 0, sizeof cg);
}

constexpr auto MAX_MISC_ENTS = 2000;

using cgMiscEntData_t = struct cgMiscEntData_s
{
	char model[MAX_QPATH];
	qhandle_t hModel;
	vec3_t origin;
	vec3_t angles;
	vec3_t scale;
	float radius;
	float zOffset; //some models need a z offset for culling, because of stupid wrong model origins
};

static cgMiscEntData_t MiscEnts[MAX_MISC_ENTS]; //statically allocated for now.
static int NumMiscEnts = 0;

void CG_CreateMiscEntFromGent(const gentity_t* ent, const vec3_t scale, const float zOff)
{
	//store the model data
	if (NumMiscEnts == MAX_MISC_ENTS)
	{
		Com_Error(ERR_DROP, "Maximum misc_model_static reached (%d)\n", MAX_MISC_ENTS);
	}

	if (!ent || !ent->model || !ent->model[0])
	{
		Com_Error(ERR_DROP, "misc_model_static with no model.");
	}
	const size_t len = strlen(ent->model);
	if (len < 4 || Q_stricmp(&ent->model[len - 4], ".md3") != 0)
	{
		Com_Error(ERR_DROP, "misc_model_static model(%s) is not an md3.", ent->model);
	}
	cgMiscEntData_t* MiscEnt = &MiscEnts[NumMiscEnts++];
	memset(MiscEnt, 0, sizeof * MiscEnt);

	strcpy(MiscEnt->model, ent->model);
	VectorCopy(ent->s.angles, MiscEnt->angles);
	VectorCopy(scale, MiscEnt->scale);
	VectorCopy(ent->s.origin, MiscEnt->origin);
	MiscEnt->zOffset = zOff;
}

#define VectorScaleVector(a,b,c)		(((c)[0]=(a)[0]*(b)[0]),((c)[1]=(a)[1]*(b)[1]),((c)[2]=(a)[2]*(b)[2]))
//call on standard model load to spawn the queued stuff
void CG_CreateMiscEnts()
{
	for (int i = 0; i < NumMiscEnts; i++)
	{
		vec3_t maxs;
		vec3_t mins;
		cgMiscEntData_t* MiscEnt = &MiscEnts[i];

		MiscEnt->hModel = cgi_R_RegisterModel(MiscEnt->model);
		if (MiscEnt->hModel == 0)
		{
			Com_Error(ERR_DROP, "misc_model_static failed to load model '%s'", MiscEnt->model);
		}

		cgi_R_ModelBounds(MiscEnt->hModel, mins, maxs);

		VectorScaleVector(mins, MiscEnt->scale, mins);
		VectorScaleVector(maxs, MiscEnt->scale, maxs);
		MiscEnt->radius = DistanceSquared(mins, maxs);
	}
}

void CG_DrawMiscEnts()
{
	cgMiscEntData_t* MiscEnt = MiscEnts;
	refEntity_t ref_ent;
	vec3_t cullOrigin;

	memset(&ref_ent, 0, sizeof ref_ent);
	ref_ent.reType = RT_MODEL;
	ref_ent.frame = 0;
	ref_ent.renderfx = RF_LIGHTING_ORIGIN;
	for (int i = 0; i < NumMiscEnts; i++)
	{
		VectorCopy(MiscEnt->origin, cullOrigin);
		cullOrigin[2] += MiscEnt->zOffset + 1.0f;

		if (gi.inPVS(cg.refdef.vieworg, cullOrigin))
		{
			vec3_t difference;
			VectorSubtract(MiscEnt->origin, cg.refdef.vieworg, difference);
			if (VectorLengthSquared(difference) - MiscEnt->radius <= 8192 * 8192/*RMG_distancecull.value*/)
			{
				//fixme: need access to the real cull dist here
				ref_ent.hModel = MiscEnt->hModel;
				AnglesToAxis(MiscEnt->angles, ref_ent.axis);
				VectorCopy(MiscEnt->scale, ref_ent.modelScale);
				VectorCopy(MiscEnt->origin, ref_ent.origin);
				VectorCopy(cullOrigin, ref_ent.lightingOrigin);
				ScaleModelAxis(&ref_ent);
				cgi_R_AddRefEntityToScene(&ref_ent);
			}
		}
		MiscEnt++;
	}
}

void CG_TransitionPermanent()
{
	centity_t* cent = cg_entities;

	cg_numpermanents = 0;
	for (int i = 0; i < MAX_GENTITIES; i++, cent++)
	{
		if (cgi_GetDefaultState(i, &cent->currentState))
		{
			cent->nextState = &cent->currentState;
			VectorCopy(cent->currentState.origin, cent->lerpOrigin);
			VectorCopy(cent->currentState.angles, cent->lerpAngles);
			cent->currentValid = qtrue;

			cg_permanents[cg_numpermanents++] = cent;
		}
	}
}

/*
Ghoul2 Insert End
*/

/*
=================
CG_PreInit

Called when DLL loads (after subsystem restart, but before gamestate is received)
=================
*/
void CG_PreInit()
{
	CG_Init_CG();

	memset(&cgs, 0, sizeof cgs);
	iCGResetCount = 0;

	CG_RegisterCvars();

	//moved from CG_GameStateReceived because it's loaded sooner now
	CG_InitLocalEntities();

	CG_InitMarkPolys();
}

extern void R_LoadWeatherParms();
/*
=================
CG_Init

Called after every level change or subsystem restart
=================
*/
void CG_Init(const int serverCommandSequence)
{
	cgs.serverCommandSequence = serverCommandSequence;

	cgi_Cvar_Set("cg_drawHUD", "1");

	cgi_Cvar_Set("cg_setVaderBreath", "0");
	cgi_Cvar_Set("cg_setVaderBreathdamaged", "0");

	// fonts...
	//
	cgs.media.charsetShader = cgi_R_RegisterShaderNoMip("gfx/2d/charsgrid_med");

	cgs.media.qhFontSmall = cgi_R_RegisterFont("ocr_a");
	cgs.media.qhFontMedium = cgi_R_RegisterFont("ergoec");

	cgs.media.whiteShader = cgi_R_RegisterShader("white");
	cgs.media.loadTick = cgi_R_RegisterShaderNoMip("gfx/hud/load_tick");
	cgs.media.loadTickCap = cgi_R_RegisterShaderNoMip("gfx/hud/load_tick_cap");
	cgs.media.load_SerenitySaberSystems = cgi_R_RegisterShaderNoMip("gfx/hud/load_SerenitySaberSystems");

	const char* force_icon_files[NUM_FORCE_POWERS] =
	{
		//icons matching enums forcePowers_t
		"gfx/mp/f_icon_lt_heal", //FP_HEAL,
		"gfx/mp/f_icon_levitation", //FP_LEVITATION,
		"gfx/mp/f_icon_speed", //FP_SPEED,
		"gfx/mp/f_icon_push", //FP_PUSH,
		"gfx/mp/f_icon_pull", //FP_PULL,
		"gfx/mp/f_icon_lt_telepathy", //FP_TELEPATHY,
		"gfx/mp/f_icon_dk_grip", //FP_GRIP,
		"gfx/mp/f_icon_dk_l1", //FP_LIGHTNING,
		"gfx/mp/f_icon_saber_throw", //FP_SABERTHROW
		"gfx/mp/f_icon_saber_defend", //FP_SABERDEFEND,
		"gfx/mp/f_icon_saber_attack", //FP_SABERATTACK,
		"gfx/mp/f_icon_dk_rage", //FP_RAGE,
		"gfx/mp/f_icon_lt_protect", //FP_PROTECT,
		"gfx/mp/f_icon_lt_absorb", //FP_ABSORB,
		"gfx/mp/f_icon_dk_drain", //FP_DRAIN,
		"gfx/mp/f_icon_sight", //FP_SEE,
		"gfx/mp/f_icon_dk_destruction", //FP_DESTRUCTION,
		"gfx/mp/f_icon_lt_stasis", //FP_STASIS,
		"gfx/mp/f_icon_lt_grasp", //FP_GRASP
		"gfx/mp/f_icon_repulse", //FP_REPULSE
		"gfx/mp/f_icon_dk_lightning_strike", //FP_LIGHTNING_STRIKE
		"gfx/mp/f_icon_dk_fear", //FP_FEAR
		"gfx/mp/f_icon_dk_deadlysight", //FP_DEADLYSIGHT
		"gfx/mp/f_icon_lt_projection", //FP_PROJECTION
		"gfx/mp/f_icon_lt_blast", //FP_BLAST
	};

	// Precache inventory icons
	for (int i = 0; i < NUM_FORCE_POWERS; i++)
	{
		if (force_icon_files[i])
		{
			force_icons[i] = cgi_R_RegisterShaderNoMip(force_icon_files[i]);
		}
	}

	CG_LoadHudMenu(); // load new hud stuff

	cgi_UI_Menu_OpenByName("loadscreen");

	//rww - Moved from CG_GameStateReceived (we don't want to clear perm ents)
	memset(cg_entities, 0, sizeof cg_entities);

	CG_TransitionPermanent();

	cg.loadLCARSStage = 0;

	CG_GameStateReceived();

	CG_InitConsoleCommands();

	CG_TrueViewInit();

	cg.weaponPickupTextTime = 0;

	cg.missionInfoFlashTime = 0;

	cg.missionStatusShow = qfalse;

	cg.missionFailedScreen = qfalse; // Screen hasn't been opened.
	cgi_UI_MenuCloseAll(); // So the loadscreen menu will turn off just after the opening snapshot
}

/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
void CG_Shutdown()
{
	in_camera = false;
	FX_Free();
}

//// DEBUG STUFF
/*
-------------------------
CG_DrawNode
-------------------------
*/
void CG_DrawNode(vec3_t origin, const int type)
{
	localEntity_t* ex = CG_AllocLocalEntity();

	ex->leType = LE_SPRITE;
	ex->startTime = cg.time;
	ex->endTime = ex->startTime + 51;
	VectorCopy(origin, ex->refEntity.origin);

	ex->refEntity.customShader = cgi_R_RegisterShader("gfx/misc/nav_node");

	float scale = 16.0f;

	switch (type)
	{
	case NODE_NORMAL:
		ex->color[0] = 255;
		ex->color[1] = 255;
		ex->color[2] = 0;
		break;

	case NODE_FLOATING:
		ex->color[0] = 0;
		ex->color[1] = 255;
		ex->color[2] = 255;
		scale += 16.0f;
		break;

	case NODE_GOAL:
		ex->color[0] = 255;
		ex->color[1] = 0;
		ex->color[2] = 0;
		scale += 16.0f;
		break;

	case NODE_NAVGOAL:
		ex->color[0] = 0;
		ex->color[1] = 255;
		ex->color[2] = 0;
		break;
	default:;
	}

	ex->radius = scale;
}

/*
-------------------------
CG_DrawRadius
-------------------------
*/

void CG_DrawRadius(vec3_t origin, const unsigned int radius, const int type)
{
	localEntity_t* ex = CG_AllocLocalEntity();

	ex->leType = LE_QUAD;
	ex->radius = radius;
	ex->startTime = cg.time;
	ex->endTime = ex->startTime + 51;
	VectorCopy(origin, ex->refEntity.origin);

	ex->refEntity.customShader = cgi_R_RegisterShader("gfx/misc/nav_radius");

	switch (type)
	{
	case NODE_NORMAL:
		ex->color[0] = 255;
		ex->color[1] = 255;
		ex->color[2] = 0;
		break;

	case NODE_FLOATING:
		ex->color[0] = 0;
		ex->color[1] = 255;
		ex->color[2] = 255;
		break;

	case NODE_GOAL:
		ex->color[0] = 255;
		ex->color[1] = 0;
		ex->color[2] = 0;
		break;

	case NODE_NAVGOAL:
		ex->color[0] = 0;
		ex->color[1] = 255;
		ex->color[2] = 0;
		break;
	default:;
	}
}

/*
-------------------------
CG_DrawEdge
-------------------------
*/

void CG_DrawEdge(vec3_t start, vec3_t end, const int type)
{
	switch (type)
	{
		// NAVIGATION EDGES BETWEEN POINTS
		//=====================================
	case EDGE_NORMAL:
	{
		FX_AddLine(start, end, 4.0f, 0.5f, 0.5f, 51, cgi_R_RegisterShader("gfx/misc/nav_line"));
	}
	break;
	case EDGE_LARGE:
	{
		FX_AddLine(start, end, 15.0f, 0.5f, 0.5f, 51, cgi_R_RegisterShader("gfx/misc/nav_line"));
	}
	break;
	case EDGE_BLOCKED:
	{
		vec3_t color = { 255, 0, 0 }; // RED
		FX_AddLine(start, end, 4.0f, 0.5f, 0.5f, color, color, 51, cgi_R_RegisterShader("gfx/misc/nav_line"), 0);
	}
	break;
	case EDGE_FLY:
	{
		vec3_t color = { 0, 255, 255 }; // GREEN
		FX_AddLine(start, end, 4.0f, 0.5f, 0.5f, color, color, 51, cgi_R_RegisterShader("gfx/misc/nav_line"), 0);
	}
	break;
	case EDGE_JUMP:
	{
		vec3_t color = { 0, 0, 255 }; // BLUE
		FX_AddLine(start, end, 4.0f, 0.5f, 0.5f, color, color, 51, cgi_R_RegisterShader("gfx/misc/nav_line"), 0);
	}
	break;

	// EDGE NODES
	//=====================================
	case EDGE_NODE_NORMAL:
	{
		vec3_t color = { 155, 155, 155 };
		FX_AddLine(start, end, 1.0f, 1.0f, 1.0f, color, color, 151, cgi_R_RegisterShader("gfx/misc/whiteline2"), 0);
	}
	break;
	case EDGE_NODE_FLOATING:
	{
		vec3_t color = { 155, 155, 0 };
		FX_AddLine(start, end, 1.0f, 1.0f, 1.0f, color, color, 151, cgi_R_RegisterShader("gfx/misc/whiteline2"), 0);
	}
	break;
	case EDGE_NODE_GOAL:
	{
		vec3_t color = { 0, 0, 155 };
		FX_AddLine(start, end, 1.0f, 1.0f, 1.0f, color, color, 151, cgi_R_RegisterShader("gfx/misc/whiteline2"), 0);
	}
	break;
	case EDGE_NODE_COMBAT:
	{
		vec3_t color = { 155, 0, 0 };
		FX_AddLine(start, end, 1.0f, 1.0f, 1.0f, color, color, 151, cgi_R_RegisterShader("gfx/misc/whiteline2"), 0);
	}
	break;

	// NEAREST NAV
	//=====================================
	case EDGE_NEARESTVALID:
	{
		vec3_t color = { 155, 155, 155 }; // WHITE
		FX_AddLine(-1, start, end, 1.0f, 1.0f, 0, 1.0f, 1.0f, FX_ALPHA_LINEAR, color, color, 0, 51,
			cgi_R_RegisterShader("gfx/misc/whiteline2"), 0, 0);
	}
	break;

	case EDGE_NEARESTINVALID:
	{
		vec3_t color = { 155, 0, 0 }; // WHITE
		FX_AddLine(-1, start, end, 1.0f, 1.0f, 0, 1.0f, 1.0f, FX_ALPHA_LINEAR, color, color, 0, 51,
			cgi_R_RegisterShader("gfx/misc/whiteline2"), 0, 0);
	}
	break;

	// NEAREST NAV CELLS
	//=====================================
	case EDGE_CELL:
	{
		vec3_t color = { 155, 155, 155 }; // WHITE
		FX_AddLine(-1, start, end, 1.0f, 1.0f, 0, 1.0f, 1.0f, FX_ALPHA_LINEAR, color, color, 0, 51,
			cgi_R_RegisterShader("gfx/misc/whiteline2"), 0, 0);
	}
	break;
	case EDGE_CELL_EMPTY:
	{
		vec3_t color = { 255, 0, 0 }; // RED
		FX_AddLine(-1, start, end, 1.0f, 1.0f, 0, 1.0f, 1.0f, FX_ALPHA_LINEAR, color, color, 0, 51,
			cgi_R_RegisterShader("gfx/misc/whiteline2"), 0, 0);
	}
	break;

	// ACTOR PATHS
	//=============
	case EDGE_PATH:
	{
		vec3_t color = { 0, 0, 155 }; // WHITE
		FX_AddLine(start, end, 5.0f, 0.5f, 0.5f, color, color, 151, cgi_R_RegisterShader("gfx/misc/nav_arrow_new"), 0);
	}
	break;

	case EDGE_PATHBLOCKED:
	{
		vec3_t color = { 255, 0, 0 }; // RED
		FX_AddLine(start, end, 5.0f, 0.5f, 0.5f, color, color, 151, cgi_R_RegisterShader("gfx/misc/nav_arrow_new"), 0);
		break;
	}

	case EDGE_FOLLOWPOS:
	{
		vec3_t color = { 0, 255, 0 }; // GREEN
		FX_AddLine(start, end, 5.0f, 0.5f, 0.5f, color, color, 151, cgi_R_RegisterShader("gfx/misc/nav_arrow_new"), 0);
		break;
	}

	// STEERING
	//=====================================
	case EDGE_IMPACT_SAFE:
	{
		vec3_t color = { 155, 155, 155 }; // WHITE
		FX_AddLine(start, end, 1.0f, 1.0f, 1.0f, color, color, 151, cgi_R_RegisterShader("gfx/misc/whiteline2"), 0);
	}
	break;
	case EDGE_IMPACT_POSSIBLE:
	{
		vec3_t color = { 255, 0, 0 }; // RED
		FX_AddLine(start, end, 1.0f, 1.0f, 1.0f, color, color, 151, cgi_R_RegisterShader("gfx/misc/whiteline2"), 0);
	}
	break;
	case EDGE_VELOCITY:
	{
		vec3_t color = { 0, 255, 0 }; // GREEN
		FX_AddLine(start, end, 1.0f, 1.0f, 1.0f, color, color, 151, cgi_R_RegisterShader("gfx/misc/whiteline2"), 0);
	}
	break;
	case EDGE_THRUST:
	{
		vec3_t color = { 0, 0, 255 }; // BLUE
		FX_AddLine(start, end, 1.0f, 1.0f, 1.0f, color, color, 151, cgi_R_RegisterShader("gfx/misc/whiteline2"), 0);
	}
	break;

	// MISC Colored Lines
	//=====================================
	case EDGE_WHITE_ONESECOND:
	{
		vec3_t color = { 155, 155, 155 }; // WHITE
		FX_AddLine(start, end, 1.0f, 1.0f, 1.0f, color, color, 1051, cgi_R_RegisterShader("gfx/misc/whiteline2"), 0);
	}
	break;
	case EDGE_WHITE_TWOSECOND:
	{
		vec3_t color = { 155, 155, 155 }; // WHITE
		FX_AddLine(start, end, 1.0f, 1.0f, 1.0f, color, color, 1051, cgi_R_RegisterShader("gfx/misc/whiteline2"), 0);
	}
	break;
	case EDGE_RED_ONESECOND:
	{
		vec3_t color = { 255, 0, 0 }; // RED
		FX_AddLine(start, end, 1.0f, 1.0f, 1.0f, color, color, 2051, cgi_R_RegisterShader("gfx/misc/whiteline2"), 0);
	}
	break;
	case EDGE_RED_TWOSECOND:
	{
		vec3_t color = { 255, 0, 0 }; // RED
		FX_AddLine(start, end, 1.0f, 1.0f, 1.0f, color, color, 2051, cgi_R_RegisterShader("gfx/misc/whiteline2"), 0);
	}
	break;

	default:
		break;
	}
}

/*
-------------------------
CG_DrawCombatPoint
-------------------------
*/

void CG_DrawCombatPoint(vec3_t origin, int type)
{
	localEntity_t* ex = CG_AllocLocalEntity();

	ex->leType = LE_SPRITE;
	ex->startTime = cg.time;
	ex->radius = 8;
	ex->endTime = ex->startTime + 51;
	VectorCopy(origin, ex->refEntity.origin);

	ex->refEntity.customShader = cgi_R_RegisterShader("gfx/misc/nav_cpoint");

	ex->color[0] = 255;
	ex->color[1] = 0;
	ex->color[2] = 255;

	/*
		switch( type )
		{
		case 0:	//FIXME: To shut up the compiler warning (more will be added here later of course)
		default:
			FX_AddSprite( origin, NULL, NULL, 8.0f, 0.0f, 1.0f, 1.0f, color, color, 0.0f, 0.0f, 51, cgi_R_RegisterShader( "gfx/misc/nav_cpoint" ) );
			break;
		}
	*/
}

/*
-------------------------
CG_DrawAlert
-------------------------
*/

void CG_DrawAlert(vec3_t origin, const float rating)
{
	vec3_t drawPos;

	VectorCopy(origin, drawPos);
	drawPos[2] += 48;

	vec3_t startRGB{};

	//Fades from green at 0, to red at 1
	startRGB[0] = rating;
	startRGB[1] = 1 - rating;
	startRGB[2] = 0;

	FX_AddSprite(drawPos, nullptr, nullptr, 16, 1.0f, 1.0f, startRGB, startRGB, 0, 0, 50, cgs.media.whiteShader);
}

constexpr auto MAX_MENUDEFFILE = 4096;

//
// ==============================
// new hud stuff ( mission pack )
// ==============================
//
qboolean CG_Asset_Parse(const char** p)
{
	const char* tempStr;
	int pointSize;

	const char* token = COM_ParseExt(p, qtrue);

	if (!token)
	{
		return qfalse;
	}

	if (Q_stricmp(token, "{") != 0)
	{
		return qfalse;
	}

	while (true)
	{
		token = COM_ParseExt(p, qtrue);
		if (!token)
		{
			return qfalse;
		}

		if (Q_stricmp(token, "}") == 0)
		{
			return qtrue;
		}

		// font
		if (Q_stricmp(token, "font") == 0)
		{
			/*
						int pointSize;

						cgi_UI_Parse_String(tempStr);
						cgi_UI_Parse_Int(&pointSize);

						if (!tempStr || !pointSize)
						{
							return qfalse;
						}
			*/
			//			cgDC.registerFont(tempStr, pointSize, &cgDC.Assets.textFont);
			continue;
		}

		// smallFont
		if (Q_stricmp(token, "smallFont") == 0)
		{
			if (!COM_ParseString(p, &tempStr) || !COM_ParseInt(p, &pointSize))
			{
				return qfalse;
			}
			//			cgDC.registerFont(tempStr, pointSize, &cgDC.Assets.smallFont);
			continue;
		}

		// smallFont - because the HUD file needs it for MP.
		if (Q_stricmp(token, "small2Font") == 0)
		{
			if (!COM_ParseString(p, &tempStr) || !COM_ParseInt(p, &pointSize))
			{
				return qfalse;
			}
			//			cgDC.registerFont(tempStr, pointSize, &cgDC.Assets.smallFont);
			continue;
		}

		// font
		if (Q_stricmp(token, "bigfont") == 0)
		{
			int point_size;
			if (!COM_ParseString(p, &tempStr) || !COM_ParseInt(p, &point_size))
			{
				return qfalse;
			}
			//			cgDC.registerFont(tempStr, pointSize, &cgDC.Assets.bigFont);
			continue;
		}

		// gradientbar
		if (Q_stricmp(token, "gradientbar") == 0)
		{
			if (!COM_ParseString(p, &tempStr))
			{
				return qfalse;
			}
			//			cgDC.Assets.gradientBar = trap_R_RegisterShaderNoMip(tempStr);
			continue;
		}

		// enterMenuSound
		if (Q_stricmp(token, "menuEnterSound") == 0)
		{
			if (!COM_ParseString(p, &tempStr))
			{
				return qfalse;
			}
			//			cgDC.Assets.menuEnterSound = trap_S_RegisterSound( tempStr );
			continue;
		}

		// exitMenuSound
		if (Q_stricmp(token, "menuExitSound") == 0)
		{
			if (!COM_ParseString(p, &tempStr))
			{
				return qfalse;
			}
			//			cgDC.Assets.menuExitSound = trap_S_RegisterSound( tempStr );
			continue;
		}

		// itemFocusSound
		if (Q_stricmp(token, "itemFocusSound") == 0)
		{
			if (!COM_ParseString(p, &tempStr))
			{
				return qfalse;
			}
			//			cgDC.Assets.itemFocusSound = trap_S_RegisterSound( tempStr );
			continue;
		}

		// menuBuzzSound
		if (Q_stricmp(token, "menuBuzzSound") == 0)
		{
			if (!COM_ParseString(p, &tempStr))
			{
				return qfalse;
			}
			//			cgDC.Assets.menuBuzzSound = trap_S_RegisterSound( tempStr );
			continue;
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////

		if (Q_stricmp(token, "cursor") == 0)
		{
			//			if (!COM_ParseString(p, &cgDC.Assets.cursorStr))
			//			{
			//				return qfalse;
			//			}
			//			cgDC.Assets.cursor = trap_R_RegisterShaderNoMip( cgDC.Assets.cursorStr);
			continue;
		}

		if (Q_stricmp(token, "cursor_anakin") == 0)
		{
			//			if (!COM_ParseString(p, &cgDC.Assets.cursorStr))
			//			{
			//				return qfalse;
			//			}
			//			cgDC.Assets.cursor_anakin = trap_R_RegisterShaderNoMip( cgDC.Assets.cursorStr);
			continue;
		}

		if (Q_stricmp(token, "cursor_jk") == 0)
		{
			//			if (!COM_ParseString(p, &cgDC.Assets.cursorStr))
			//			{
			//				return qfalse;
			//			}
			//			cgDC.Assets.cursor_jk = trap_R_RegisterShaderNoMip( cgDC.Assets.cursorStr);
			continue;
		}

		if (Q_stricmp(token, "cursor_katarn") == 0)
		{
			//			if (!COM_ParseString(p, &cgDC.Assets.cursorStr))
			//			{
			//				return qfalse;
			//			}
			//			cgDC.Assets.cursor_katarn = trap_R_RegisterShaderNoMip( cgDC.Assets.cursorStr);
			continue;
		}

		if (Q_stricmp(token, "cursor_kylo") == 0)
		{
			//			if (!COM_ParseString(p, &cgDC.Assets.cursorStr))
			//			{
			//				return qfalse;
			//			}
			//			cgDC.Assets.cursor_kylo = trap_R_RegisterShaderNoMip( cgDC.Assets.cursorStr);
			continue;
		}

		if (Q_stricmp(token, "cursor_luke") == 0)
		{
			//			if (!COM_ParseString(p, &cgDC.Assets.cursorStr))
			//			{
			//				return qfalse;
			//			}
			//			cgDC.Assets.cursor_luke = trap_R_RegisterShaderNoMip( cgDC.Assets.cursorStr);
			continue;
		}

		if (Q_stricmp(token, "cursor_obiwan") == 0)
		{
			//			if (!COM_ParseString(p, &cgDC.Assets.cursorStr))
			//			{
			//				return qfalse;
			//			}
			//			cgDC.Assets.cursor_obiwan = trap_R_RegisterShaderNoMip( cgDC.Assets.cursorStr);
			continue;
		}

		if (Q_stricmp(token, "cursor_oldrepublic") == 0)
		{
			//			if (!COM_ParseString(p, &cgDC.Assets.cursorStr))
			//			{
			//				return qfalse;
			//			}
			//			cgDC.Assets.cursor_oldrepublic = trap_R_RegisterShaderNoMip( cgDC.Assets.cursorStr);
			continue;
		}

		if (Q_stricmp(token, "cursor_quigon") == 0)
		{
			//			if (!COM_ParseString(p, &cgDC.Assets.cursorStr))
			//			{
			//				return qfalse;
			//			}
			//			cgDC.Assets.cursor_quigon = trap_R_RegisterShaderNoMip( cgDC.Assets.cursorStr);
			continue;
		}

		if (Q_stricmp(token, "cursor_rey") == 0)
		{
			//			if (!COM_ParseString(p, &cgDC.Assets.cursorStr))
			//			{
			//				return qfalse;
			//			}
			//			cgDC.Assets.cursor_rey = trap_R_RegisterShaderNoMip( cgDC.Assets.cursorStr);
			continue;
		}

		if (Q_stricmp(token, "cursor_vader") == 0)
		{
			//			if (!COM_ParseString(p, &cgDC.Assets.cursorStr))
			//			{
			//				return qfalse;
			//			}
			//			cgDC.Assets.cursor_vader = trap_R_RegisterShaderNoMip( cgDC.Assets.cursorStr);
			continue;
		}

		if (Q_stricmp(token, "cursor_windu") == 0)
		{
			//			if (!COM_ParseString(p, &cgDC.Assets.cursorStr))
			//			{
			//				return qfalse;
			//			}
			//			cgDC.Assets.cursor_windu = trap_R_RegisterShaderNoMip( cgDC.Assets.cursorStr);
			continue;
		}

		///////////////////////////////////////////////////////////////////////////////////
		if (Q_stricmp(token, "fadeCycle") == 0)
		{
			//			if (!COM_ParseInt(p, &cgDC.Assets.fadeCycle))
			//			{
			//				return qfalse;
			//			}
			continue;
		}

		if (Q_stricmp(token, "fadeAmount") == 0)
		{
			//			if (!COM_ParseFloat(p, &cgDC.Assets.fadeAmount))
			//			{
			//				return qfalse;
			//			}
			continue;
		}

		if (Q_stricmp(token, "shadowX") == 0)
		{
			//			if (!COM_ParseFloat(p, &cgDC.Assets.shadowX))
			//			{
			//				return qfalse;
			//			}
			continue;
		}

		if (Q_stricmp(token, "shadowY") == 0)
		{
			//			if (!COM_ParseFloat(p, &cgDC.Assets.shadowY))
			//			{
			//				return qfalse;
			//			}
			continue;
		}

		if (Q_stricmp(token, "shadowColor") == 0)
		{
		}
	}
}

void cgi_UI_EndParseSession(char* buf);

/*
=================
CG_ParseMenu();
=================
*/
void CG_ParseMenu(const char* menuFile)
{
	char* token;
	char* buf;

	//Com_Printf("Parsing menu file:%s\n", menuFile);

	int result = cgi_UI_StartParseSession(const_cast<char*>(menuFile), &buf);

	if (!result)
	{
		Com_Printf("Unable to load hud menu file:%s. Using default ui/testhud.menu.\n", menuFile);
		result = cgi_UI_StartParseSession("ui/testhud.menu", &buf);
		if (!result)
		{
			Com_Printf("Unable to load default ui/testhud.menu.\n");
			cgi_UI_EndParseSession(buf);
			return;
		}
	}

	char* p = buf;
	while (true)
	{
		cgi_UI_ParseExt(&token);

		if (!*token) // All done?
		{
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//	Com_Printf( "Missing { in menu file\n" );
		//	break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//	Com_Printf( "Too many menus!\n" );
		//	break;
		//}

		//		if ( *token == '}' )
		//		{
		//			break;
		//		}

		if (Q_stricmp(token, "assetGlobalDef") == 0)
		{
			/*
			if (CG_Asset_Parse(handle))
			{
				continue;
			}
			else
			{
				break;
			}
			*/
		}

		if (Q_stricmp(token, "menudef") == 0)
		{
			// start a new menu
			cgi_UI_Menu_New(p);
		}
	}

	cgi_UI_EndParseSession(buf);
}

/*
=================
CG_Load_Menu();

=================
*/
qboolean CG_Load_Menu(const char** p)
{
	const char* token = COM_ParseExt(p, qtrue);

	if (token[0] != '{')
	{
		return qfalse;
	}

	while (true)
	{
		token = COM_ParseExt(p, qtrue);

		if (Q_stricmp(token, "}") == 0)
		{
			return qtrue;
		}

		if (!token || token[0] == 0)
		{
			return qfalse;
		}

		CG_ParseMenu(token);
	}
}

/*
=================
CG_LoadMenus();

=================
*/
void CG_LoadMenus(const char* menuFile)
{
	const char* p;
	fileHandle_t f;
	char buf[MAX_MENUDEFFILE];

	//start = cgi_Milliseconds();

	int len = cgi_FS_FOpenFile(menuFile, &f, FS_READ);
	if (!f)
	{
		if (Q_isanumber(menuFile)) // cg_hudFiles 1
			CG_Printf(S_COLOR_GREEN "hud menu file skipped, using default\n");
		else
			CG_Printf(S_COLOR_YELLOW "hud menu file not found: %s, using default\n", menuFile);

		len = cgi_FS_FOpenFile("ui/jahud.txt", &f, FS_READ);
		if (!f)
		{
			cgi_Error(S_COLOR_RED "default menu file not found: ui/hud.txt, unable to continue!\n");
		}
	}

	if (len >= MAX_MENUDEFFILE)
	{
		cgi_FS_FCloseFile(f);
		cgi_Error(
			va(S_COLOR_RED "menu file too large: %s is %i, max allowed is %i", menuFile, len, MAX_MENUDEFFILE));
	}

	cgi_FS_Read(buf, len, f);
	buf[len] = 0;
	cgi_FS_FCloseFile(f);

	//	COM_Compress(buf);

	//	cgi_UI_Menu_Reset();

	p = buf;

	COM_BeginParseSession();
	while (true)
	{
		const char* token = COM_ParseExt(&p, qtrue);
		if (!token || token[0] == 0 || token[0] == '}')
		{
			break;
		}

		if (Q_stricmp(token, "}") == 0)
		{
			break;
		}

		if (Q_stricmp(token, "loadmenu") == 0)
		{
			if (CG_Load_Menu(&p))
			{
				continue;
			}
			break;
		}
	}
	COM_EndParseSession();

	//Com_Printf("UI menu load time = %d milli seconds\n", cgi_Milliseconds() - start);
}

/*
=================
CG_LoadHudMenu();

=================
*/
void CG_LoadHudMenu()
{
	const char* hudSet = cg_hudFiles.string;

	if (hudSet[0] == '\0')
	{
		hudSet = "ui/jahud.txt";
	}

	CG_LoadMenus(hudSet);
}

/*
==============================================================================

INVENTORY SELECTION

==============================================================================
*/

/*
===============
CG_InventorySelectable
===============
*/
static qboolean CG_InventorySelectable(const int index)
{
	if (cg.snap->ps.inventory[index]) // Is there any in the inventory?
	{
		return qtrue;
	}

	return qfalse;
}

/*
===============
SetInventoryTime
===============
*/
static void SetInventoryTime()
{
	if (cg.weaponSelectTime + WEAPON_SELECT_TIME > cg.time ||
		// The Weapon HUD was currently active to just swap it out with Force HUD
		cg.forcepowerSelectTime + WEAPON_SELECT_TIME > cg.time)
		// The Force HUD was currently active to just swap it out with Force HUD
	{
		cg.weaponSelectTime = 0;
		cg.forcepowerSelectTime = 0;
		cg.inventorySelectTime = cg.time + 130.0f;
	}
	else
	{
		cg.inventorySelectTime = cg.time;
	}
}

/*
===============
CG_DPPrevInventory_f
===============
*/
void CG_DPPrevInventory_f()
{
	if (!cg.snap)
	{
		return;
	}

	const int original = cg.DataPadInventorySelect;

	for (int i = 0; i < INV_MAX; i++)
	{
		cg.DataPadInventorySelect--;

		if (cg.DataPadInventorySelect < INV_ELECTROBINOCULARS || cg.DataPadInventorySelect >= INV_MAX)
		{
			cg.DataPadInventorySelect = INV_MAX - 1;
		}

		if (CG_InventorySelectable(cg.DataPadInventorySelect))
		{
			return;
		}
	}

	cg.DataPadInventorySelect = original;
}

/*
===============
CG_DPNextInventory_f
===============
*/
void CG_DPNextInventory_f()
{
	if (!cg.snap)
	{
		return;
	}

	const int original = cg.DataPadInventorySelect;

	for (int i = 0; i < INV_MAX; i++)
	{
		cg.DataPadInventorySelect++;

		if (cg.DataPadInventorySelect < INV_ELECTROBINOCULARS || cg.DataPadInventorySelect >= INV_MAX)
		{
			cg.DataPadInventorySelect = INV_ELECTROBINOCULARS;
		}

		if (CG_InventorySelectable(cg.DataPadInventorySelect) && inv_icons[cg.DataPadInventorySelect])
		{
			return;
		}
	}

	cg.DataPadInventorySelect = original;
}

/*
===============
CG_NextInventory_f
===============
*/
void CG_NextInventory_f()
{
	if (!cg.snap)
	{
		return;
	}

	// The first time it's been hit so just show inventory but don't advance in inventory.
	const float* color = CG_FadeColor(cg.inventorySelectTime, WEAPON_SELECT_TIME);
	if (!color)
	{
		SetInventoryTime();
		return;
	}

	const int original = cg.inventorySelect;

	for (int i = 0; i < INV_MAX; i++)
	{
		cg.inventorySelect++;

		if (cg.inventorySelect < INV_ELECTROBINOCULARS || cg.inventorySelect >= INV_GRAPPLEHOOK)
		{
			if (cg_entities[0].gent->s.weapon == WP_DROIDEKA)
			{
				cg.inventorySelect = INV_BARRIER;
			}
			else
			{
				cg.inventorySelect = INV_ELECTROBINOCULARS;
			}
		}

		if (CG_InventorySelectable(cg.inventorySelect) && inv_icons[cg.inventorySelect])
		{
			G_StartNextItemEffect(cg_entities[0].gent, MEF_NO_SPIN, 700, 0.3f, 0);
			cgi_S_StartSound(nullptr, 0, CHAN_AUTO, cgs.media.selectSound2);
			SetInventoryTime();
			return;
		}
	}

	cg.inventorySelect = original;
}

/*
===============
CG_UseInventory_f
===============
*/
/*
this func was moved to Cmd_UseInventory_f in g_cmds.cpp
*/

/*
===============
CG_PrevInventory_f
===============
*/
void CG_PrevInventory_f()
{
	if (!cg.snap)
	{
		return;
	}

	// The first time it's been hit so just show inventory but don't advance in inventory.
	const float* color = CG_FadeColor(cg.inventorySelectTime, WEAPON_SELECT_TIME);
	if (!color)
	{
		SetInventoryTime();
		return;
	}

	const int original = cg.inventorySelect;

	for (int i = 0; i < INV_MAX; i++)
	{
		cg.inventorySelect--;

		if (cg.inventorySelect < INV_ELECTROBINOCULARS || cg.inventorySelect >= INV_GRAPPLEHOOK)
		{
			if (cg_entities[0].gent->s.weapon == WP_DROIDEKA)
			{
				cg.inventorySelect = INV_BARRIER;
			}
			else
			{
				cg.inventorySelect = INV_MAX - 1;
			}
		}

		if (CG_InventorySelectable(cg.inventorySelect) && inv_icons[cg.inventorySelect])
		{
			G_StartNextItemEffect(cg_entities[0].gent, MEF_NO_SPIN, 700, 0.3f, 0);
			cgi_S_StartSound(nullptr, 0, CHAN_AUTO, cgs.media.selectSound2);
			SetInventoryTime();
			return;
		}
	}

	cg.inventorySelect = original;
}

/*
===================
FindInventoryItemTag
===================
*/
gitem_t* FindInventoryItemTag(const int tag)
{
	for (int i = 1; i < bg_numItems; i++)
	{
		if (bg_itemlist[i].giTag == tag && bg_itemlist[i].giType == IT_HOLDABLE)
			// I guess giTag's aren't unique amongst items..must also make sure it's a holdable
		{
			return &bg_itemlist[i];
		}
	}

	return nullptr;
}

/*
===================
CG_DrawInventorySelect
===================
*/
void CG_DrawInventorySelect()
{
	int i;
	int iconCnt;
	int sideLeftIconCnt, sideRightIconCnt;
	constexpr vec4_t textColor = { .312f, .75f, .621f, 1.0f };
	char text[1024] = { 0 };

	// don't display if dead
	if (cg.predicted_player_state.stats[STAT_HEALTH] <= 0 || cg.snap->ps.viewEntity > 0 && cg.snap->ps.viewEntity <
		ENTITYNUM_WORLD)
	{
		return;
	}

	if (cg.inventorySelectTime + WEAPON_SELECT_TIME < cg.time) // Time is up for the HUD to display
	{
		return;
	}

	if (cg.predicted_player_state.communicatingflags & (1 << CF_SABERLOCKING) && cg_saberLockCinematicCamera.integer)
	{
		return;
	}

	if ((cg_SerenityJediEngineHudMode.integer == 4 || cg_SerenityJediEngineHudMode.integer == 5) && !
		cg_drawSelectionScrollBar.integer)
	{
		return;
	}

	const bool isOnVeh = G_IsRidingVehicle(cg_entities[0].gent) != nullptr;

	if (isOnVeh) //PM_WeaponOkOnVehicle
	{
		return;
	}

	int x2, y2, w2, h2;
	if (!cgi_UI_GetMenuInfo("inventoryselecthud", &x2, &y2, &w2, &h2))
	{
		return;
	}

	cg.iconSelectTime = cg.inventorySelectTime;

	// showing weapon select clears pickup item display, but not the blend blob
	cg.itemPickupTime = 0;

	// count the number of items owned
	int count = 0;
	for (i = 0; i < INV_MAX; i++)
	{
		if (CG_InventorySelectable(i) && inv_icons[i])
		{
			count++;
		}
	}

	if (!count)
	{
		cgi_SP_GetStringTextString("SP_INGAME_EMPTY_INV", text, sizeof text);
		const int w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontSmall, 1.0f);
		const int x = (SCREEN_WIDTH - w) / 2;
		CG_DrawProportionalString(x, y2 + 22, text, CG_CENTER | CG_SMALLFONT, colorTable[CT_ICON_BLUE]);
		return;
	}

	constexpr int sideMax = 3; // Max number of icons on the side

	// Calculate how many icons will appear to either side of the center one
	const int holdCount = count - 1; // -1 for the center icon
	if (holdCount == 0) // No icons to either side
	{
		sideLeftIconCnt = 0;
		sideRightIconCnt = 0;
	}
	else if (count > 2 * sideMax) // Go to the max on each side
	{
		sideLeftIconCnt = sideMax;
		sideRightIconCnt = sideMax;
	}
	else // Less than max, so do the calc
	{
		sideLeftIconCnt = holdCount / 2;
		sideRightIconCnt = holdCount - sideLeftIconCnt;
	}

	i = cg.inventorySelect - 1;
	if (i < 0)
	{
		i = INV_MAX - 1;
	}

	constexpr int smallIconSize = 22;
	constexpr int bigIconSize = 45;
	constexpr int pad = 16;

	constexpr int x = 320;
	constexpr int y = 410;

	// Left side ICONS
	// Work backwards from current icon
	int holdX = x - (bigIconSize / 2 + pad + smallIconSize);
	//height = smallIconSize * cg.iconHUDPercent;
	float addX = static_cast<float>(smallIconSize) * .75;

	for (iconCnt = 0; iconCnt < sideLeftIconCnt; i--)
	{
		if (i < 0)
		{
			i = INV_MAX - 1;
		}

		if (!CG_InventorySelectable(i) || !inv_icons[i])
		{
			continue;
		}

		++iconCnt; // Good icon

		if (inv_icons[i])
		{
			cgi_R_SetColor(nullptr);
			CG_DrawPic(holdX, y + 10, smallIconSize, smallIconSize, inv_icons[i]);

			cgi_R_SetColor(colorTable[CT_ICON_BLUE]);
			CG_DrawNumField(holdX + addX, y + smallIconSize, 2, cg.snap->ps.inventory[i], 6, 12,
				NUM_FONT_SMALL, qfalse);

			holdX -= smallIconSize + pad;
		}
	}

	// Current Center Icon
	//height = bigIconSize * cg.iconHUDPercent;
	if (inv_icons[cg.inventorySelect])
	{
		cgi_R_SetColor(nullptr);
		CG_DrawPic(x - bigIconSize / 2, y - (bigIconSize - smallIconSize) / 2 + 10, bigIconSize, bigIconSize,
			inv_icons[cg.inventorySelect]);
		addX = static_cast<float>(bigIconSize) * .75;
		cgi_R_SetColor(colorTable[CT_ICON_BLUE]);
		CG_DrawNumField(x - bigIconSize / 2 + addX, y, 2, cg.snap->ps.inventory[cg.inventorySelect], 6, 12,
			NUM_FONT_SMALL, qfalse);

		if (inv_names[cg.inventorySelect])
		{
			// FIXME: This is ONLY a temp solution, the icon stuff, etc, should all just use items.dat for everything
			const gitem_t* item = FindInventoryItemTag(cg.inventorySelect);

			if (item && item->classname && item->classname[0])
			{
				char itemName[256], data[1024]; // FIXME: do these really need to be this large??  does it matter?

				Com_sprintf(itemName, sizeof itemName, "SP_INGAME_%s", item->classname);

				if (cgi_SP_GetStringTextString(itemName, data, sizeof data))
				{
					const int w = cgi_R_Font_StrLenPixels(data, cgs.media.qhFontSmall, 1.0f);
					const int ox = (SCREEN_WIDTH - w) / 2;

					cgi_R_Font_DrawString(ox, SCREEN_HEIGHT - 24, data, textColor, cgs.media.qhFontSmall, -1,
						1.0f);
				}
			}
		}
	}

	i = cg.inventorySelect + 1;
	if (i > INV_MAX - 1)
	{
		i = 0;
	}

	// Right side ICONS
	// Work forwards from current icon
	holdX = x + bigIconSize / 2 + pad;
	//height = smallIconSize * cg.iconHUDPercent;
	addX = static_cast<float>(smallIconSize) * .75;
	for (iconCnt = 0; iconCnt < sideRightIconCnt; i++)
	{
		if (i > INV_MAX - 1)
		{
			i = 0;
		}

		if (!CG_InventorySelectable(i) || !inv_icons[i])
		{
			continue;
		}

		++iconCnt; // Good icon

		if (inv_icons[i])
		{
			cgi_R_SetColor(nullptr);
			CG_DrawPic(holdX, y + 10, smallIconSize, smallIconSize, inv_icons[i]);

			cgi_R_SetColor(colorTable[CT_ICON_BLUE]);
			CG_DrawNumField(holdX + addX, y + smallIconSize, 2, cg.snap->ps.inventory[i], 6, 12,
				NUM_FONT_SMALL, qfalse);

			holdX += smallIconSize + pad;
		}
	}
}

void cg_draw_inventory_select_side()
{
	// don't display if dead
	if (cg.predicted_player_state.stats[STAT_HEALTH] <= 0
		|| cg.snap->ps.viewEntity > 0 && cg.snap->ps.viewEntity < ENTITYNUM_WORLD)
	{
		return;
	}

	int x2, y2, w2, h2;
	if (!cgi_UI_GetMenuInfo("inventoryselecthud", &x2, &y2, &w2, &h2))
	{
		return;
	}

	cg.iconSelectTime = cg.inventorySelectTime;

	// showing weapon select clears pickup item display, but not the blend blob
	cg.itemPickupTime = 0;

	// count the number of items owned
	int count = 0;
	for (int i = 0; i < INV_MAX; i++)
	{
		if (CG_InventorySelectable(i) && inv_icons[i])
		{
			count++;
		}
	}

	if (!count)
	{
		return;
	}

	// Current Center Icon
	if (inv_icons[cg.inventorySelect])
	{
		constexpr int big_icon_size = 10;
		cgi_R_SetColor(nullptr);
		if (cg_SerenityJediEngineHudMode.integer == 4) // vertial
		{
			CG_DrawPic(27, 447, big_icon_size, big_icon_size, inv_icons[cg.inventorySelect]);
		}
		else // horizontal
		{
			CG_DrawPic(13, 439, big_icon_size, big_icon_size, inv_icons[cg.inventorySelect]);
		}
		cgi_R_SetColor(colorTable[CT_ICON_BLUE]);
	}
}

void CG_DrawInventorySelect_text()
{
	constexpr vec4_t text_color = { .312f, .75f, .621f, 1.0f };
	char text[1024] = { 0 };

	// don't display if dead
	if (cg.predicted_player_state.stats[STAT_HEALTH] <= 0
		|| cg.snap->ps.viewEntity > 0 && cg.snap->ps.viewEntity <
		ENTITYNUM_WORLD)
	{
		return;
	}

	if (cg.inventorySelectTime + WEAPON_SELECT_TIME < cg.time) // Time is up for the HUD to display
	{
		return;
	}

	int x2, y2, w2, h2;
	if (!cgi_UI_GetMenuInfo("inventoryselecthud", &x2, &y2, &w2, &h2))
	{
		return;
	}

	cg.iconSelectTime = cg.inventorySelectTime;

	// showing weapon select clears pickup item display, but not the blend blob
	cg.itemPickupTime = 0;

	// count the number of items owned
	int count = 0;

	for (int i = 0; i < INV_MAX; i++)
	{
		if (CG_InventorySelectable(i) && inv_icons[i])
		{
			count++;
		}
	}

	if (!count)
	{
		cgi_SP_GetStringTextString("SP_INGAME_EMPTY_INV", text, sizeof text);
		if (cg_SerenityJediEngineHudMode.integer == 4) // vertial
		{
			cgi_R_Font_DrawString(9, SCREEN_HEIGHT - 21, text, text_color, cgs.media.qhFontSmall, -1, 0.5f);
		}
		else // horizontal
		{
			cgi_R_Font_DrawString(59, SCREEN_HEIGHT - 21, text, text_color, cgs.media.qhFontSmall, -1, 0.5f);
		}
		return;
	}

	// Current Center Icon
	if (inv_icons[cg.inventorySelect])
	{
		//if (cg_SerenityJediEngineHudMode.integer == 4 || cg_SerenityJediEngineHudMode.integer == 5)
		cgi_R_SetColor(text_color);
		if (cg_SerenityJediEngineHudMode.integer == 4) // vertial
		{
			CG_DrawNumField(31, 449, 2, cg.snap->ps.inventory[cg.inventorySelect], 3, 6, NUM_FONT_SMALL, qfalse);
		}
		else // horizontal
		{
			CG_DrawNumField(16, 441, 2, cg.snap->ps.inventory[cg.inventorySelect], 3, 6, NUM_FONT_SMALL, qfalse);
		}

		if (inv_names[cg.inventorySelect])
		{
			const gitem_t* item = FindInventoryItemTag(cg.inventorySelect);

			if (item && item->classname && item->classname[0])
			{
				char item_name[256], data[1024];

				Com_sprintf(item_name, sizeof item_name, "SP_INGAME_%s", item->classname);

				if (cgi_SP_GetStringTextString(item_name, data, sizeof data) && !cg_drawSelectionScrollBar.integer)
				{
					if (cg_SerenityJediEngineHudMode.integer == 4) // vertial
					{
						cgi_R_Font_DrawString(9, SCREEN_HEIGHT - 21, data, text_color, cgs.media.qhFontSmall, -1,
							0.5f);
					}
					else // horizontal
					{
						cgi_R_Font_DrawString(59, SCREEN_HEIGHT - 21, data, text_color, cgs.media.qhFontSmall, -1,
							0.5f);
					}
				}
			}
		}
	}
}

int cgi_UI_GetItemText(char* menuFile, char* itemName, char* text);

const char* inventoryDesc[15] =
{
	"NEURO_SAAV_DESC",
	"BACTA_DESC",
	"INQUISITOR_DESC",
	"LA_GOGGLES_DESC",
	"PORTABLE_SENTRY_DESC",
	"GOODIE_KEY_DESC",
	"SECURITY_KEY_DP_DESC",
	"CLOAK_DESC",
	"BARRIER_DESC",
	"GRAPPLE_DESC",
};

/*
===================
CG_DrawDataPadInventorySelect
===================
*/
void CG_DrawDataPadInventorySelect()
{
	int i;
	int iconCnt;
	int sideLeftIconCnt, sideRightIconCnt;
	char text[1024] = { 0 };
	constexpr vec4_t textColor = { .312f, .75f, .621f, 1.0f };

	// count the number of items owned
	int count = 0;
	for (i = 0; i < INV_MAX; i++)
	{
		if (CG_InventorySelectable(i) && inv_icons[i])
		{
			count++;
		}
	}

	if (!count)
	{
		cgi_SP_GetStringTextString("SP_INGAME_EMPTY_INV", text, sizeof text);
		const int w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontSmall, 1.0f);
		const int x = (SCREEN_WIDTH - w) / 2;
		CG_DrawProportionalString(x, 300 + 22, text, CG_CENTER | CG_SMALLFONT, colorTable[CT_ICON_BLUE]);
		return;
	}

	constexpr int sideMax = 1; // Max number of icons on the side

	// Calculate how many icons will appear to either side of the center one
	const int holdCount = count - 1; // -1 for the center icon
	if (holdCount == 0) // No icons to either side
	{
		sideLeftIconCnt = 0;
		sideRightIconCnt = 0;
	}
	else if (count > 2 * sideMax) // Go to the max on each side
	{
		sideLeftIconCnt = sideMax;
		sideRightIconCnt = sideMax;
	}
	else // Less than max, so do the calc
	{
		sideLeftIconCnt = holdCount / 2;
		sideRightIconCnt = holdCount - sideLeftIconCnt;
	}

	i = cg.DataPadInventorySelect - 1;
	if (i < 0)
	{
		i = INV_MAX - 1;
	}

	constexpr int smallIconSize = 22;
	constexpr int bigIconSize = 45;
	constexpr int bigPad = 64;
	constexpr int pad = 32;

	constexpr int centerXPos = 320;
	constexpr int graphicYPos = 300;

	// Left side ICONS
	// Work backwards from current icon
	int holdX = centerXPos - (bigIconSize / 2 + bigPad + smallIconSize);
	float addX = static_cast<float>(smallIconSize) * .75;

	for (iconCnt = 0; iconCnt < sideLeftIconCnt; i--)
	{
		if (i < 0)
		{
			i = INV_MAX - 1;
		}

		if (!CG_InventorySelectable(i) || !inv_icons[i])
		{
			continue;
		}

		++iconCnt; // Good icon

		if (inv_icons[i])
		{
			cgi_R_SetColor(colorTable[CT_WHITE]);
			CG_DrawPic(holdX, graphicYPos + 10, smallIconSize, smallIconSize, inv_icons[i]);

			cgi_R_SetColor(colorTable[CT_ICON_BLUE]);
			CG_DrawNumField(holdX + addX, graphicYPos + smallIconSize, 2, cg.snap->ps.inventory[i], 6, 12,
				NUM_FONT_SMALL, qfalse);

			holdX -= smallIconSize + pad;
		}
	}

	// Current Center Icon
	if (inv_icons[cg.DataPadInventorySelect])
	{
		cgi_R_SetColor(colorTable[CT_WHITE]);
		CG_DrawPic(centerXPos - bigIconSize / 2, graphicYPos - (bigIconSize - smallIconSize) / 2 + 10,
			bigIconSize, bigIconSize, inv_icons[cg.DataPadInventorySelect]);
		addX = static_cast<float>(bigIconSize) * .75;
		cgi_R_SetColor(colorTable[CT_ICON_BLUE]);
		CG_DrawNumField(centerXPos - bigIconSize / 2 + addX, graphicYPos, 2,
			cg.snap->ps.inventory[cg.DataPadInventorySelect], 6, 12,
			NUM_FONT_SMALL, qfalse);
	}

	i = cg.DataPadInventorySelect + 1;
	if (i > INV_MAX - 1)
	{
		i = 0;
	}

	// Right side ICONS
	// Work forwards from current icon
	holdX = centerXPos + bigIconSize / 2 + bigPad;
	addX = static_cast<float>(smallIconSize) * .75;
	for (iconCnt = 0; iconCnt < sideRightIconCnt; i++)
	{
		if (i > INV_MAX - 1)
		{
			i = 0;
		}

		if (!CG_InventorySelectable(i) || !inv_icons[i])
		{
			continue;
		}

		++iconCnt; // Good icon

		if (inv_icons[i])
		{
			cgi_R_SetColor(colorTable[CT_WHITE]);
			CG_DrawPic(holdX, graphicYPos + 10, smallIconSize, smallIconSize, inv_icons[i]);

			cgi_R_SetColor(colorTable[CT_ICON_BLUE]);
			CG_DrawNumField(holdX + addX, graphicYPos + smallIconSize, 2, cg.snap->ps.inventory[i], 6, 12,
				NUM_FONT_SMALL, qfalse);

			holdX += smallIconSize + pad;
		}
	}

	// draw the weapon description
	if (cg.DataPadInventorySelect >= 0 && cg.DataPadInventorySelect < 13)
	{
		if (!cgi_SP_GetStringTextString(va("SP_INGAME_%s", inventoryDesc[cg.DataPadInventorySelect]), text,
			sizeof text))
		{
			cgi_SP_GetStringTextString(va("MD_%s", inventoryDesc[cg.DataPadInventorySelect]), text, sizeof text);
		}

		if (text[0])
		{
			CG_DisplayBoxedText(70, 50, 500, 300, text,
				cgs.media.qhFontSmall,
				0.7f,
				textColor
			);
		}
	}
}

/*
===============
SetForcePowerTime
===============
*/
void SetForcePowerTime()
{
	if (cg.weaponSelectTime + WEAPON_SELECT_TIME > cg.time ||
		// The Weapon HUD was currently active to just swap it out with Force HUD
		cg.inventorySelectTime + WEAPON_SELECT_TIME > cg.time)
		// The Inventory HUD was currently active to just swap it out with Force HUD
	{
		cg.weaponSelectTime = 0;
		cg.inventorySelectTime = 0;
		cg.forcepowerSelectTime = cg.time + 130.0f;
	}
	else
	{
		cg.forcepowerSelectTime = cg.time;
	}
}

int showPowers[MAX_SHOWPOWERS] =
{
	FP_ABSORB,
	FP_HEAL,
	FP_PROTECT,
	FP_TELEPATHY,
	FP_LIGHTNING_STRIKE,
	FP_BLAST,

	FP_STASIS,
	FP_SPEED,
	FP_PUSH,
	FP_PULL,
	FP_SEE,
	FP_GRASP,
	FP_REPULSE,

	FP_DRAIN,
	FP_LIGHTNING,
	FP_RAGE,
	FP_GRIP,
	FP_DESTRUCTION,
	FP_FEAR,
	FP_DEADLYSIGHT,
	FP_PROJECTION,
};

const char* showPowersName[MAX_SHOWPOWERS] =
{
	"SP_INGAME_ABSORB2",
	"SP_INGAME_HEAL2",
	"SP_INGAME_PROTECT2",
	"SP_INGAME_MINDTRICK2",
	"SP_INGAME_LIGHTNING_STRIKE2",
	"SP_INGAME_BLAST2",

	"SP_INGAME_STASIS2",
	"SP_INGAME_SPEED2",
	"SP_INGAME_PUSH2",
	"SP_INGAME_PULL2",
	"SP_INGAME_SEEING2",
	"SP_INGAME_GRASP2",
	"SP_INGAME_REPULSE2",

	"SP_INGAME_DRAIN2",
	"SP_INGAME_LIGHTNING2",
	"SP_INGAME_DARK_RAGE2",
	"SP_INGAME_GRIP2",
	"SP_INGAME_DESTRUCTION2",
	"SP_INGAME_FEAR2",
	"SP_INGAME_DEADLYSIGHT2",
	"SP_INGAME_PROJECTION2",
};

// Keep these with groups light side, core, and dark side
int showDataPadPowers[MAX_DPSHOWPOWERS] =
{
	// Light side
	FP_ABSORB,
	FP_HEAL,
	FP_PROTECT,
	FP_TELEPATHY,
	FP_LIGHTNING_STRIKE,
	FP_BLAST,
	FP_STASIS,

	// Core Powers
	FP_LEVITATION,
	FP_SPEED,
	FP_PUSH,
	FP_PULL,
	FP_SABERTHROW,
	FP_SABER_DEFENSE,
	FP_SABER_OFFENSE,
	FP_SEE,
	FP_GRASP,
	FP_REPULSE,

	//Dark Side
	FP_DRAIN,
	FP_LIGHTNING,
	FP_RAGE,
	FP_GRIP,
	FP_DESTRUCTION,
	FP_FEAR,
	FP_DEADLYSIGHT,
	FP_PROJECTION,
};

/*
===============
ForcePower_Valid
===============
*/
qboolean ForcePower_Valid(const int index)
{
	const gentity_t* player = &g_entities[0];

	assert(MAX_SHOWPOWERS == sizeof showPowers / sizeof showPowers[0]);
	assert(index < MAX_SHOWPOWERS); //is this a valid index?
	if (player->client->ps.forcePowersKnown & 1 << showPowers[index] &&
		player->client->ps.forcePowerLevel[showPowers[index]]) // Does he have the force power?
	{
		return qtrue;
	}

	return qfalse;
}

/*
===============
CG_NextForcePower_f
===============
*/
void CG_NextForcePower_f()
{
	const gentity_t* player = &g_entities[0];

	if (!cg.snap || in_camera)
	{
		return;
	}

	if (!player->client->ps.forcePowersKnown)
	{
		CG_NextInventory_f();
		return;
	}

	SetForcePowerTime();

	if (cg.forcepowerSelectTime + WEAPON_SELECT_TIME < cg.time)
	{
		return;
	}

	const int original = cg.forcepowerSelect;

	for (int i = 0; i < MAX_SHOWPOWERS; i++)
	{
		cg.forcepowerSelect++;

		if (cg.forcepowerSelect >= MAX_SHOWPOWERS)
		{
			cg.forcepowerSelect = 0;
		}

		if (ForcePower_Valid(cg.forcepowerSelect)) // Does he have the force power?
		{
			G_StartNextItemEffect(cg_entities[0].gent, MEF_NO_SPIN, 700, 0.3f, 0);
			cgi_S_StartSound(nullptr, 0, CHAN_AUTO, cgs.media.selectSound2);
			return;
		}
	}

	cg.forcepowerSelect = original;
}

/*
===============
CG_PrevForcePower_f
===============
*/
void CG_PrevForcePower_f()
{
	const gentity_t* player = &g_entities[0];

	if (!cg.snap || in_camera)
	{
		return;
	}

	if (!player->client->ps.forcePowersKnown)
	{
		CG_PrevInventory_f();
		return;
	}

	SetForcePowerTime();

	if (cg.forcepowerSelectTime + WEAPON_SELECT_TIME < cg.time)
	{
		return;
	}

	const int original = cg.forcepowerSelect;

	for (int i = 0; i < MAX_SHOWPOWERS; i++)
	{
		cg.forcepowerSelect--;

		if (cg.forcepowerSelect < 0)
		{
			cg.forcepowerSelect = MAX_SHOWPOWERS - 1;
		}

		if (ForcePower_Valid(cg.forcepowerSelect)) // Does he have the force power?
		{
			G_StartNextItemEffect(cg_entities[0].gent, MEF_NO_SPIN, 700, 0.3f, 0);
			cgi_S_StartSound(nullptr, 0, CHAN_AUTO, cgs.media.selectSound2);
			return;
		}
	}

	cg.forcepowerSelect = original;
}

/*
===================
CG_DrawForceSelect
===================
*/
void CG_DrawForceSelect()
{
	int i;
	int sideLeftIconCnt, sideRightIconCnt;
	int iconCnt;
	char text[1024] = { 0 };
	constexpr int yOffset = 0;
	int sideMax;
	int smallIconSize, bigIconSize;

	// don't display if dead
	if (cg.predicted_player_state.stats[STAT_HEALTH] <= 0 || cg.snap->ps.viewEntity > 0 && cg.snap->ps.viewEntity <
		ENTITYNUM_WORLD)
	{
		return;
	}

	if (cg.predicted_player_state.communicatingflags & (1 << CF_SABERLOCKING) && cg_saberLockCinematicCamera.integer)
	{
		return;
	}

	if (cg.forcepowerSelectTime + WEAPON_SELECT_TIME < cg.time) // Time is up for the HUD to display
	{
		return;
	}

	// count the number of powers owned
	int count = 0;

	for (i = 0; i < MAX_SHOWPOWERS; ++i)
	{
		if (ForcePower_Valid(i))
		{
			count++;
		}
	}

	if (count == 0) // If no force powers, don't display
	{
		return;
	}
	const bool isOnVeh = G_IsRidingVehicle(cg_entities[0].gent) != nullptr;

	cg.iconSelectTime = cg.forcepowerSelectTime;

	// showing weapon select clears pickup item display, but not the blend blob
	cg.itemPickupTime = 0;

	if (isOnVeh) //PM_WeaponOkOnVehicle
	{
		sideMax = 1; // Max number of icons on the side
	}
	else
	{
		sideMax = 3; // Max number of icons on the side
	}

	// Calculate how many icons will appear to either side of the center one
	const int holdCount = count - 1; // -1 for the center icon
	if (holdCount == 0) // No icons to either side
	{
		sideLeftIconCnt = 0;
		sideRightIconCnt = 0;
	}
	else if (count > 2 * sideMax) // Go to the max on each side
	{
		sideLeftIconCnt = sideMax;
		sideRightIconCnt = sideMax;
	}
	else // Less than max, so do the calc
	{
		sideLeftIconCnt = holdCount / 2;
		sideRightIconCnt = holdCount - sideLeftIconCnt;
	}

	if (isOnVeh) //PM_WeaponOkOnVehicle
	{
		smallIconSize = 11;
		bigIconSize = 22.5;
	}
	else
	{
		smallIconSize = 22;
		bigIconSize = 45;
	}

	constexpr int pad = 12;

	constexpr int x = 320;
	constexpr int y = 425;

	i = cg.forcepowerSelect - 1;
	if (i < 0)
	{
		i = MAX_SHOWPOWERS - 1;
	}

	cgi_R_SetColor(nullptr);
	// Work backwards from current icon
	int holdX = x - (bigIconSize / 2 + pad + smallIconSize);
	for (iconCnt = 1; iconCnt < sideLeftIconCnt + 1; i--)
	{
		if (i < 0)
		{
			i = MAX_SHOWPOWERS - 1;
		}

		if (!ForcePower_Valid(i)) // Does he have this power?
		{
			continue;
		}

		++iconCnt; // Good icon

		if (force_icons[showPowers[i]])
		{
			if (isOnVeh) //PM_WeaponOkOnVehicle
			{
				CG_DrawPic(holdX, y - 10 + yOffset, smallIconSize, smallIconSize, force_icons[showPowers[i]]);
				holdX -= smallIconSize + pad;
			}
			else
			{
				CG_DrawPic(holdX, y + yOffset, smallIconSize, smallIconSize, force_icons[showPowers[i]]);
				holdX -= smallIconSize + pad;
			}
		}
	}

	// Current Center Icon
	if (force_icons[showPowers[cg.forcepowerSelect]])
	{
		if (isOnVeh) //PM_WeaponOkOnVehicle
		{
			CG_DrawPic(x - bigIconSize / static_cast<float>(2), y - (static_cast<float>(bigIconSize) - smallIconSize) / 2 - 10 + yOffset, bigIconSize,
				bigIconSize, force_icons[showPowers[cg.forcepowerSelect]]);
		}
		else
		{
			CG_DrawPic(x - bigIconSize / static_cast<float>(2), y - (static_cast<float>(bigIconSize) - smallIconSize) / 2 + yOffset, bigIconSize,
				bigIconSize, force_icons[showPowers[cg.forcepowerSelect]]);
		}
	}

	i = cg.forcepowerSelect + 1;
	if (i >= MAX_SHOWPOWERS)
	{
		i = 0;
	}

	// Work forwards from current icon
	holdX = x + bigIconSize / 2 + pad;
	for (iconCnt = 1; iconCnt < sideRightIconCnt + 1; i++)
	{
		if (i >= MAX_SHOWPOWERS)
		{
			i = 0;
		}

		if (!ForcePower_Valid(i)) // Does he have this power?
		{
			continue;
		}

		++iconCnt; // Good icon

		if (force_icons[showPowers[i]])
		{
			if (isOnVeh) //PM_WeaponOkOnVehicle
			{
				CG_DrawPic(holdX, y - 10 + yOffset, smallIconSize, smallIconSize, force_icons[showPowers[i]]);
				holdX += smallIconSize + pad;
			}
			else
			{
				CG_DrawPic(holdX, y + yOffset, smallIconSize, smallIconSize, force_icons[showPowers[i]]);
				holdX += smallIconSize + pad;
			}
		}
	}

	if (isOnVeh) //PM_WeaponOkOnVehicle
	{
		//
	}
	else
	{
		// This only a temp solution.
		if (cgi_SP_GetStringTextString(showPowersName[cg.forcepowerSelect], text, sizeof text))
		{
			const int w = cgi_R_Font_StrLenPixels(text, cgs.media.qhFontSmall, 1.0f);
			const int ox = (SCREEN_WIDTH - w) / 2;
			cgi_R_Font_DrawString(ox, SCREEN_HEIGHT - 24 + yOffset, text, colorTable[CT_ICON_BLUE],
				cgs.media.qhFontSmall, -1, 1.0f);
		}
	}
}

void CG_DrawForceSelect_text()
{
	char text[1024] = { 0 };

	// don't display if dead
	if (cg.predicted_player_state.stats[STAT_HEALTH] <= 0
		|| cg.snap->ps.viewEntity > 0 && cg.snap->ps.viewEntity < ENTITYNUM_WORLD)
	{
		return;
	}

	if (cg.forcepowerSelectTime + WEAPON_SELECT_TIME < cg.time) // Time is up for the HUD to display
	{
		return;
	}

	// count the number of powers owned
	int count = 0;

	for (int i = 0; i < MAX_SHOWPOWERS; ++i)
	{
		if (ForcePower_Valid(i))
		{
			count++;
		}
	}

	if (count == 0) // If no force powers, don't display
	{
		return;
	}

	cg.iconSelectTime = cg.forcepowerSelectTime;

	// showing weapon select clears pickup item display, but not the blend blob
	cg.itemPickupTime = 0;

	// This only a temp solution.
	if (cgi_SP_GetStringTextString(showPowersName[cg.forcepowerSelect], text, sizeof text) && !
		cg_drawSelectionScrollBar
		.integer)
	{
		constexpr int yOffset = 0;
		if (cg_SerenityJediEngineHudMode.integer == 4) // vertial
		{
			cgi_R_Font_DrawString(9, SCREEN_HEIGHT - 21 + yOffset, text, colorTable[CT_ICON_BLUE],
				cgs.media.qhFontSmall, -1, 0.5f);
		}
		else // horizontal
		{
			cgi_R_Font_DrawString(59, SCREEN_HEIGHT - 21 + yOffset, text, colorTable[CT_ICON_BLUE],
				cgs.media.qhFontSmall, -1, 0.5f);
		}
	}
}

void CG_DrawForceSelect_side()
{
	// don't display if dead
	if (cg.predicted_player_state.stats[STAT_HEALTH] <= 0 || cg.snap->ps.viewEntity > 0 && cg.snap->ps.viewEntity <
		ENTITYNUM_WORLD)
	{
		return;
	}

	// count the number of powers owned
	int count = 0;

	for (int i = 0; i < MAX_SHOWPOWERS; ++i)
	{
		if (ForcePower_Valid(i))
		{
			count++;
		}
	}

	if (count == 0) // If no force powers, don't display
	{
		return;
	}

	cg.iconSelectTime = cg.forcepowerSelectTime;

	// showing weapon select clears pickup item display, but not the blend blob
	cg.itemPickupTime = 0;

	cgi_R_SetColor(nullptr);

	// Current Center Icon
	if (force_icons[showPowers[cg.forcepowerSelect]])
	{
		constexpr int big_icon_size = 24;
		constexpr int small_icon_size = 12;
		constexpr int y_offset = 0;

		if (cg_SerenityJediEngineHudMode.integer == 4) // vertical
		{
			CG_DrawPic(32 - big_icon_size / 2, 420 - (big_icon_size - small_icon_size) / 2 + y_offset,
				big_icon_size,
				big_icon_size, force_icons[showPowers[cg.forcepowerSelect]]);
		}
		else // horizonal
		{
			CG_DrawPic(45 - big_icon_size / 2, 438 - (big_icon_size - small_icon_size) / 2 + y_offset,
				big_icon_size,
				big_icon_size, force_icons[showPowers[cg.forcepowerSelect]]);
		}
	}
}

/*
===============
ForcePowerDataPad_Valid
===============
*/
qboolean ForcePowerDataPad_Valid(const int index)
{
	const gentity_t* player = &g_entities[0];

	assert(index < MAX_DPSHOWPOWERS);
	if (player->client->ps.forcePowersKnown & 1 << showDataPadPowers[index] &&
		player->client->ps.forcePowerLevel[showDataPadPowers[index]]) // Does he have the force power?
	{
		return qtrue;
	}

	return qfalse;
}

/*
===============
CG_DPNextForcePower_f
===============
*/
void CG_DPNextForcePower_f()
{
	if (!cg.snap)
	{
		return;
	}

	const int original = cg.DataPadforcepowerSelect;

	for (int i = 0; i < MAX_DPSHOWPOWERS; i++)
	{
		cg.DataPadforcepowerSelect++;

		if (cg.DataPadforcepowerSelect >= MAX_DPSHOWPOWERS)
		{
			cg.DataPadforcepowerSelect = 0;
		}

		if (ForcePowerDataPad_Valid(cg.DataPadforcepowerSelect)) // Does he have the force power?
		{
			return;
		}
	}

	cg.DataPadforcepowerSelect = original;
}

/*
===============
CG_DPPrevForcePower_f
===============
*/
void CG_DPPrevForcePower_f()
{
	if (!cg.snap)
	{
		return;
	}

	const int original = cg.DataPadforcepowerSelect;

	for (int i = 0; i < MAX_DPSHOWPOWERS; i++)
	{
		cg.DataPadforcepowerSelect--;

		if (cg.DataPadforcepowerSelect < 0)
		{
			cg.DataPadforcepowerSelect = MAX_DPSHOWPOWERS - 1;
		}

		if (ForcePowerDataPad_Valid(cg.DataPadforcepowerSelect)) // Does he have the force power?
		{
			return;
		}
	}

	cg.DataPadforcepowerSelect = original;
}

const char* forcepowerDesc[NUM_FORCE_POWERS] =
{
	"FORCE_ABSORB_DESC",
	"FORCE_HEAL_DESC",
	"FORCE_PROTECT_DESC",
	"FORCE_MIND_TRICK_DESC",
	"FORCE_LIGHTNINGSTRIKE_DESC",
	"FORCE_BLAST_DESC",
	"FORCE_STASIS_DESC",

	"FORCE_JUMP_DESC",
	"FORCE_SPEED_DESC",
	"FORCE_PUSH_DESC",
	"FORCE_PULL_DESC",
	"FORCE_SABER_THROW_DESC",
	"FORCE_SABER_DEFENSE_DESC",
	"FORCE_SABER_OFFENSE_DESC",
	"FORCE_SENSE_DESC",
	"FORCE_GRASP_DESC",
	"FORCE_REPULSE_DESC",

	"FORCE_DRAIN_DESC",
	"FORCE_LIGHTNING_DESC",
	"FORCE_RAGE_DESC",
	"FORCE_GRIP_DESC",
	"FORCE_DESTRUCTION_DESC",
	"FORCE_FEAR_DESC",
	"FORCE_DEADLYSIGHT_DESC",
	"FORCE_PROJECTION_DESC",
};

const char* forcepowerLvl1Desc[NUM_FORCE_POWERS] =
{
	"FORCE_ABSORB_LVL1_DESC",
	"FORCE_HEAL_LVL1_DESC",
	"FORCE_PROTECT_LVL1_DESC",
	"FORCE_MIND_TRICK_LVL1_DESC",
	"FORCE_LIGHTNING_STRIKE_LVL1_DESC",
	"FORCE_BLAST_LVL1_DESC",
	"FORCE_STASIS_LVL1_DESC",

	"FORCE_JUMP_LVL1_DESC",
	"FORCE_SPEED_LVL1_DESC",
	"FORCE_PUSH_LVL1_DESC",
	"FORCE_PULL_LVL1_DESC",
	"FORCE_SABER_THROW_LVL1_DESC",
	"FORCE_SABER_DEFENSE_LVL1_DESC",
	"FORCE_SABER_OFFENSE_LVL1_DESC",
	"FORCE_SENSE_LVL1_DESC",
	"FORCE_GRASP_LVL1_DESC",
	"FORCE_REPULSE_LVL1_DESC",

	"FORCE_DRAIN_LVL1_DESC",
	"FORCE_LIGHTNING_LVL1_DESC",
	"FORCE_RAGE_LVL1_DESC",
	"FORCE_GRIP_LVL1_DESC",
	"FORCE_DESTRUCTION_LVL1_DESC",
	"FORCE_FEAR_LVL1_DESC",
	"FORCE_DEADLYSIGHT_LVL1_DESC",
	"FORCE_PROJECTION_LVL1_DESC",
};

const char* forcepowerLvl2Desc[NUM_FORCE_POWERS] =
{
	"FORCE_ABSORB_LVL2_DESC",
	"FORCE_HEAL_LVL2_DESC",
	"FORCE_PROTECT_LVL2_DESC",
	"FORCE_MIND_TRICK_LVL2_DESC",
	"FORCE_LIGHTNING_STRIKE_LVL2_DESC",
	"FORCE_BLAST_LVL2_DESC",
	"FORCE_STASIS_LVL2_DESC",

	"FORCE_JUMP_LVL2_DESC",
	"FORCE_SPEED_LVL2_DESC",
	"FORCE_PUSH_LVL2_DESC",
	"FORCE_PULL_LVL2_DESC",
	"FORCE_SABER_THROW_LVL2_DESC",
	"FORCE_SABER_DEFENSE_LVL2_DESC",
	"FORCE_SABER_OFFENSE_LVL2_DESC",
	"FORCE_SENSE_LVL2_DESC",
	"FORCE_GRASP_LVL2_DESC",
	"FORCE_REPULSE_LVL2_DESC",

	"FORCE_DRAIN_LVL2_DESC",
	"FORCE_LIGHTNING_LVL2_DESC",
	"FORCE_RAGE_LVL2_DESC",
	"FORCE_GRIP_LVL2_DESC",
	"FORCE_DESTRUCTION_LVL2_DESC",
	"FORCE_FEAR_LVL2_DESC",
	"FORCE_DEADLYSIGHT_LVL2_DESC",
	"FORCE_PROJECTION_LVL2_DESC",
};

const char* forcepowerLvl3Desc[NUM_FORCE_POWERS] =
{
	"FORCE_ABSORB_LVL3_DESC",
	"FORCE_HEAL_LVL3_DESC",
	"FORCE_PROTECT_LVL3_DESC",
	"FORCE_MIND_TRICK_LVL3_DESC",
	"FORCE_LIGHTNING_STRIKE_LVL3_DESC",
	"FORCE_BLAST_LVL3_DESC",
	"FORCE_STASIS_LVL3_DESC",

	"FORCE_JUMP_LVL3_DESC",
	"FORCE_SPEED_LVL3_DESC",
	"FORCE_PUSH_LVL3_DESC",
	"FORCE_PULL_LVL3_DESC",
	"FORCE_SABER_THROW_LVL3_DESC",
	"FORCE_SABER_DEFENSE_LVL3_DESC",
	"FORCE_SABER_OFFENSE_LVL3_DESC",
	"FORCE_SENSE_LVL3_DESC",
	"FORCE_GRASP_LVL3_DESC",
	"FORCE_REPULSE_LVL3_DESC",

	"FORCE_DRAIN_LVL3_DESC",
	"FORCE_LIGHTNING_LVL3_DESC",
	"FORCE_RAGE_LVL3_DESC",
	"FORCE_GRIP_LVL3_DESC",
	"FORCE_DESTRUCTION_LVL3_DESC",
	"FORCE_FEAR_LVL3_DESC",
	"FORCE_DEADLYSIGHT_LVL3_DESC",
	"FORCE_PROJECTION_LVL3_DESC",
};

/*
===================
CG_DrawDataPadForceSelect
===================
*/
void CG_DrawDataPadForceSelect()
{
	int i;
	int sideLeftIconCnt, sideRightIconCnt;
	int iconCnt;
	char text[1024] = { 0 };
	char text2[1024] = { 0 };

	// count the number of powers known
	int count = 0;
	for (i = 0; i < MAX_DPSHOWPOWERS; i++)
	{
		if (ForcePowerDataPad_Valid(i))
		{
			count++;
		}
	}

	if (count < 1) // If no force powers, don't display
	{
		return;
	}

	// Time to switch new icon colors
	cg.iconSelectTime = cg.forcepowerSelectTime;

	constexpr int sideMax = 1; // Max number of icons on the side

	// Calculate how many icons will appear to either side of the center one
	const int holdCount = count - 1; // -1 for the center icon
	if (holdCount == 0) // No icons to either side
	{
		sideLeftIconCnt = 0;
		sideRightIconCnt = 0;
	}
	else if (count > 2 * sideMax) // Go to the max on each side
	{
		sideLeftIconCnt = sideMax;
		sideRightIconCnt = sideMax;
	}
	else // Less than max, so do the calc
	{
		sideLeftIconCnt = holdCount / 2;
		sideRightIconCnt = holdCount - sideLeftIconCnt;
	}

	constexpr int smallIconSize = 22;
	constexpr int bigIconSize = 45;
	constexpr int bigPad = 64;
	constexpr int pad = 32;

	constexpr int centerXPos = 320;
	constexpr int graphicYPos = 310;

	i = cg.DataPadforcepowerSelect - 1;
	if (i < 0)
	{
		i = MAX_DPSHOWPOWERS - 1;
	}

	// Print icons to the left of the center
	cgi_R_SetColor(colorTable[CT_WHITE]);

	// Work backwards from current icon
	int holdX = centerXPos - (bigIconSize / 2 + bigPad + smallIconSize);
	for (iconCnt = 1; iconCnt < sideLeftIconCnt + 1; i--)
	{
		if (i < 0)
		{
			i = MAX_DPSHOWPOWERS - 1;
		}

		if (!ForcePowerDataPad_Valid(i)) // Does he have this power?
		{
			continue;
		}

		++iconCnt; // Good icon

		if (force_icons[showDataPadPowers[i]])
		{
			CG_DrawPic(holdX, graphicYPos, smallIconSize, smallIconSize, force_icons[showDataPadPowers[i]]);
		}

		// A new force power
		if (cg_updatedDataPadForcePower1.integer - 1 == showDataPadPowers[i] ||
			cg_updatedDataPadForcePower2.integer - 1 == showDataPadPowers[i] ||
			cg_updatedDataPadForcePower3.integer - 1 == showDataPadPowers[i])
		{
			CG_DrawPic(holdX, graphicYPos, smallIconSize, smallIconSize, cgs.media.DPForcePowerOverlay);
		}

		if (force_icons[showDataPadPowers[i]])
		{
			holdX -= smallIconSize + pad;
		}
	}

	// Current Center Icon
	if (force_icons[showDataPadPowers[cg.DataPadforcepowerSelect]])
	{
		cgi_R_SetColor(colorTable[CT_WHITE]);
		CG_DrawPic(centerXPos - bigIconSize / 2, graphicYPos - (bigIconSize - smallIconSize) / 2, bigIconSize,
			bigIconSize, force_icons[showDataPadPowers[cg.DataPadforcepowerSelect]]);

		// New force power
		if (cg_updatedDataPadForcePower1.integer - 1 == showDataPadPowers[cg.DataPadforcepowerSelect] ||
			cg_updatedDataPadForcePower2.integer - 1 == showDataPadPowers[cg.DataPadforcepowerSelect] ||
			cg_updatedDataPadForcePower3.integer - 1 == showDataPadPowers[cg.DataPadforcepowerSelect])
		{
			CG_DrawPic(centerXPos - bigIconSize / 2, graphicYPos - (bigIconSize - smallIconSize) / 2,
				bigIconSize,
				bigIconSize, cgs.media.DPForcePowerOverlay);
		}
	}

	i = cg.DataPadforcepowerSelect + 1;
	if (i >= MAX_DPSHOWPOWERS)
	{
		i = 0;
	}

	cgi_R_SetColor(colorTable[CT_WHITE]);

	// Work forwards from current icon
	holdX = centerXPos + bigIconSize / 2 + bigPad;
	for (iconCnt = 1; iconCnt < sideRightIconCnt + 1; i++)
	{
		if (i >= MAX_DPSHOWPOWERS)
		{
			i = 0;
		}

		if (!ForcePowerDataPad_Valid(i)) // Does he have this power?
		{
			continue;
		}

		++iconCnt; // Good icon

		if (force_icons[showDataPadPowers[i]])
		{
			CG_DrawPic(holdX, graphicYPos, smallIconSize, smallIconSize, force_icons[showDataPadPowers[i]]);
		}

		// A new force power
		if (cg_updatedDataPadForcePower1.integer - 1 == showDataPadPowers[i] ||
			cg_updatedDataPadForcePower2.integer - 1 == showDataPadPowers[i] ||
			cg_updatedDataPadForcePower3.integer - 1 == showDataPadPowers[i])
		{
			CG_DrawPic(holdX, graphicYPos, smallIconSize, smallIconSize, cgs.media.DPForcePowerOverlay);
		}

		if (force_icons[showDataPadPowers[i]])
		{
			holdX += smallIconSize + pad;
		}
	}

	if (!cgi_SP_GetStringTextString(va("SP_INGAME_%s", forcepowerDesc[cg.DataPadforcepowerSelect]), text,
		sizeof text))
	{
		cgi_SP_GetStringTextString(va("SPMOD_INGAME_%s", forcepowerDesc[cg.DataPadforcepowerSelect]), text,
			sizeof text);
	}

	if (player->client->ps.forcePowerLevel[showDataPadPowers[cg.DataPadforcepowerSelect]] == 1)
	{
		if (!cgi_SP_GetStringTextString(va("SP_INGAME_%s", forcepowerLvl1Desc[cg.DataPadforcepowerSelect]), text2,
			sizeof text2))
		{
			cgi_SP_GetStringTextString(va("SPMOD_INGAME_%s", forcepowerLvl1Desc[cg.DataPadforcepowerSelect]), text2,
				sizeof text2);
		}
	}
	else if (player->client->ps.forcePowerLevel[showDataPadPowers[cg.DataPadforcepowerSelect]] == 2)
	{
		if (!cgi_SP_GetStringTextString(va("SP_INGAME_%s", forcepowerLvl2Desc[cg.DataPadforcepowerSelect]), text2,
			sizeof text2))
		{
			cgi_SP_GetStringTextString(va("SPMOD_INGAME_%s", forcepowerLvl2Desc[cg.DataPadforcepowerSelect]), text2,
				sizeof text2);
		}
	}
	else
	{
		if (!cgi_SP_GetStringTextString(va("SP_INGAME_%s", forcepowerLvl3Desc[cg.DataPadforcepowerSelect]), text2,
			sizeof text2))
		{
			cgi_SP_GetStringTextString(va("SPMOD_INGAME_%s", forcepowerLvl3Desc[cg.DataPadforcepowerSelect]), text2,
				sizeof text2);
		}
	}

	if (text[0])
	{
		constexpr short textboxXPos = 40;
		constexpr short textboxYPos = 60;
		constexpr int textboxWidth = 560;
		constexpr int textboxHeight = 300;
		constexpr float textScale = 1.0f;

		CG_DisplayBoxedText(textboxXPos, textboxYPos, textboxWidth, textboxHeight, va("%s%s", text, text2),
			4,
			textScale,
			colorTable[CT_ICON_BLUE]
		);
	}
}