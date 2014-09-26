#pragma once
#include <qstring.h>


class cConfig {
public:
	QString vireioDir;
	
	//general settings
	QString stereoDevice;
	int     trackerMode;
	bool    logToConsole;
	bool    logToFile;
	bool    pauseOnLaunch;
	bool    streamingEnable;
	QString streamingAddress;
	int     streamingPort;
	QString streamingCodec;
	int     streamingBitrate;
	bool    showNotifications;

	//game/rprofile settings
	QString exePath;
	QString profileName;

	//profile settings
	QString exeName;                    // Game executable name
	QString shaderRulePath;             // Full path of shader rules for this game
	QString VRboostPath;                // Full path of VRboost rules for this game
	int     VRboostMinShaderCount;      // Minimum Vertex Shader Count to apply VRboost (security)
	int     VRboostMaxShaderCount;      // Maximum Vertex Shader Count to apply VRboost (security)
	int     game_type;                  // Game type enumeration. Matching D3DProxyDevice::ProxyTypes
	bool    rollEnabled;                // True if headtracking-roll is to be enabled
	float   worldScaleFactor;           // Value the eye seperation is to be multiplied with. (mm * worldScaleFactor = mm in game units)
	float   convergence;                // Convergence or Neutral Point distance, in meters
	bool    swap_eyes;                  // Option to swap the eye output for different 3D systems
	float   yaw_multiplier;             // Game-specific tracking multiplier (yaw)
	float   pitch_multiplier;           // Game-specific tracking multiplier (pitch)
	float   roll_multiplier;            // Game-specific tracking multiplier (roll)
	float   position_multiplier;        // Game-specific position multiplier (for X, Y and Z)
	float   DistortionScale;            // The scale to apply distortion
	float   YOffset;                    // The Y offset from the centre of the screen on the Y-axis *
	float   IPDOffset;                  // The IPD offset from the centre of the screen on the X-axis *
	int     hud3DDepthMode;             // Current HUD mode
	float   hud3DDepthPresets[4];       // HUD 3D Depth presets.
	float   hudDistancePresets[4];      // HUD Distance presets.
	int     hudHotkeys[5];              // HUD Hotkeys.
	int     gui3DDepthMode;             // Current GUI mode
	float   gui3DDepthPresets[4];       // GUI 3D Depth presets.
	float   guiSquishPresets[4];        // GUI Size presets.
	int     guiHotkeys[5];              // GUI Hotkeys.
	int     VRBoostResetHotkey;         // Hotkey option to reset VRBoost
	float   WorldFOV;                   // Game-specific global FOV setting
	float   PlayerFOV;                  // Game-specific player surroundings FOV setting
	float   FarPlaneFOV;                // Game-specific FOV setting for the most distant objects
	float   CameraTranslateX;           // Translates the camera along the X-Axis
	float   CameraTranslateY;           // Translates the camera along the Y-Axis
	float   CameraTranslateZ;           // Translates the camera along the Z-Axis
	float   CameraDistance;             // Changes the distance of the camera
	float   CameraZoom;                 // Changes the cameras zoom
	float   CameraHorizonAdjustment;    // Adjusts the cameras X axis rotation to match the horizon
	float   ConstantValue1;             // Game-specific constant value 1
	float   ConstantValue2;             // Game-specific constant value 2
	float   ConstantValue3;             // Game-specific constant value 3
	QString SteamAppId;                 // Game Steam app id (number only). Create game desktop shortcut from steam, open properties and find the number.
	QString CommandLineArguments;       // Arguments passed to executable

	//device settings
	QString      shader;
	bool         isHmd;
	int          resolutionWidth;
	int          resolutionHeight;
	float        physicalWidth;
	float        physicalHeight;
	float        distortionCoefficients[4];
	float        chromaCoefficients[4];
	float        eyeToScreenDistance;
	float        physicalLensSeparation;
	float        lensYCenterOffset;
	float        lensIPDCenterOffset;
	float        minDistortionScale;
	bool         chromaticAberrationCorrection;
	
	//user settings
	float        PlayerIPD;

	//calculated values
	float        screenAspectRatio;
	float        scaleToFillHorizontal;
	float        lensXCenterOffset;

	enum{
		SAVE_GENERAL  = 1,
		SAVE_GAME     = 2,
		SAVE_PROFILE  = 4,
		SAVE_DEVICE   = 8
	};

	cConfig();

	bool load( const QString& file );
	bool save( const QString& file , int flags );

	bool loadProfile( );
	bool saveProfile( );
	bool loadDevice ( );
	bool loadOculusSdk( );

	void        calculateValues();
	QString     getMainConfigFile( );
	QString     getGameConfigFile( const QString& gameExePath );
	QString     getProfileConfigFile( );
	QString     findProfileFileForExe( const QString& gameExePath );
	QStringList getAvailableProfiles( );
	QStringList getAvailableDevices ( );
};
