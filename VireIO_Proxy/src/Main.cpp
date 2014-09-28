/********************************************************************
Vireio Perception: Open-Source Stereoscopic 3D Driver
Copyright (C) 2012 Andres Hernandez

File <Main.cpp> :
Copyright (C) 2012 Andres Hernandez

Vireio Perception Version History:
v1.0.0 2012 by Andres Hernandez
v1.0.X 2013 by John Hicks, Neil Schneider
v1.1.x 2013 by Primary Coding Author: Chris Drain
Team Support: John Hicks, Phil Larkson, Neil Schneider
v2.0.x 2013 by Denis Reischl, Neil Schneider, Joshua Brown

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************/

#include <windows.h>
#include <d3d9.h>
#include "D3D9ProxyDirect3D.h"
#include <Vireio.h>
#include <qcoreapplication.h>
#include <qdebug.h>
#include <cConfig.h>
#include <qfileinfo.h>
#include <qmessagebox.h>


namespace{
	bool    loaded = false;

	IDirect3D9*   (WINAPI* ORIG_Direct3DCreate9   )( unsigned int nSDKVersion );
	HRESULT       (WINAPI* ORIG_Direct3DCreate9Ex )( unsigned int SDKVersion , IDirect3D9Ex **ppD3D );

	IDirect3D9* WINAPI NEW_Direct3DCreate9( unsigned int nSDKVersion ){
		printf("vireio: creating Direct3D\n" );

		//This workaround for steam GameOverlayRendereer hooks
		char signature[5] = { 0x8B , 0xFF , 0x55 , 0x8B , 0xEC };
		if( memcmp( ORIG_Direct3DCreate9 , signature , 5 ) != 0 ){
			printf("vireio: jmp-base another hook detected. reverting jmp hook back. THIS MAY CRASH!\n" );

			MEMORY_BASIC_INFORMATION mbi;
			VirtualQuery( ORIG_Direct3DCreate9 , &mbi , sizeof(MEMORY_BASIC_INFORMATION) );

			VirtualProtect( mbi.BaseAddress , mbi.RegionSize , PAGE_READWRITE , &mbi.Protect );

			memcpy( ORIG_Direct3DCreate9 , signature , 5 );

			DWORD ret;
			VirtualProtect( mbi.BaseAddress , mbi.RegionSize , mbi.Protect , &ret );
			
			printf("vireio: reverted.\n" );
		}


		IDirect3D9* ret = ORIG_Direct3DCreate9(nSDKVersion);
		if( !ret ){
			return 0;
		}
		
		return new D3D9ProxyDirect3D( ret , 0 , config );
	}

	HRESULT WINAPI NEW_Direct3DCreate9Ex( unsigned int nSDKVersion , IDirect3D9Ex** ret ){
		// Ex device somehow behave diferently (hl2.exe)
		printf("vireio: creating Direct3DEx (not supported)\n");
		return -1;
		//HRESULT result = ORIG_Direct3DCreate9Ex(nSDKVersion,ret);
		//if( SUCCEEDED(result) ){
		//	IDirect3D9Ex* proxy = PROXY_Direct3DCreate9( *ret , *ret );
		//	if( proxy ){
		//		*ret = proxy;
		//	}
		//}
		//return result;
	}

}



BOOL APIENTRY DllMain( HINSTANCE dll , DWORD fdwReason, LPVOID ){
	if( fdwReason == DLL_PROCESS_DETACH ){
		if( config.logToConsole && config.pauseOnLaunch ){
			MessageBoxA( 0 , "pause" , "click ok to exit proccess" , 0 );
		}
	}

	if( fdwReason != DLL_PROCESS_ATTACH ){
		return TRUE;
	}

	AllocConsole();
	freopen("CONOUT$", "w", stdout);

	static char exe_path[MAX_PATH];
	static char dll_path[MAX_PATH];

	GetModuleFileNameA( 0   , exe_path , MAX_PATH );
	GetModuleFileNameA( dll , dll_path , MAX_PATH );

	printf( "Vireio: loading...\n" );

	HijackHookInstall();

	config.vireioDir = QFileInfo(dll_path).absolutePath() + "/../";

	if( !config.load( config.getMainConfigFile() ) ){
		printf( "virieo: load general settings failed\n" );
		return TRUE;
	}

	if( !config.load( config.getGameConfigFile(exe_path) ) ){
		printf( "virieo: load game settings failed\n" );
		return TRUE;
	}

	if( !config.loadDevice( ) ){
		printf( "virieo: load device settings failed \n" );
		return TRUE;
	}

	if( !config.loadProfile( ) ){
		printf( "virieo: load profile settings failed\n" );
		return TRUE;
	}

	//load game settings again, in case if there any overrides for profile settings
	config.load( config.getGameConfigFile(exe_path) );

	config.calculateValues();

	printf( "stereoDevice                  = %s\n" , config.stereoDevice.toLocal8Bit().data() );
	printf( "trackerMode                   = %d\n" , config.trackerMode );
	printf( "logToConsole                  = %d\n" , config.logToConsole );
	printf( "logToFile                     = %d\n" , config.logToFile );
	printf( "logHijack                     = %d\n" , config.logHijack );
	printf( "pauseOnLaunch                 = %d\n" , config.pauseOnLaunch );
	printf( "streamingEnable               = %d\n" , config.streamingEnable );
	printf( "streamingAddress              = %s\n" , config.streamingAddress.toLocal8Bit().data() );
	printf( "streamingPort                 = %d\n" , config.streamingPort );
	printf( "streamingCodec                = %s\n" , config.streamingCodec.toLocal8Bit().data() );
	printf( "streamingBitrate              = %d\n" , config.streamingBitrate );
	printf( "showNotifications             = %d\n" , config.showNotifications );
	printf( "exePath                       = %s\n" , config.exePath.toLocal8Bit().data() );
	printf( "profileName                   = %s\n" , config.profileName.toLocal8Bit().data() );
	printf( "exeName                       = %s\n" , config.exeName.toLocal8Bit().data() );                   
	printf( "shaderRule                    = %s\n" , config.shaderRule.toLocal8Bit().data() );            
	printf( "VRboostRule                   = %s\n" , config.VRboostRule.toLocal8Bit().data() );               
	printf( "VRboostMinShaderCount         = %d\n" , config.VRboostMinShaderCount );     
	printf( "VRboostMaxShaderCount         = %d\n" , config.VRboostMaxShaderCount );     
	printf( "game_type                     = %d\n" , config.game_type );                 
	printf( "rollEnabled                   = %d\n" , config.rollEnabled );               
	printf( "worldScaleFactor              = %f\n" , config.worldScaleFactor );          
	printf( "convergence                   = %f\n" , config.convergence );               
	printf( "swap_eyes                     = %d\n" , config.swap_eyes );                 
	printf( "trackerYawMultiplier          = %f\n" , config.trackerYawMultiplier );            
	printf( "trackerPitchMultiplier        = %f\n" , config.trackerPitchMultiplier );          
	printf( "trackerRollMultiplier         = %f\n" , config.trackerRollMultiplier );           
	printf( "trackerPositionMultiplier     = %f\n" , config.trackerPositionMultiplier ); 
	printf( "trackerMouseYawMultiplier     = %f\n" , config.trackerMouseYawMultiplier );            
	printf( "trackerMousePitchMultiplier   = %f\n" , config.trackerMousePitchMultiplier );          
	printf( "trackerMouseEmulation         = %d\n" , config.trackerMouseEmulation );           
	printf( "DistortionScale               = %f\n" , config.DistortionScale );           
	printf( "YOffset                       = %f\n" , config.YOffset );                   
	printf( "IPDOffset                     = %f\n" , config.IPDOffset );                 
	printf( "hud3DDepthMode                = %d\n" , config.hud3DDepthMode );            
	printf( "hud3DDepthPresets             = %f\n" , config.hud3DDepthPresets[0] );   
	printf( "hudDistancePresets            = %f\n" , config.hudDistancePresets[0] ); 
	printf( "hudHotkeys                    = %d\n" , config.hudHotkeys[0] );            
	printf( "gui3DDepthMode                = %d\n" , config.gui3DDepthMode );            
	printf( "gui3DDepthPresets             = %f\n" , config.gui3DDepthPresets[0] );    
	printf( "guiSquishPresets              = %f\n" , config.guiSquishPresets[0] );     
	printf( "guiHotkeys                    = %d\n" , config.guiHotkeys );           
	printf( "VRBoostResetHotkey            = %d\n" , config.VRBoostResetHotkey );        
	printf( "WorldFOV                      = %f\n" , config.WorldFOV );                  
	printf( "PlayerFOV                     = %f\n" , config.PlayerFOV );                 
	printf( "FarPlaneFOV                   = %f\n" , config.FarPlaneFOV );               
	printf( "CameraTranslateX              = %f\n" , config.CameraTranslateX );          
	printf( "CameraTranslateY              = %f\n" , config.CameraTranslateY );          
	printf( "CameraTranslateZ              = %f\n" , config.CameraTranslateZ );          
	printf( "CameraDistance                = %f\n" , config.CameraDistance );            
	printf( "CameraZoom                    = %f\n" , config.CameraZoom );                
	printf( "CameraHorizonAdjustment       = %f\n" , config.CameraHorizonAdjustment );   
	printf( "ConstantValue1                = %f\n" , config.ConstantValue1 );            
	printf( "ConstantValue2                = %f\n" , config.ConstantValue2 );            
	printf( "ConstantValue3                = %f\n" , config.ConstantValue3 );            
	printf( "SteamAppId                    = %s\n" , config.SteamAppId.toLocal8Bit().data() );                
	printf( "CommandLineArguments          = %s\n" , config.CommandLineArguments.toLocal8Bit().data() );      
	printf( "shader                        = %s\n" , config.shader.toLocal8Bit().data() );
	printf( "isHmd                         = %d\n" , config.isHmd );
	printf( "resolutionWidth               = %d\n" , config.resolutionWidth );
	printf( "resolutionHeight              = %d\n" , config.resolutionHeight );
	printf( "physicalWidth                 = %f\n" , config.physicalWidth );
	printf( "physicalHeight                = %f\n" , config.physicalHeight );
	printf( "distortionCoefficients        = %f\n" , config.distortionCoefficients[0] );
	printf( "chromaCoefficients            = %f\n" , config.chromaCoefficients[0] );
	printf( "eyeToScreenDistance           = %f\n" , config.eyeToScreenDistance );
	printf( "physicalLensSeparation        = %f\n" , config.physicalLensSeparation );
	printf( "lensYCenterOffset             = %f\n" , config.lensYCenterOffset );
	printf( "lensIPDCenterOffset           = %f\n" , config.lensIPDCenterOffset );
	printf( "minDistortionScale            = %f\n" , config.minDistortionScale );
	printf( "chromaticAberrationCorrection = %d\n" , config.chromaticAberrationCorrection );
	printf( "useOvrDeviceSettings          = %d\n" , config.useOvrDeviceSettings );
	printf( "PlayerIPD                     = %f\n" , config.PlayerIPD );
	printf( "screenAspectRatio             = %f\n" , config.screenAspectRatio );
	printf( "scaleToFillHorizontal         = %f\n" , config.scaleToFillHorizontal );
	printf( "lensXCenterOffset             = %f\n" , config.lensXCenterOffset );


	HijackHookAdd( "d3d9.dll" , "Direct3DCreate9"        , (void**)&ORIG_Direct3DCreate9    , (void*)NEW_Direct3DCreate9   );
	HijackHookAdd( "d3d9.dll" , "Direct3DCreate9Ex"      , (void**)&ORIG_Direct3DCreate9Ex  , (void*)NEW_Direct3DCreate9Ex );


	if( !config.logToConsole ){
		fclose(stdout);
		FreeConsole();
	}

	if( config.logToFile ){
		freopen( "vireio_log.txt" , "w" , stdout );
	}

	if( config.pauseOnLaunch ){
		MessageBoxA( 0 , "pause" , "click ok to resume" , 0 );
	}

	LoadLibraryA( "d3d9.dll" );


	return TRUE;
}

