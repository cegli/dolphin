// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <cmath>

#include "Common/CommonTypes.h"
#include "Common/FileUtil.h"
#include "Common/IniFile.h"
#include "Common/StringUtil.h"
#include "Core/ConfigManager.h"
#include "Core/Core.h"
#include "Core/Movie.h"
#include "VideoCommon/OnScreenDisplay.h"
#include "VideoCommon/VideoCommon.h"
#include "VideoCommon/VideoConfig.h"
#include "VideoCommon/VR.h"

VideoConfig g_Config;
VideoConfig g_ActiveConfig;
// VR settings need to be saved manually, with a prompt if settings are changed. This detects when they have changed.
VideoConfig g_SavedConfig;

void UpdateActiveConfig()
{
	if (Movie::IsPlayingInput() && Movie::IsConfigSaved())
		Movie::SetGraphicsConfig();
	g_ActiveConfig = g_Config;
	if (g_has_hmd)
		g_ActiveConfig.bUseXFB = false;
}

VideoConfig::VideoConfig()
{
	bRunning = false;
	bFullscreen = false;

	// Needed for the first frame, I think
	fAspectRatioHackW = 1;
	fAspectRatioHackH = 1;

	// disable all features by default
	backend_info.APIType = API_NONE;
	backend_info.bUseMinimalMipCount = false;
	backend_info.bSupportsExclusiveFullscreen = false;

	// Game-specific stereoscopy settings
	bStereoMonoEFBDepth = false;
	iStereoSeparationPercent = 100;
	iStereoConvergencePercent = 100;

	// VR
	fScale = 1.0f;
	fLeanBackAngle = 0;
	bPullUp20fps = false;
	bPullUp30fps = false;
	bPullUp60fps = false;
	bEnableVR = true;
	bAsynchronousTimewarp = false;
	bLowPersistence = true;
	bDynamicPrediction = true;
	bOrientationTracking = true;
	bMagYawCorrection = true;
	bPositionTracking = true;
	bChromatic = true;
	bTimewarp = true;
	bVignette = false;
	bNoRestore = false;
	bFlipVertical = false;
	bSRGB = false;
	bOverdrive = true;
	bHqDistortion = false;
	iVRPlayer = 0;
	fTimeWarpTweak = DEFAULT_VR_TIMEWARP_TWEAK;
	iExtraFrames = DEFAULT_VR_EXTRA_FRAMES;
	iExtraVideoLoops = DEFAULT_VR_EXTRA_VIDEO_LOOPS;
	iExtraVideoLoopsDivider = DEFAULT_VR_EXTRA_VIDEO_LOOPS_DIVIDER;

	fUnitsPerMetre = DEFAULT_VR_UNITS_PER_METRE;
	// in metres
	fHudDistance = DEFAULT_VR_HUD_DISTANCE;
	fHudThickness = DEFAULT_VR_HUD_THICKNESS;
	fHud3DCloser = DEFAULT_VR_HUD_3D_CLOSER;
	fAimDistance = DEFAULT_VR_AIM_DISTANCE;
	fScreenDistance = DEFAULT_VR_SCREEN_DISTANCE;
	fScreenHeight = DEFAULT_VR_SCREEN_HEIGHT;
	iMetroidPrime = 0;
	iTelescopeEye = 0;
	fTelescopeMaxFOV = 0;

}

void VideoConfig::Load(const std::string& ini_file)
{
	IniFile iniFile;
	iniFile.Load(ini_file);

	IniFile::Section* hardware = iniFile.GetOrCreateSection("Hardware");
	hardware->Get("VSync", &bVSync, 0);
	hardware->Get("Adapter", &iAdapter, 0);

	IniFile::Section* settings = iniFile.GetOrCreateSection("Settings");
	settings->Get("wideScreenHack", &bWidescreenHack, false);
	settings->Get("AspectRatio", &iAspectRatio, (int)ASPECT_AUTO);
	settings->Get("Crop", &bCrop, false);
	settings->Get("UseXFB", &bUseXFB, 0);
	settings->Get("UseRealXFB", &bUseRealXFB, 0);
	settings->Get("SafeTextureCacheColorSamples", &iSafeTextureCache_ColorSamples,128);
	settings->Get("ShowFPS", &bShowFPS, false);
	settings->Get("LogRenderTimeToFile", &bLogRenderTimeToFile, false);
	settings->Get("OverlayStats", &bOverlayStats, false);
	settings->Get("OverlayProjStats", &bOverlayProjStats, false);
	settings->Get("ShowEFBCopyRegions", &bShowEFBCopyRegions, false);
	settings->Get("DumpTextures", &bDumpTextures, 0);
	settings->Get("HiresTextures", &bHiresTextures, 0);
	settings->Get("DumpEFBTarget", &bDumpEFBTarget, 0);
	settings->Get("FreeLook", &bFreeLook, 0);
	settings->Get("UseFFV1", &bUseFFV1, 0);
	settings->Get("EnablePixelLighting", &bEnablePixelLighting, 0);
	settings->Get("FastDepthCalc", &bFastDepthCalc, true);
	settings->Get("MSAA", &iMultisampleMode, 0);
	settings->Get("EFBScale", &iEFBScale, (int) SCALE_1X); // native
	settings->Get("DstAlphaPass", &bDstAlphaPass, false);
	settings->Get("TexFmtOverlayEnable", &bTexFmtOverlayEnable, 0);
	settings->Get("TexFmtOverlayCenter", &bTexFmtOverlayCenter, 0);
	settings->Get("WireFrame", &bWireFrame, 0);
	settings->Get("DisableFog", &bDisableFog, 0);
	settings->Get("EnableShaderDebugging", &bEnableShaderDebugging, false);
	settings->Get("BorderlessFullscreen", &bBorderlessFullscreen, false);

	IniFile::Section* enhancements = iniFile.GetOrCreateSection("Enhancements");
	enhancements->Get("ForceFiltering", &bForceFiltering, 0);
	enhancements->Get("MaxAnisotropy", &iMaxAnisotropy, 0);  // NOTE - this is x in (1 << x)
	enhancements->Get("PostProcessingShader", &sPostProcessingShader, "");
	enhancements->Get("StereoMode", &iStereoMode, 0);
	enhancements->Get("StereoSeparation", &iStereoSeparation, 20);
	enhancements->Get("StereoConvergence", &iStereoConvergence, 20);
	enhancements->Get("StereoSwapEyes", &bStereoSwapEyes, false);

	IniFile::Section* hacks = iniFile.GetOrCreateSection("Hacks");
	hacks->Get("EFBAccessEnable", &bEFBAccessEnable, true);
	hacks->Get("EFBCopyEnable", &bEFBCopyEnable, true);
	hacks->Get("EFBCopyClearDisable", &bEFBCopyClearDisable, false);
	hacks->Get("EFBToTextureEnable", &bCopyEFBToTexture, true);
	hacks->Get("EFBScaledCopy", &bCopyEFBScaled, true);
	hacks->Get("EFBCopyCacheEnable", &bEFBCopyCacheEnable, false);
	hacks->Get("EFBEmulateFormatChanges", &bEFBEmulateFormatChanges, false);

	LoadVR(File::GetUserPath(D_CONFIG_IDX) + "Dolphin.ini");

	// Load common settings
	iniFile.Load(File::GetUserPath(F_DOLPHINCONFIG_IDX));
	IniFile::Section* interface = iniFile.GetOrCreateSection("Interface");
	bool bTmp;
	interface->Get("UsePanicHandlers", &bTmp, true);
	SetEnableAlert(bTmp);

	// Shader Debugging causes a huge slowdown and it's easy to forget about it
	// since it's not exposed in the settings dialog. It's only used by
	// developers, so displaying an obnoxious message avoids some confusion and
	// is not too annoying/confusing for users.
	//
	// XXX(delroth): This is kind of a bad place to put this, but the current
	// VideoCommon is a mess and we don't have a central initialization
	// function to do these kind of checks. Instead, the init code is
	// triplicated for each video backend.
	if (bEnableShaderDebugging)
		OSD::AddMessage("Warning: Shader Debugging is enabled, performance will suffer heavily", 15000);
}

void VideoConfig::LoadVR(const std::string& ini_file)
{
	IniFile iniFile;
	iniFile.Load(ini_file);

	IniFile::Section* vr = iniFile.GetOrCreateSection("VR");
	vr->Get("Scale", &fScale, 1.0f);
	vr->Get("LeanBackAngle", &fLeanBackAngle, 0);
	vr->Get("EnableVR", &bEnableVR, true);
	vr->Get("LowPersistence", &bLowPersistence, true);
	vr->Get("DynamicPrediction", &bDynamicPrediction, true);
	vr->Get("OrientationTracking", &bOrientationTracking, true);
	vr->Get("MagYawCorrection", &bMagYawCorrection, true);
	vr->Get("PositionTracking", &bPositionTracking, true);
	vr->Get("Chromatic", &bChromatic, true);
	vr->Get("Timewarp", &bTimewarp, true);
	vr->Get("Vignette", &bVignette, false);
	vr->Get("NoRestore", &bNoRestore, false);
	vr->Get("FlipVertical", &bFlipVertical, false);
	vr->Get("sRGB", &bSRGB, false);
	vr->Get("Overdrive", &bOverdrive, true);
	vr->Get("HQDistortion", &bHqDistortion, false);
	vr->Get("Player", &iVRPlayer, 0);
	vr->Get("TimewarpTweak", &fTimeWarpTweak, DEFAULT_VR_TIMEWARP_TWEAK);
	vr->Get("NumExtraFrames", &iExtraFrames, DEFAULT_VR_EXTRA_FRAMES);
	vr->Get("NumExtraVideoLoops", &iExtraVideoLoops, DEFAULT_VR_EXTRA_VIDEO_LOOPS);
	vr->Get("NumExtraVideoLoopsDivider", &iExtraVideoLoopsDivider, DEFAULT_VR_EXTRA_VIDEO_LOOPS_DIVIDER);
	vr->Get("PullUp20fps", &bPullUp20fps, false);
	vr->Get("PullUp30fps", &bPullUp30fps, false);
	vr->Get("PullUp60fps", &bPullUp60fps, false);
}


void VideoConfig::GameIniLoad()
{
	bool gfx_override_exists = false;

	// XXX: Again, bad place to put OSD messages at (see delroth's comment above)
	// XXX: This will add an OSD message for each projection hack value... meh
#define CHECK_SETTING(section, key, var) do { \
		decltype(var) temp = var; \
		if (iniFile.GetIfExists(section, key, &var) && var != temp) { \
			std::string msg = StringFromFormat("Note: Option \"%s\" is overridden by game ini.", key); \
			OSD::AddMessage(msg, 7500); \
			gfx_override_exists = true; \
		} \
	} while (0)

	IniFile iniFile = SConfig::GetInstance().m_LocalCoreStartupParameter.LoadGameIni();

	CHECK_SETTING("Video_Hardware", "VSync", bVSync);

	CHECK_SETTING("Video_Settings", "wideScreenHack", bWidescreenHack);
	CHECK_SETTING("Video_Settings", "AspectRatio", iAspectRatio);
	CHECK_SETTING("Video_Settings", "Crop", bCrop);
	CHECK_SETTING("Video_Settings", "UseXFB", bUseXFB);
	CHECK_SETTING("Video_Settings", "UseRealXFB", bUseRealXFB);
	CHECK_SETTING("Video_Settings", "SafeTextureCacheColorSamples", iSafeTextureCache_ColorSamples);
	CHECK_SETTING("Video_Settings", "HiresTextures", bHiresTextures);
	CHECK_SETTING("Video_Settings", "EnablePixelLighting", bEnablePixelLighting);
	CHECK_SETTING("Video_Settings", "FastDepthCalc", bFastDepthCalc);
	CHECK_SETTING("Video_Settings", "MSAA", iMultisampleMode);
	int tmp = -9000;
	CHECK_SETTING("Video_Settings", "EFBScale", tmp); // integral
	if (tmp != -9000)
	{
		if (tmp != SCALE_FORCE_INTEGRAL)
		{
			iEFBScale = tmp;
		}
		else // Round down to multiple of native IR
		{
			switch (iEFBScale)
			{
			case SCALE_AUTO:
				iEFBScale = SCALE_AUTO_INTEGRAL;
				break;
			case SCALE_1_5X:
				iEFBScale = SCALE_1X;
				break;
			case SCALE_2_5X:
				iEFBScale = SCALE_2X;
				break;
			default:
				break;
			}
		}
	}

	CHECK_SETTING("Video_Settings", "DstAlphaPass", bDstAlphaPass);
	CHECK_SETTING("Video_Settings", "DisableFog", bDisableFog);

	CHECK_SETTING("Video_Enhancements", "ForceFiltering", bForceFiltering);
	CHECK_SETTING("Video_Enhancements", "MaxAnisotropy", iMaxAnisotropy);  // NOTE - this is x in (1 << x)
	CHECK_SETTING("Video_Enhancements", "PostProcessingShader", sPostProcessingShader);
	CHECK_SETTING("Video_Enhancements", "StereoMode", iStereoMode);
	CHECK_SETTING("Video_Enhancements", "StereoSeparation", iStereoSeparation);
	CHECK_SETTING("Video_Enhancements", "StereoConvergence", iStereoConvergence);
	CHECK_SETTING("Video_Enhancements", "StereoSwapEyes", bStereoSwapEyes);

	CHECK_SETTING("Video_Stereoscopy", "StereoMonoEFBDepth", bStereoMonoEFBDepth);
	CHECK_SETTING("Video_Stereoscopy", "StereoSeparationPercent", iStereoSeparationPercent);
	CHECK_SETTING("Video_Stereoscopy", "StereoConvergencePercent", iStereoConvergencePercent);

	CHECK_SETTING("Video_Hacks", "EFBAccessEnable", bEFBAccessEnable);
	CHECK_SETTING("Video_Hacks", "EFBCopyEnable", bEFBCopyEnable);
	CHECK_SETTING("Video_Hacks", "EFBCopyClearDisable", bEFBCopyClearDisable);
	CHECK_SETTING("Video_Hacks", "EFBToTextureEnable", bCopyEFBToTexture);
	CHECK_SETTING("Video_Hacks", "EFBScaledCopy", bCopyEFBScaled);
	CHECK_SETTING("Video_Hacks", "EFBCopyCacheEnable", bEFBCopyCacheEnable);
	CHECK_SETTING("Video_Hacks", "EFBEmulateFormatChanges", bEFBEmulateFormatChanges);

	CHECK_SETTING("Video", "ProjectionHack", iPhackvalue[0]);
	CHECK_SETTING("Video", "PH_SZNear", iPhackvalue[1]);
	CHECK_SETTING("Video", "PH_SZFar", iPhackvalue[2]);
	CHECK_SETTING("Video", "PH_ZNear", sPhackvalue[0]);
	CHECK_SETTING("Video", "PH_ZFar", sPhackvalue[1]);
	CHECK_SETTING("Video", "PerfQueriesEnable", bPerfQueriesEnable);

	fUnitsPerMetre = DEFAULT_VR_UNITS_PER_METRE;
	fHudDistance = DEFAULT_VR_HUD_DISTANCE;
	fHudThickness = DEFAULT_VR_HUD_THICKNESS;
	fHud3DCloser = DEFAULT_VR_HUD_3D_CLOSER;
	fCameraForward = DEFAULT_VR_CAMERA_FORWARD;
	fCameraPitch = DEFAULT_VR_CAMERA_PITCH;
	fAimDistance = DEFAULT_VR_AIM_DISTANCE;
	fScreenHeight = DEFAULT_VR_SCREEN_HEIGHT;
	fScreenDistance = DEFAULT_VR_SCREEN_DISTANCE;
	fScreenThickness = DEFAULT_VR_HUD_THICKNESS;
	fScreenRight = DEFAULT_VR_SCREEN_RIGHT;
	fScreenUp = DEFAULT_VR_SCREEN_UP;
	fScreenPitch = DEFAULT_VR_SCREEN_PITCH;
	bDisable3D = false;
	bHudFullscreen = false;
	bHudOnTop = false;
	iSelectedLayer = -1;
	iFlashState = 0;


	CHECK_SETTING("VR", "Disable3D", bDisable3D);
	CHECK_SETTING("VR", "HudFullscreen", bHudFullscreen);
	CHECK_SETTING("VR", "HudOnTop", bHudOnTop);
	CHECK_SETTING("VR", "UnitsPerMetre", fUnitsPerMetre);
	CHECK_SETTING("VR", "HudThickness", fHudThickness);
	CHECK_SETTING("VR", "HudDistance", fHudDistance);
	CHECK_SETTING("VR", "Hud3DCloser", fHud3DCloser);
	CHECK_SETTING("VR", "CameraForward", fCameraForward);
	CHECK_SETTING("VR", "CameraPitch", fCameraPitch);
	CHECK_SETTING("VR", "AimDistance", fAimDistance);
	CHECK_SETTING("VR", "ScreenHeight", fScreenHeight);
	CHECK_SETTING("VR", "ScreenThickness", fScreenThickness);
	CHECK_SETTING("VR", "ScreenDistance", fScreenDistance);
	CHECK_SETTING("VR", "ScreenRight", fScreenRight);
	CHECK_SETTING("VR", "ScreenUp", fScreenUp);
	CHECK_SETTING("VR", "ScreenPitch", fScreenPitch);
	CHECK_SETTING("VR", "MetroidPrime", iMetroidPrime);
	CHECK_SETTING("VR", "TelescopeEye", iTelescopeEye);
	CHECK_SETTING("VR", "TelescopeFOV", fTelescopeMaxFOV);

	NOTICE_LOG(VR, "%f units per metre (each unit is %f cm), HUD is %fm away and %fm thick", fUnitsPerMetre, 100.0f / fUnitsPerMetre, fHudDistance, fHudThickness);

	g_SavedConfig = *this;
	if (gfx_override_exists)
		OSD::AddMessage("Warning: Opening the graphics configuration will reset settings and might cause issues!", 10000);
}

void VideoConfig::GameIniSave()
{
	// Load game ini
	std::string GameIniFileDefault = SConfig::GetInstance().m_LocalCoreStartupParameter.m_strGameIniDefault;
	std::string GameIniFileDefaultRevisionSpecific = SConfig::GetInstance().m_LocalCoreStartupParameter.m_strGameIniDefaultRevisionSpecific;
	std::string GameIniFileLocal = SConfig::GetInstance().m_LocalCoreStartupParameter.m_strGameIniLocal;

	IniFile GameIniDefault, GameIniLocal;
	GameIniDefault.Load(GameIniFileDefault);
	if (GameIniFileDefaultRevisionSpecific != "")
		GameIniDefault.Load(GameIniFileDefaultRevisionSpecific, true);
	GameIniLocal.Load(GameIniFileLocal);

	#define SAVE_IF_NOT_DEFAULT(section, key, val, def) do { \
		if (GameIniDefault.Exists((section), (key))) { \
			std::remove_reference<decltype((val))>::type tmp__; \
			GameIniDefault.GetOrCreateSection((section))->Get((key), &tmp__); \
			if ((val) != tmp__) \
				GameIniLocal.GetOrCreateSection((section))->Set((key), (val)); \
			else \
				GameIniLocal.DeleteKey((section), (key)); \
		} else if ((val) != (def)) \
			GameIniLocal.GetOrCreateSection((section))->Set((key), (val)); \
		else \
			GameIniLocal.DeleteKey((section), (key)); \
	} while (0)

	SAVE_IF_NOT_DEFAULT("VR", "Disable3D", bDisable3D, false);
	SAVE_IF_NOT_DEFAULT("VR", "UnitsPerMetre", (float)fUnitsPerMetre, DEFAULT_VR_UNITS_PER_METRE);
	SAVE_IF_NOT_DEFAULT("VR", "HudFullscreen", bHudFullscreen, false);
	SAVE_IF_NOT_DEFAULT("VR", "HudOnTop", bHudOnTop, false);
	SAVE_IF_NOT_DEFAULT("VR", "HudDistance", (float)fHudDistance, DEFAULT_VR_HUD_DISTANCE);
	SAVE_IF_NOT_DEFAULT("VR", "HudThickness", (float)fHudThickness, DEFAULT_VR_HUD_THICKNESS);
	SAVE_IF_NOT_DEFAULT("VR", "Hud3DCloser", (float)fHud3DCloser, DEFAULT_VR_HUD_3D_CLOSER);
	SAVE_IF_NOT_DEFAULT("VR", "CameraForward", (float)fCameraForward, DEFAULT_VR_CAMERA_FORWARD);
	SAVE_IF_NOT_DEFAULT("VR", "CameraPitch", (float)fCameraPitch, DEFAULT_VR_CAMERA_PITCH);
	SAVE_IF_NOT_DEFAULT("VR", "AimDistance", (float)fAimDistance, DEFAULT_VR_AIM_DISTANCE);
	SAVE_IF_NOT_DEFAULT("VR", "ScreenHeight", (float)fScreenHeight, DEFAULT_VR_SCREEN_HEIGHT);
	SAVE_IF_NOT_DEFAULT("VR", "ScreenDistance", (float)fScreenDistance, DEFAULT_VR_SCREEN_DISTANCE);
	SAVE_IF_NOT_DEFAULT("VR", "ScreenThickness", (float)fScreenThickness, DEFAULT_VR_SCREEN_THICKNESS);
	SAVE_IF_NOT_DEFAULT("VR", "ScreenUp", (float)fScreenUp, DEFAULT_VR_SCREEN_UP);
	SAVE_IF_NOT_DEFAULT("VR", "ScreenRight", (float)fScreenRight, DEFAULT_VR_SCREEN_RIGHT);
	SAVE_IF_NOT_DEFAULT("VR", "ScreenPitch", (float)fScreenPitch, DEFAULT_VR_SCREEN_PITCH);
	GameIniLocal.Save(SConfig::GetInstance().m_LocalCoreStartupParameter.m_strGameIniLocal);
	g_SavedConfig = *this;
}

void VideoConfig::GameIniReset()
{
	// Load game ini
	std::string GameIniFileDefault = SConfig::GetInstance().m_LocalCoreStartupParameter.m_strGameIniDefault;
	std::string GameIniFileDefaultRevisionSpecific = SConfig::GetInstance().m_LocalCoreStartupParameter.m_strGameIniDefaultRevisionSpecific;

	IniFile GameIniDefault;
	GameIniDefault.Load(GameIniFileDefault);
	if (GameIniFileDefaultRevisionSpecific != "")
		GameIniDefault.Load(GameIniFileDefaultRevisionSpecific, true);

#define LOAD_DEFAULT(section, key, var, def) do { \
			decltype(var) temp = var; \
			if (GameIniDefault.GetIfExists(section, key, &var)) \
				var = temp; \
			else \
				var = def; \
	 	} while (0)

	LOAD_DEFAULT("VR", "Disable3D", bDisable3D, false);
	LOAD_DEFAULT("VR", "UnitsPerMetre", fUnitsPerMetre, DEFAULT_VR_UNITS_PER_METRE);
	LOAD_DEFAULT("VR", "HudFullscreen", bHudFullscreen, false);
	LOAD_DEFAULT("VR", "HudOnTop", bHudOnTop, false);
	LOAD_DEFAULT("VR", "HudDistance", fHudDistance, DEFAULT_VR_HUD_DISTANCE);
	LOAD_DEFAULT("VR", "HudThickness", fHudThickness, DEFAULT_VR_HUD_THICKNESS);
	LOAD_DEFAULT("VR", "Hud3DCloser", fHud3DCloser, DEFAULT_VR_HUD_3D_CLOSER);
	LOAD_DEFAULT("VR", "CameraForward", fCameraForward, DEFAULT_VR_CAMERA_FORWARD);
	LOAD_DEFAULT("VR", "CameraPitch", fCameraPitch, DEFAULT_VR_CAMERA_PITCH);
	LOAD_DEFAULT("VR", "AimDistance", fAimDistance, DEFAULT_VR_AIM_DISTANCE);
	LOAD_DEFAULT("VR", "ScreenHeight", fScreenHeight, DEFAULT_VR_SCREEN_HEIGHT);
	LOAD_DEFAULT("VR", "ScreenDistance", fScreenDistance, DEFAULT_VR_SCREEN_DISTANCE);
	LOAD_DEFAULT("VR", "ScreenThickness", fScreenThickness, DEFAULT_VR_SCREEN_THICKNESS);
	LOAD_DEFAULT("VR", "ScreenUp", fScreenUp, DEFAULT_VR_SCREEN_UP);
	LOAD_DEFAULT("VR", "ScreenRight", fScreenRight, DEFAULT_VR_SCREEN_RIGHT);
	LOAD_DEFAULT("VR", "ScreenPitch", fScreenPitch, DEFAULT_VR_SCREEN_PITCH);
}

void VideoConfig::VerifyValidity()
{
	// TODO: Check iMaxAnisotropy value
	if (iAdapter < 0 || iAdapter > ((int)backend_info.Adapters.size() - 1)) iAdapter = 0;
	if (iMultisampleMode < 0 || iMultisampleMode >= (int)backend_info.AAModes.size()) iMultisampleMode = 0;
	if (g_has_rift)
		iStereoMode = STEREO_OCULUS;
	else if (g_has_vr920)
		iStereoMode = STEREO_VR920;
	else if (iStereoMode == STEREO_OCULUS || iStereoMode == STEREO_VR920)
		iStereoMode = 0;
	if (!backend_info.bSupportsStereoscopy) iStereoMode = 0;
}

void VideoConfig::Save(const std::string& ini_file)
{
	IniFile iniFile;
	iniFile.Load(ini_file);

	IniFile::Section* hardware = iniFile.GetOrCreateSection("Hardware");
	hardware->Set("VSync", bVSync);
	hardware->Set("Adapter", iAdapter);

	IniFile::Section* settings = iniFile.GetOrCreateSection("Settings");
	settings->Set("AspectRatio", iAspectRatio);
	settings->Set("Crop", bCrop);
	settings->Set("wideScreenHack", bWidescreenHack);
	settings->Set("UseXFB", bUseXFB);
	settings->Set("UseRealXFB", bUseRealXFB);
	settings->Set("SafeTextureCacheColorSamples", iSafeTextureCache_ColorSamples);
	settings->Set("ShowFPS", bShowFPS);
	settings->Set("LogRenderTimeToFile", bLogRenderTimeToFile);
	settings->Set("OverlayStats", bOverlayStats);
	settings->Set("OverlayProjStats", bOverlayProjStats);
	settings->Set("DumpTextures", bDumpTextures);
	settings->Set("HiresTextures", bHiresTextures);
	settings->Set("DumpEFBTarget", bDumpEFBTarget);
	settings->Set("FreeLook", bFreeLook);
	settings->Set("UseFFV1", bUseFFV1);
	settings->Set("EnablePixelLighting", bEnablePixelLighting);
	settings->Set("FastDepthCalc", bFastDepthCalc);
	settings->Set("ShowEFBCopyRegions", bShowEFBCopyRegions);
	settings->Set("MSAA", iMultisampleMode);
	settings->Set("EFBScale", iEFBScale);
	settings->Set("TexFmtOverlayEnable", bTexFmtOverlayEnable);
	settings->Set("TexFmtOverlayCenter", bTexFmtOverlayCenter);
	settings->Set("Wireframe", bWireFrame);
	settings->Set("DstAlphaPass", bDstAlphaPass);
	settings->Set("DisableFog", bDisableFog);
	settings->Set("EnableShaderDebugging", bEnableShaderDebugging);
	settings->Set("BorderlessFullscreen", bBorderlessFullscreen);

	IniFile::Section* enhancements = iniFile.GetOrCreateSection("Enhancements");
	enhancements->Set("ForceFiltering", bForceFiltering);
	enhancements->Set("MaxAnisotropy", iMaxAnisotropy);
	enhancements->Set("PostProcessingShader", sPostProcessingShader);
	enhancements->Set("StereoMode", iStereoMode);
	enhancements->Set("StereoSeparation", iStereoSeparation);
	enhancements->Set("StereoConvergence", iStereoConvergence);
	enhancements->Set("StereoSwapEyes", bStereoSwapEyes);

	IniFile::Section* hacks = iniFile.GetOrCreateSection("Hacks");
	hacks->Set("EFBAccessEnable", bEFBAccessEnable);
	hacks->Set("EFBCopyEnable", bEFBCopyEnable);
	hacks->Set("EFBCopyClearDisable", bEFBCopyClearDisable);
	hacks->Set("EFBToTextureEnable", bCopyEFBToTexture);
	hacks->Set("EFBScaledCopy", bCopyEFBScaled);
	hacks->Set("EFBCopyCacheEnable", bEFBCopyCacheEnable);
	hacks->Set("EFBEmulateFormatChanges", bEFBEmulateFormatChanges);

	SaveVR(File::GetUserPath(D_CONFIG_IDX) + "Dolphin.ini");
	iniFile.Save(ini_file);
}

void VideoConfig::SaveVR(const std::string& ini_file)
{
	IniFile iniFile;
	iniFile.Load(ini_file);

	IniFile::Section* vr = iniFile.GetOrCreateSection("VR");
	vr->Set("Scale", fScale);
	vr->Set("LeanBackAngle", fLeanBackAngle);
	vr->Set("EnableVR", bEnableVR);
	vr->Set("LowPersistence", bLowPersistence);
	vr->Set("DynamicPrediction", bDynamicPrediction);
	vr->Set("OrientationTracking", bOrientationTracking);
	vr->Set("MagYawCorrection", bMagYawCorrection);
	vr->Set("PositionTracking", bPositionTracking);
	vr->Set("Chromatic", bChromatic);
	vr->Set("Timewarp", bTimewarp);
	vr->Set("Vignette", bVignette);
	vr->Set("NoRestore", bNoRestore);
	vr->Set("FlipVertical", bFlipVertical);
	vr->Set("sRGB", bSRGB);
	vr->Set("Overdrive", bOverdrive);
	vr->Set("HQDistortion", bHqDistortion);
	vr->Set("Player", iVRPlayer);
	vr->Set("TimewarpTweak", fTimeWarpTweak);
	vr->Set("NumExtraFrames", iExtraFrames);
	vr->Set("NumExtraVideoLoops", iExtraVideoLoops);
	vr->Set("NumExtraVideoLoopsDivider", iExtraVideoLoopsDivider);
	vr->Set("PullUp20fps", bPullUp20fps);
	vr->Set("PullUp30fps", bPullUp30fps);
	vr->Set("PullUp60fps", bPullUp60fps);

	iniFile.Save(ini_file);
}

bool VideoConfig::IsVSync()
{
	return bVSync && !Core::GetIsFramelimiterTempDisabled();
}

bool VideoConfig::VRSettingsModified()
{
	return fUnitsPerMetre != g_SavedConfig.fUnitsPerMetre
		|| fHudThickness != g_SavedConfig.fHudThickness
		|| fHudDistance != g_SavedConfig.fHudDistance
		|| fHud3DCloser != g_SavedConfig.fHud3DCloser
		|| fCameraForward != g_SavedConfig.fCameraForward
		|| fCameraPitch != g_SavedConfig.fCameraPitch
		|| fAimDistance != g_SavedConfig.fAimDistance
		|| fScreenHeight != g_SavedConfig.fScreenHeight
		|| fScreenThickness != g_SavedConfig.fScreenThickness
		|| fScreenDistance != g_SavedConfig.fScreenDistance
		|| fScreenRight != g_SavedConfig.fScreenRight
		|| fScreenUp != g_SavedConfig.fScreenUp
		|| fScreenPitch != g_SavedConfig.fScreenPitch
		|| fTelescopeMaxFOV != g_SavedConfig.fTelescopeMaxFOV
		|| bDisable3D != g_SavedConfig.bDisable3D
		|| bHudFullscreen != g_SavedConfig.bHudFullscreen
		|| bHudOnTop != g_SavedConfig.bHudOnTop
		|| iTelescopeEye != g_SavedConfig.iTelescopeEye
		|| iMetroidPrime != g_SavedConfig.iMetroidPrime;
}
