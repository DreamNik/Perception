/********************************************************************
Vireio Perception: Open-Source Stereoscopic 3D Driver
Copyright (C) 2012 Andres Hernandez

File <SharedMemoryTracker.cpp> and
Class <SharedMemoryTracker> :
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

#include <VireIO.h>
#include <cTracker.h>
#include <Windows.h>
#include <string>
#include <iostream>


static char szName[] = "VireioSMTrack" ;

namespace{
	struct TrackData{
		int DataID;	/**< increased every time data has been sent */

		float Yaw;
		float Pitch;
		float Roll;

		float X;
		float Y;
		float Z;
	};
}




class cTracker_smt : public cTracker {
public:

	bool       openSharedMemory();
	HANDLE     hMapFile;
	TrackData* pTrackBuf;


	cTracker_smt(){
		hMapFile = NULL;
		pTrackBuf = NULL;
	}


	~cTracker_smt(){
		close();
	}


	bool open( ) override{
		hMapFile = CreateFileMapping(
			INVALID_HANDLE_VALUE,	// use paging file
			NULL,					// default security
			PAGE_READWRITE,			// read/write access
			0,						// maximum object size (high-order DWORD)
			sizeof(TrackData),		// maximum object size (low-order DWORD)
			szName);				// name of mapping object

		if (hMapFile == NULL)										// Could not create file mapping object
			return false;

		pTrackBuf = (TrackData*) MapViewOfFile(hMapFile,			// handle to map object
			FILE_MAP_ALL_ACCESS,									// read/write permission
			0,
			0,
			sizeof(TrackData));

		if (pTrackBuf == NULL)										// Could not map view of file
		{
			CloseHandle(hMapFile);
			return false;
		}

		return true;
	}


	void close( ) override{
		UnmapViewOfFile(pTrackBuf);
		CloseHandle(hMapFile);
	}


	bool update( ) override{
		if( !pTrackBuf ){
			return false;
		}

		currentYaw   = -pTrackBuf->Yaw;
		currentPitch =  pTrackBuf->Pitch;
		currentRoll  = -pTrackBuf->Roll;

		// Convert yaw, pitch to positive degrees.
		// (-180.0f...0.0f -> 180.0f....360.0f)
		// (0.0f...180.0f -> 0.0f...180.0f)
		currentYaw   =  fmodf(currentYaw + 2*M_PI , 2*M_PI );
		currentPitch = -fmodf(currentYaw + 2*M_PI , 2*M_PI );
		return true;
	}
};


cTracker* VireIO_Create_Tracker_SharedMemory(){
	return new cTracker_smt;
}


/*
		// Get difference.
		deltaYaw   += yaw - currentYaw;
		deltaPitch += pitch - currentPitch;

		// hack to avoid errors while translating over 360/0
		if(fabs(deltaYaw) > 4.0f) deltaYaw = 0.0f;
		if(fabs(deltaPitch) > 4.0f) deltaPitch = 0.0f;

		// Pass to mouse data (long integer).
		mouseData.mi.dx = (long)(deltaYaw*multiplierYaw);
		mouseData.mi.dy = (long)(deltaPitch*multiplierPitch);

		// Keep fractional difference in the delta so it's added to the next update.
		deltaYaw -= ((float)mouseData.mi.dx)/multiplierYaw;
		deltaPitch -= ((float)mouseData.mi.dy)/multiplierPitch;

#ifdef _DEBUG
		OutputDebugStringA("Motion Tracker SendInput\n");
#endif
		// Send to mouse input
		if (mouseEmulation)
			SendInput(1, &mouseData, sizeof(INPUT));

		// Set current data.
		currentYaw = yaw;
		currentPitch = pitch;
		currentRoll = (float)( roll * (PI/180.0) * multiplierRoll);	// convert from deg to radians then apply mutiplier
	}
}
*/