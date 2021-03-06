/********************************************************************
Vireio Perception: Open-Source Stereoscopic 3D Driver
Copyright (C) 2012 Andres Hernandez

File <Vireio.h> and
Namespace <vireio> :
Copyright (C) 2012 Andres Hernandez
Modifications Copyright (C) 2013 Chris Drain

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


#pragma once
#include <qstringlist.h>
#include <qstring.h>
#include <qdatetime.h>
#include <d3d9.h>
#include <assert.h>

// 64mm in meters
#define IPD_DEFAULT 0.064f


template<class T>
void SAFE_RELEASE( T& x ){
	if(x){
		x->Release();
		x=0;
	}
}



template<class T>
void SAFE_RELEASE( QList<T>& x ){
	for( T& v : x ){
		SAFE_RELEASE( v );
	}
}

template<class T,class K>
void SAFE_RELEASE( QMap<K,T>& x ){
	for( T& v : x.values() ){
		SAFE_RELEASE( v );
	}
	x.clear();
}


template<class T>
void SAFE_ASSIGN( T& x , T n ){
	SAFE_RELEASE( x );
	x = n;
	if( x ){
		x->AddRef();
	}
}

template<class T,class K>
void SAFE_ASSIGN( QMap<K,T>& x , const QMap<K,T>& n ){
	SAFE_RELEASE( x );
	for( K k : n.keys() ){
		x[k] = n[k];
		if( x[k] ){
			x[k]->AddRef();
		}
	}
}


template<class T>
class cPtr{
public:

	cPtr( ){
		p = 0;
	}

	cPtr( T* ptr ){
		p = 0;
		(*this) = ptr;
	}

	cPtr( const cPtr<T>& other ){
		p = 0;
		(*this) = other;
	}

	~cPtr(){
		clear();
	}

	T* operator->(){
		return p;
	}

	operator T*(){
		return p;
	}

	T** clearAndGetPtr(){
		clear();
		return &p;
	}


	void operator = ( T* ptr ){
		if( ptr ){
			ptr->AddRef();
		}

		clear();

		p = ptr;
	}


	void operator = (  const cPtr<T>& other ){
		(*this) = other.p;
	}


	void clear( ){
		if( p ){
			p->Release();
			p = 0;
		}
	}

	void releaseAndDelete( ){
		if( p ){
			p->Release();
			delete p;
			p = 0;
		}
	}

	void detach( ){
		p = 0;
	}

	operator bool(){
		return p;
	}

private:
	T* p;
};












namespace vireio {
	enum RenderPosition
	{
		// probably need an 'Original' here
		Left = 1,
		Right = 2		
	};

	void UnWrapTexture(IDirect3DBaseTexture9* pWrappedTexture, IDirect3DBaseTexture9** ppActualLeftTexture, IDirect3DBaseTexture9** ppActualRightTexture);
	bool AlmostSame(float a, float b, float epsilon);
	void clamp(float* toClamp, float min, float max);
};


QStringList HijackListProcesses( );
QString     HijackAttachToProcess( int index         );
QString     HijackLaunchProcess  ( QString exe_path , QString args , QStringList environment );

QString     HijackInject     ( HANDLE proccess );
void        HijackHookAdd    ( const char* moduleName , const char* functionName , void** oldFunctionPtr  , void* newFunction );
void        HijackHookUpdate ( );
void        HijackHookInstall( );

#include "cConfig.h"
extern cConfig config;
extern cConfig configBackup;


class D3D9ProxySurface;
class D3D9ProxyTexture;
class D3D9ProxyVolumeTexture;
class D3D9ProxyCubeTexture;
class D3D9ProxyStateBlock;
class D3D9ProxySwapChain;
class D3D9ProxyVertexShader;
class D3D9ProxyPixelShader;
class D3D9ProxyDirect3D;
class D3D9ProxyVertexBuffer;
class D3D9ProxyIndexBuffer;
class D3D9ProxyVertexDeclaration;
class D3D9ProxyQuery;

class StereoBackBuffer;
class StereoView;
class MotionTracker;
class ViewAdjustment;
class cTracker;

cTracker* Vireio_Create_Tracker_FreeSpace();
cTracker* Vireio_Create_Tracker_FreeTrack();
cTracker* Vireio_Create_Tracker_SharedMemory();
cTracker* Vireio_Create_Tracker_Oculus();


QStringList       Vireio_getDisplayAdapters();
std::vector<RECT> Vireio_getDesktops();



class cTimingHelper{
public:
	LARGE_INTEGER i1;
	LARGE_INTEGER i2;
	const char*   name;

	static inline int& depth(){
		static int i;
		return i;
	}

	cTimingHelper( const char* n ){
		if( config.logVerbose ){
			name = n;
			QueryPerformanceCounter( &i1 );

			for( int c=0 ; c<depth() ; c++ ){
				printf( " " , c );
			}

			printf( "Begin %s\n" , name );

			depth()++;

		}
	}

	~cTimingHelper( ){
		if( config.logVerbose ){
			QueryPerformanceCounter( &i2 );

			depth()--;
			
			for( int c=0 ; c<depth() ; c++ ){
				printf( " " , c );
			}

			printf( "End   %s - %lld\n" , name , i2.QuadPart - i1.QuadPart );
		}
	}
};




#define EXPAND(X) X

#define METHOD_EXPAND_0(F,A)      
#define METHOD_EXPAND_1( F,A,B)      F(A,B)
#define METHOD_EXPAND_2( F,A,B,...)  F(A,B) , EXPAND(METHOD_EXPAND_1(F,__VA_ARGS__))
#define METHOD_EXPAND_3( F,A,B,...)  F(A,B) , EXPAND(METHOD_EXPAND_2(F,__VA_ARGS__))
#define METHOD_EXPAND_4( F,A,B,...)  F(A,B) , EXPAND(METHOD_EXPAND_3(F,__VA_ARGS__))
#define METHOD_EXPAND_5( F,A,B,...)  F(A,B) , EXPAND(METHOD_EXPAND_4(F,__VA_ARGS__))
#define METHOD_EXPAND_6( F,A,B,...)  F(A,B) , EXPAND(METHOD_EXPAND_5(F,__VA_ARGS__))
#define METHOD_EXPAND_7( F,A,B,...)  F(A,B) , EXPAND(METHOD_EXPAND_6(F,__VA_ARGS__))
#define METHOD_EXPAND_8( F,A,B,...)  F(A,B) , EXPAND(METHOD_EXPAND_7(F,__VA_ARGS__))
#define METHOD_EXPAND_9( F,A,B,...)  F(A,B) , EXPAND(METHOD_EXPAND_8(F,__VA_ARGS__))
#define METHOD_EXPAND_10(F,A,B,...)  F(A,B) , EXPAND(METHOD_EXPAND_9(F,__VA_ARGS__))
#define METHOD_EXPAND_11(F,A,B,...)  F(A,B) , EXPAND(METHOD_EXPAND_10(F,__VA_ARGS__))
#define METHOD_EXPAND_12(F,A,B,...)  F(A,B) , EXPAND(METHOD_EXPAND_11(F,__VA_ARGS__))

#define METHOD_EXPAND_F12(A,B) A B
#define METHOD_EXPAND_F2(A,B) B

#define METHOD_SELECT_EXPAND(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,A11,A12,A13,A14,A15,A16,A17,A18,A19,A20,A21,A22,A23,A24,A25,A26,...) A26
#define METHOD_EXPAND_FUNC(F,...)					   \
	EXPAND(											   \
		EXPAND(										   \
			METHOD_SELECT_EXPAND(					   \
				__VA_ARGS__ ,						   \
				METHOD_EXPAND_12,METHOD_EXPAND_12 ,	   \
				METHOD_EXPAND_11,METHOD_EXPAND_11 ,	   \
				METHOD_EXPAND_10,METHOD_EXPAND_10 ,	   \
				METHOD_EXPAND_9 ,METHOD_EXPAND_9 ,	   \
				METHOD_EXPAND_8 ,METHOD_EXPAND_8 ,	   \
				METHOD_EXPAND_7 ,METHOD_EXPAND_7 ,	   \
				METHOD_EXPAND_6 ,METHOD_EXPAND_6 ,	   \
				METHOD_EXPAND_5 ,METHOD_EXPAND_5 ,	   \
				METHOD_EXPAND_4 ,METHOD_EXPAND_4 ,	   \
				METHOD_EXPAND_3 ,METHOD_EXPAND_3 ,	   \
				METHOD_EXPAND_2 ,METHOD_EXPAND_2 ,	   \
				METHOD_EXPAND_1 ,METHOD_EXPAND_1 ,	   \
				METHOD_EXPAND_0 ,METHOD_EXPAND_0	   \
			)										   \
		)											   \
		(F,__VA_ARGS__)								   \
	)

#define METHOD_EXPAND_ALL(...)  METHOD_EXPAND_FUNC( METHOD_EXPAND_F12 , __VA_ARGS__ )
#define METHOD_EXPAND_NAME(...) METHOD_EXPAND_FUNC( METHOD_EXPAND_F2  , __VA_ARGS__ )



//uncomment second to enable VERY verbose output (and comment first)
#define METHOD_LOG(ret,spec,base,name,...) cTimingHelper __vireio_timing_helper(#name);
//#define METHOD_LOG(ret,spec,base,name,...) printf( #base "::" #name "\n" );


//This macro expand to method declaration
#define METHOD_IMPL(ret,spec,base,name,...)                     \
	ret spec base::name( METHOD_EXPAND_ALL(__VA_ARGS__) ){ METHOD_LOG(ret,spec,base,name,__VA_ARGS__)

	//This macro expand to method declaration and calls same method of  "actualEx"
#define METHOD_THRU_EX(ret,spec,base,name,...)		              \
	ret spec base::name( METHOD_EXPAND_ALL(__VA_ARGS__) ){		  \
		METHOD_LOG(ret,spec,base,name,__VA_ARGS__)                \
		return actualEx->name( METHOD_EXPAND_NAME(__VA_ARGS__) ); \
	}

//This macro expand to method declaration and calls same method of "actual"
#define METHOD_THRU(ret,spec,base,name,...)				        \
	ret spec base::name( METHOD_EXPAND_ALL(__VA_ARGS__) ){		\
		METHOD_LOG(ret,spec,base,name,__VA_ARGS__)              \
		return actual->name( METHOD_EXPAND_NAME(__VA_ARGS__) );	\
	}

//This macro expand to method declaration and calls same method of "right" and "actual"
#define METHOD_THRU_LR(ret,spec,base,name,...)                  \
	ret spec base::name( METHOD_EXPAND_ALL(__VA_ARGS__) ){ 		\
		METHOD_LOG(ret,spec,base,name,__VA_ARGS__)              \
		if( right ){ 											\
			right->name( METHOD_EXPAND_NAME(__VA_ARGS__) );		\
		}														\
		return actual->name( METHOD_EXPAND_NAME(__VA_ARGS__) );	\
	}



#define M_PI                    3.141592654
#define RADIANS_TO_DEGREES(rad) ((float) rad * (float) (180.0 / M_PI))
