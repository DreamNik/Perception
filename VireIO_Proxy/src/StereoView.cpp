/********************************************************************
Vireio Perception: Open-Source Stereoscopic 3D Driver
Copyright (C) 2012 Andres Hernandez

File <StereoView.cpp> and
Class <StereoView> :
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

#include "StereoView.h"
#include <Streamer.h>
#include "D3D9ProxySurface.h"

StereoView::StereoView( ){
	OutputDebugStringA("Created SteroView\n");
	initialized = false;
	XOffset = 0;

	// set all member pointers to NULL to prevent uninitialized objects being used
	m_pActualDevice = NULL;
	backBuffer = NULL;
	leftTexture = NULL;
	rightTexture = NULL;

	leftSurface = NULL;
	rightSurface = NULL;

	screenVertexBuffer = NULL;
	lastVertexShader = NULL;
	lastPixelShader = NULL;
	lastTexture = NULL;
	lastTexture1 = NULL;
	lastVertexDeclaration = NULL;
	lastRenderTarget0 = NULL;
	lastRenderTarget1 = NULL;
	viewEffect = NULL;
	sb = NULL;

	// set behavior accordingly to game type

	switch(config.game_type)
	{
	case D3DProxyDevice::FIXED:
		howToSaveRenderStates = HowToSaveRenderStates::STATE_BLOCK;
		break;
	case D3DProxyDevice::SOURCE:
	case D3DProxyDevice::SOURCE_L4D:
	case D3DProxyDevice::SOURCE_ESTER:
		howToSaveRenderStates = HowToSaveRenderStates::SELECTED_STATES_MANUALLY;
		break;
	case D3DProxyDevice::UNREAL:
	case D3DProxyDevice::UNREAL_MIRROR:
	case D3DProxyDevice::UNREAL_UT3:
	case D3DProxyDevice::UNREAL_BIOSHOCK:
		howToSaveRenderStates = HowToSaveRenderStates::STATE_BLOCK;
		break;
	case D3DProxyDevice::UNREAL_BORDERLANDS:
		howToSaveRenderStates = HowToSaveRenderStates::DO_NOT_SAVE_AND_RESTORE;
		break;
	case D3DProxyDevice::EGO:
	case D3DProxyDevice::EGO_DIRT:
		howToSaveRenderStates = HowToSaveRenderStates::STATE_BLOCK;
		break;
	case D3DProxyDevice::REALV:
	case D3DProxyDevice::REALV_ARMA:
		howToSaveRenderStates = HowToSaveRenderStates::STATE_BLOCK;
		break;
	case D3DProxyDevice::UNITY:
	case D3DProxyDevice::UNITY_SLENDER:
		howToSaveRenderStates = HowToSaveRenderStates::STATE_BLOCK;
		break;
	case D3DProxyDevice::GAMEBRYO:
	case D3DProxyDevice::GAMEBRYO_SKYRIM:
		howToSaveRenderStates = HowToSaveRenderStates::STATE_BLOCK;
		break;
	case D3DProxyDevice::LFS:
		howToSaveRenderStates = HowToSaveRenderStates::STATE_BLOCK;
		break;
	case D3DProxyDevice::CDC:
		howToSaveRenderStates = HowToSaveRenderStates::STATE_BLOCK;
		break;
	default:
		howToSaveRenderStates = HowToSaveRenderStates::STATE_BLOCK;
		break;
	}

	m_pStreamer = new Streamer( );
}


StereoView::~StereoView(){
	delete m_pStreamer;
}


void StereoView::Init(IDirect3DDevice9* pActualDevice){
	if( initialized ){
		return;
	}

	m_pActualDevice = pActualDevice;

	InitShaderEffects();
	InitTextureBuffers();
	InitVertexBuffers();
	CalculateShaderVariables();

	initialized = true;
}

/**
* Releases all Direct3D objects.
***/
void StereoView::ReleaseEverything(){
	SAFE_RELEASE( backBuffer )
	SAFE_RELEASE( leftTexture )
	SAFE_RELEASE( rightTexture )
	SAFE_RELEASE( leftSurface )
	SAFE_RELEASE( rightSurface )
	SAFE_RELEASE( lastVertexShader )
	SAFE_RELEASE( lastPixelShader )
	SAFE_RELEASE( lastTexture )
	SAFE_RELEASE( lastTexture1 )
	SAFE_RELEASE( lastVertexDeclaration )
	SAFE_RELEASE( lastRenderTarget0 )
	SAFE_RELEASE( lastRenderTarget1 )

	viewEffect->OnLostDevice();

	initialized = false;
}

/**
* Draws stereoscopic frame.
***/
void StereoView::Draw(D3D9ProxySurface* stereoCapableSurface)
{
	// Copy left and right surfaces to textures to use as shader input
	// TODO match aspect ratio of source in target ? 
	IDirect3DSurface9* leftImage = stereoCapableSurface->actual;
	IDirect3DSurface9* rightImage = stereoCapableSurface->right;

	m_pActualDevice->StretchRect(leftImage, NULL, leftSurface, NULL, D3DTEXF_NONE);

	if (stereoCapableSurface->right)
		m_pActualDevice->StretchRect(rightImage, NULL, rightSurface, NULL, D3DTEXF_NONE);
	else
		m_pActualDevice->StretchRect(leftImage, NULL, rightSurface, NULL, D3DTEXF_NONE);

	// how to save (backup) render states ?
	switch(howToSaveRenderStates)
	{
	case HowToSaveRenderStates::STATE_BLOCK:
		m_pActualDevice->CreateStateBlock(D3DSBT_ALL, &sb);
		break;
	case HowToSaveRenderStates::SELECTED_STATES_MANUALLY:
		SaveState();
		break;
	case HowToSaveRenderStates::ALL_STATES_MANUALLY:
		SaveAllRenderStates(m_pActualDevice);
		SetAllRenderStatesDefault(m_pActualDevice);
		break;
	case HowToSaveRenderStates::DO_NOT_SAVE_AND_RESTORE:
		break;
	}

	// set states for fullscreen render
	SetState();

	// all render settings start here
	m_pActualDevice->SetFVF(D3DFVF_TEXVERTEX);

	// swap eyes
	if(!config.swap_eyes)
	{
		m_pActualDevice->SetTexture(0, leftTexture);
		m_pActualDevice->SetTexture(1, rightTexture);
	}
	else 
	{
		m_pActualDevice->SetTexture(0, rightTexture);
		m_pActualDevice->SetTexture(1, leftTexture);
	}

	if (FAILED(m_pActualDevice->SetRenderTarget(0, backBuffer))) {
		OutputDebugStringA("SetRenderTarget backbuffer failed\n");
	}

	if (FAILED(m_pActualDevice->SetStreamSource(0, screenVertexBuffer, 0, sizeof(TEXVERTEX)))) {
		OutputDebugStringA("SetStreamSource failed\n");
	}

	UINT iPass, cPasses;

	if (FAILED(viewEffect->SetTechnique("ViewShader"))) {
		OutputDebugStringA("SetTechnique failed\n");
	}

	SetViewEffectInitialValues();

	// now, render
	if (FAILED(viewEffect->Begin(&cPasses, 0))) {
		OutputDebugStringA("Begin failed\n");
	}

	for(iPass = 0; iPass < cPasses; iPass++)
	{
		if (FAILED(viewEffect->BeginPass(iPass))) {
			OutputDebugStringA("Beginpass failed\n");
		}

		if (FAILED(m_pActualDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, 2))) {
			OutputDebugStringA("Draw failed\n");
		}

		if (FAILED(viewEffect->EndPass())) {
			OutputDebugStringA("Beginpass failed\n");
		}
	}

	if (FAILED(viewEffect->End())) {
		OutputDebugStringA("End failed\n");
	}

	m_pStreamer->send( m_pActualDevice );

	// how to restore render states ?
	switch(howToSaveRenderStates)
	{
	case HowToSaveRenderStates::STATE_BLOCK:
		// apply stored render states
		sb->Apply();
		sb->Release();
		sb = NULL;
		break;
	case HowToSaveRenderStates::SELECTED_STATES_MANUALLY:
		RestoreState();
		break;
	case HowToSaveRenderStates::ALL_STATES_MANUALLY:
		RestoreAllRenderStates(m_pActualDevice);
		break;
	case HowToSaveRenderStates::DO_NOT_SAVE_AND_RESTORE:
		break;
	}
}

/**
* Saves screenshot and shot of left and right surface.
***/
void StereoView::SaveScreen()
{
	static int screenCount = 0;
	++screenCount;

	char fileName[32];
	sprintf(fileName, "%d_final.bmp", screenCount);
	char fileNameLeft[32];
	sprintf(fileNameLeft, "%d_left.bmp", screenCount);
	char fileNameRight[32];
	sprintf(fileNameRight, "%d_right.bmp", screenCount);

#ifdef _DEBUG
	OutputDebugStringA(fileName);
	OutputDebugStringA("\n");
#endif	

	D3DXSaveSurfaceToFileA(fileNameLeft, D3DXIFF_BMP, leftSurface, NULL, NULL);
	D3DXSaveSurfaceToFileA(fileNameRight, D3DXIFF_BMP, rightSurface, NULL, NULL);
	D3DXSaveSurfaceToFileA(fileName, D3DXIFF_BMP, backBuffer, NULL, NULL);
}

/**
* Calls ID3DXEffect::OnResetDevice.
***/
void StereoView::PostReset()
{
	CalculateShaderVariables();
	viewEffect->OnResetDevice();
}

/**
* Inits the left and right texture buffer.
* Also gets viewport data and back buffer render target.
***/
void StereoView::InitTextureBuffers()
{
	m_pActualDevice->GetViewport(&viewport);
	D3DSURFACE_DESC pDesc = D3DSURFACE_DESC();
	m_pActualDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);
	backBuffer->GetDesc(&pDesc);

#ifdef _DEBUG
	char buf[32];
	LPCSTR psz = NULL;

	sprintf(buf,"viewport width: %d",viewport.Width);
	psz = buf;
	OutputDebugStringA(psz);
	OutputDebugStringA("\n");

	sprintf(buf,"backbuffer width: %d",pDesc.Width);
	psz = buf;
	OutputDebugStringA(psz);
	OutputDebugStringA("\n");
#endif

	m_pActualDevice->CreateTexture(pDesc.Width, pDesc.Height, 0, D3DUSAGE_RENDERTARGET, pDesc.Format, D3DPOOL_DEFAULT, &leftTexture, NULL);
	leftTexture->GetSurfaceLevel(0, &leftSurface);

	m_pActualDevice->CreateTexture(pDesc.Width, pDesc.Height, 0, D3DUSAGE_RENDERTARGET, pDesc.Format, D3DPOOL_DEFAULT, &rightTexture, NULL);
	rightTexture->GetSurfaceLevel(0, &rightSurface);
}

/**
* Inits a simple full screen vertex buffer containing 4 vertices.
***/
void StereoView::InitVertexBuffers()
{
	OutputDebugStringA("SteroView initVertexBuffers\n");

	HRESULT result = m_pActualDevice->CreateVertexBuffer(sizeof(TEXVERTEX) * 4, NULL,
		D3DFVF_TEXVERTEX, D3DPOOL_MANAGED , &screenVertexBuffer, NULL);

	if( FAILED(result) ){
		OutputDebugStringA("SteroView initVertexBuffers failed\n");
	}

	TEXVERTEX* vertices;

	screenVertexBuffer->Lock(0, 0, (void**)&vertices, NULL);

	float scale = 1.0f;

	RECT* rDest = new RECT();
	rDest->left = 0;
	rDest->right = int(viewport.Width*scale);
	rDest->top = 0;
	rDest->bottom = int(viewport.Height*scale);

	//Setup vertices
	vertices[0].x = (float) rDest->left - 0.5f;
	vertices[0].y = (float) rDest->top - 0.5f;
	vertices[0].z = 0.0f;
	vertices[0].rhw = 1.0f;
	vertices[0].u = 0.0f;
	vertices[0].v = 0.0f;

	vertices[1].x = (float) rDest->right - 0.5f;
	vertices[1].y = (float) rDest->top - 0.5f;
	vertices[1].z = 0.0f;
	vertices[1].rhw = 1.0f;
	vertices[1].u = 1.0f;
	vertices[1].v = 0.0f;

	vertices[2].x = (float) rDest->right - 0.5f;
	vertices[2].y = (float) rDest->bottom - 0.5f;
	vertices[2].z = 0.0f;
	vertices[2].rhw = 1.0f;
	vertices[2].u = 1.0f;	
	vertices[2].v = 1.0f;

	vertices[3].x = (float) rDest->left - 0.5f;
	vertices[3].y = (float) rDest->bottom - 0.5f;
	vertices[3].z = 0.0f;
	vertices[3].rhw = 1.0f;
	vertices[3].u = 0.0f;
	vertices[3].v = 1.0f;

	screenVertexBuffer->Unlock();
}

/**
* Loads stereo mode effect file.
***/
void StereoView::InitShaderEffects()
{
	if (FAILED(D3DXCreateEffectFromFileA(m_pActualDevice, (config.vireioDir+"shaders/"+config.shader).toLocal8Bit(), NULL, NULL, D3DXFX_DONOTSAVESTATE, NULL, &viewEffect, NULL))) {
		OutputDebugStringA("Effect creation failed\n");
	}
}



/**
* Update all vertex shader constants.
***/
void StereoView::SetViewEffectInitialValues() {
	viewEffect->SetInt       ( "viewWidth"       , viewport.Width );
	viewEffect->SetInt       ( "viewHeight"      , viewport.Height );
	viewEffect->SetFloatArray( "LensCenter"      , LensCenter , 2 );
	viewEffect->SetFloatArray( "Scale"           , Scale , 2);
	viewEffect->SetFloatArray( "ScaleIn"         , ScaleIn , 2);
	viewEffect->SetFloatArray( "HmdWarpParam"    , config.distortionCoefficients , 4 );
	viewEffect->SetFloat     ( "ViewportXOffset" , -ViewportXOffset );
	viewEffect->SetFloat     ( "ViewportYOffset" , -ViewportYOffset );

	if( config.chromaticAberrationCorrection ){
		viewEffect->SetFloatArray( "Chroma",  config.chromaCoefficients , 4 );
	}else{
		static float noChroma[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		viewEffect->SetFloatArray( "Chroma" , noChroma , 4 );
	}

	float resolution[2];
	resolution[0] = config.resolutionWidth;
	resolution[1] = config.resolutionHeight;
	viewEffect->SetFloatArray( "Resolution" , resolution , 2 );
} 


/**
* Calculate all vertex shader constants.
***/ 
void StereoView::CalculateShaderVariables() {
	// Center of half screen is 0.25 in x (halfscreen x input in 0 to 0.5 range)
	// Lens offset is in a -1 to 1 range. Using in shader with a 0 to 0.5 range so use 25% of the value.
	LensCenter[0] = 0.25f + (config.lensXCenterOffset * 0.25f) - (config.lensIPDCenterOffset - config.IPDOffset);

	// Center of halfscreen range is 0.5 in y (halfscreen y input in 0 to 1 range)
	LensCenter[1] = config.lensYCenterOffset - config.YOffset;

	
	
	ViewportXOffset = XOffset;
	ViewportYOffset = HeadYOffset;

	D3DSURFACE_DESC eyeTextureDescriptor;
	leftSurface->GetDesc(&eyeTextureDescriptor);

	float inputTextureAspectRatio = (float)eyeTextureDescriptor.Width / (float)eyeTextureDescriptor.Height;
	
	// Note: The range is shifted using the LensCenter in the shader before the scale is applied so you actually end up with a -1 to 1 range
	// in the distortion function rather than the 0 to 2 I mention below.
	// Input texture scaling to sample the 0 to 0.5 x range of the half screen area in the correct aspect ratio in the distortion function
	// x is changed from 0 to 0.5 to 0 to 2.
	ScaleIn[0] = 4.0f;
	// y is changed from 0 to 1 to 0 to 2 and scaled to account for aspect ratio
	ScaleIn[1] = 2.0f / (inputTextureAspectRatio * 0.5f); // 1/2 aspect ratio for differing input ranges
	
	float scaleFactor = (1.0f / (config.scaleToFillHorizontal + config.DistortionScale));

	// Scale from 0 to 2 to 0 to 1  for x and y 
	// Then use scaleFactor to fill horizontal space in line with the lens and adjust for aspect ratio for y.
	Scale[0] = (1.0f / 4.0f) * scaleFactor;
	Scale[1] = (1.0f / 2.0f) * scaleFactor * inputTextureAspectRatio;
} 




/**
* Workaround for Half Life 2 for now.
***/
void StereoView::SaveState()
{
	m_pActualDevice->GetTextureStageState(0, D3DTSS_COLOROP, &tssColorOp);
	m_pActualDevice->GetTextureStageState(0, D3DTSS_COLORARG1, &tssColorArg1);
	m_pActualDevice->GetTextureStageState(0, D3DTSS_ALPHAOP, &tssAlphaOp);
	m_pActualDevice->GetTextureStageState(0, D3DTSS_ALPHAARG1, &tssAlphaArg1);
	m_pActualDevice->GetTextureStageState(0, D3DTSS_CONSTANT, &tssConstant);

	m_pActualDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &rsAlphaEnable);
	m_pActualDevice->GetRenderState(D3DRS_ZWRITEENABLE, &rsZWriteEnable);
	m_pActualDevice->GetRenderState(D3DRS_ZENABLE, &rsZEnable);
	m_pActualDevice->GetRenderState(D3DRS_SRGBWRITEENABLE, &rsSrgbEnable);

	m_pActualDevice->GetSamplerState(0, D3DSAMP_SRGBTEXTURE, &ssSrgb);
	m_pActualDevice->GetSamplerState(1, D3DSAMP_SRGBTEXTURE, &ssSrgb1);
	
	m_pActualDevice->GetSamplerState(0, D3DSAMP_ADDRESSU, &ssAddressU);
	m_pActualDevice->GetSamplerState(0, D3DSAMP_ADDRESSV, &ssAddressV);
	m_pActualDevice->GetSamplerState(0, D3DSAMP_ADDRESSW, &ssAddressW);

	m_pActualDevice->GetSamplerState(0, D3DSAMP_MAGFILTER, &ssMag0);
	m_pActualDevice->GetSamplerState(1, D3DSAMP_MAGFILTER, &ssMag1);
	m_pActualDevice->GetSamplerState(0, D3DSAMP_MINFILTER, &ssMin0);
	m_pActualDevice->GetSamplerState(1, D3DSAMP_MINFILTER, &ssMin1);
	m_pActualDevice->GetSamplerState(0, D3DSAMP_MIPFILTER, &ssMip0);
	m_pActualDevice->GetSamplerState(1, D3DSAMP_MIPFILTER, &ssMip1);

	m_pActualDevice->GetTexture(0, &lastTexture);
	m_pActualDevice->GetTexture(1, &lastTexture1);

	m_pActualDevice->GetVertexShader(&lastVertexShader);
	m_pActualDevice->GetPixelShader(&lastPixelShader);

	m_pActualDevice->GetVertexDeclaration(&lastVertexDeclaration);

	m_pActualDevice->GetRenderTarget(0, &lastRenderTarget0);
	m_pActualDevice->GetRenderTarget(1, &lastRenderTarget1);
}

/**
* Set all states and settings for fullscreen render.
* Also sets identity world, view and projection matrix. 
***/
void StereoView::SetState()
{
	D3DXMATRIX	identity;
	m_pActualDevice->SetTransform(D3DTS_WORLD, D3DXMatrixIdentity(&identity));
	m_pActualDevice->SetTransform(D3DTS_VIEW, &identity);
	m_pActualDevice->SetTransform(D3DTS_PROJECTION, &identity);
	m_pActualDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	m_pActualDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	m_pActualDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
	m_pActualDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	m_pActualDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	m_pActualDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);// This fixed interior or car not being drawn in rFactor
	m_pActualDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE); 

	m_pActualDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	m_pActualDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	m_pActualDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	m_pActualDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_CONSTANT);
	m_pActualDevice->SetTextureStageState(0, D3DTSS_CONSTANT, 0xffffffff);

	m_pActualDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	m_pActualDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
	m_pActualDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	m_pActualDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);  

	//m_pActualDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, 0);  // will cause visual errors in HL2

	if( config.game_type == D3DProxyDevice::SOURCE_L4D ||
		config.game_type == D3DProxyDevice::SOURCE_ESTER)
	{
		m_pActualDevice->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, ssSrgb);
		m_pActualDevice->SetSamplerState(1, D3DSAMP_SRGBTEXTURE, ssSrgb);
	}
	else
	{
		//Borderlands Dark Eye FIX
		m_pActualDevice->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, 0);
		m_pActualDevice->SetSamplerState(1, D3DSAMP_SRGBTEXTURE, 0);
	}
	

	m_pActualDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	m_pActualDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	m_pActualDevice->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);
	m_pActualDevice->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	m_pActualDevice->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	m_pActualDevice->SetSamplerState(1, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);

	// TODO Need to check m_pActualDevice capabilities if we want a prefered order of fallback rather than 
	// whatever the default is being used when a mode isn't supported.
	// Example - GeForce 660 doesn't appear to support D3DTEXF_ANISOTROPIC on the MAGFILTER (at least
	// according to the spam of error messages when running with the directx debug runtime)
	m_pActualDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_ANISOTROPIC);
	m_pActualDevice->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_ANISOTROPIC);
	m_pActualDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
	m_pActualDevice->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
	m_pActualDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
	m_pActualDevice->SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

	//m_pActualDevice->SetTexture(0, NULL);
	//m_pActualDevice->SetTexture(1, NULL);

	m_pActualDevice->SetVertexShader(NULL);
	m_pActualDevice->SetPixelShader(NULL);

	m_pActualDevice->SetVertexDeclaration(NULL);

	//It's a Direct3D9 error when using the debug runtine to set RenderTarget 0 to NULL
	//m_pActualDevice->SetRenderTarget(0, NULL);
	m_pActualDevice->SetRenderTarget(1, NULL);
	m_pActualDevice->SetRenderTarget(2, NULL);
	m_pActualDevice->SetRenderTarget(3, NULL);
}

/**
* Workaround for Half Life 2 for now.
***/
void StereoView::RestoreState()
{
	m_pActualDevice->SetTextureStageState(0, D3DTSS_COLOROP, tssColorOp);
	m_pActualDevice->SetTextureStageState(0, D3DTSS_COLORARG1, tssColorArg1);
	m_pActualDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, tssAlphaOp);
	m_pActualDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, tssAlphaArg1);
	m_pActualDevice->SetTextureStageState(0, D3DTSS_CONSTANT, tssConstant);

	m_pActualDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, rsAlphaEnable);
	m_pActualDevice->SetRenderState(D3DRS_ZWRITEENABLE, rsZWriteEnable);
	m_pActualDevice->SetRenderState(D3DRS_ZENABLE, rsZEnable);
	m_pActualDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, rsSrgbEnable);

	m_pActualDevice->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, ssSrgb);
	m_pActualDevice->SetSamplerState(1, D3DSAMP_SRGBTEXTURE, ssSrgb1);

	m_pActualDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, ssAddressU);
	m_pActualDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, ssAddressV);
	m_pActualDevice->SetSamplerState(0, D3DSAMP_ADDRESSW, ssAddressW);

	m_pActualDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, ssMag0);
	m_pActualDevice->SetSamplerState(1, D3DSAMP_MAGFILTER, ssMag1);
	m_pActualDevice->SetSamplerState(0, D3DSAMP_MINFILTER, ssMin0);
	m_pActualDevice->SetSamplerState(1, D3DSAMP_MINFILTER, ssMin1);
	m_pActualDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, ssMip0);
	m_pActualDevice->SetSamplerState(1, D3DSAMP_MIPFILTER, ssMip1);

	m_pActualDevice->SetTexture(0, lastTexture);
	SAFE_RELEASE( lastTexture )

	m_pActualDevice->SetTexture(1, lastTexture1);
	SAFE_RELEASE( lastTexture1 )

	m_pActualDevice->SetVertexShader(lastVertexShader);
	SAFE_RELEASE( lastVertexShader )

	m_pActualDevice->SetPixelShader(lastPixelShader);
	SAFE_RELEASE( lastPixelShader )

	m_pActualDevice->SetVertexDeclaration(lastVertexDeclaration);
	SAFE_RELEASE( lastVertexDeclaration )

	m_pActualDevice->SetRenderTarget(0, lastRenderTarget0);
	SAFE_RELEASE( lastRenderTarget0 )

	m_pActualDevice->SetRenderTarget(1, lastRenderTarget1);
	SAFE_RELEASE( lastRenderTarget1 )
}

/**
* Saves all Direct3D 9 render states.
* Used for games that do not work with state blocks for some reason.
***/
void StereoView::SaveAllRenderStates(LPDIRECT3DDEVICE9 pDevice)
{
	// save all Direct3D 9 RenderStates 
	DWORD dwCount = 0;
	pDevice->GetRenderState(D3DRS_ZENABLE                     , &renderStates[dwCount++]); 
	pDevice->GetRenderState(D3DRS_FILLMODE                    , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_SHADEMODE                   , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_ZWRITEENABLE                , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_ALPHATESTENABLE             , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_LASTPIXEL                   , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_SRCBLEND                    , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_DESTBLEND                   , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_CULLMODE                    , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_ZFUNC                       , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_ALPHAREF                    , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_ALPHAFUNC                   , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_DITHERENABLE                , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_ALPHABLENDENABLE            , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_FOGENABLE                   , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_SPECULARENABLE              , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_FOGCOLOR                    , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_FOGTABLEMODE                , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_FOGSTART                    , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_FOGEND                      , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_FOGDENSITY                  , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_RANGEFOGENABLE              , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_STENCILENABLE               , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_STENCILFAIL                 , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_STENCILZFAIL                , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_STENCILPASS                 , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_STENCILFUNC                 , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_STENCILREF                  , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_STENCILMASK                 , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_STENCILWRITEMASK            , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_TEXTUREFACTOR               , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_WRAP0                       , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_WRAP1                       , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_WRAP2                       , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_WRAP3                       , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_WRAP4                       , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_WRAP5                       , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_WRAP6                       , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_WRAP7                       , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_CLIPPING                    , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_LIGHTING                    , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_AMBIENT                     , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_FOGVERTEXMODE               , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_COLORVERTEX                 , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_LOCALVIEWER                 , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_NORMALIZENORMALS            , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_DIFFUSEMATERIALSOURCE       , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_SPECULARMATERIALSOURCE      , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_AMBIENTMATERIALSOURCE       , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_EMISSIVEMATERIALSOURCE      , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_VERTEXBLEND                 , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_CLIPPLANEENABLE             , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_POINTSIZE                   , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_POINTSIZE_MIN               , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_POINTSPRITEENABLE           , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_POINTSCALEENABLE            , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_POINTSCALE_A                , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_POINTSCALE_B                , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_POINTSCALE_C                , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_MULTISAMPLEANTIALIAS        , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_MULTISAMPLEMASK             , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_PATCHEDGESTYLE              , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_DEBUGMONITORTOKEN           , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_POINTSIZE_MAX               , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE    , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_COLORWRITEENABLE            , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_TWEENFACTOR                 , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_BLENDOP                     , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_POSITIONDEGREE              , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_NORMALDEGREE                , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_SCISSORTESTENABLE           , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_SLOPESCALEDEPTHBIAS         , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_ANTIALIASEDLINEENABLE       , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_MINTESSELLATIONLEVEL        , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_MAXTESSELLATIONLEVEL        , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_ADAPTIVETESS_X              , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_ADAPTIVETESS_Y              , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_ADAPTIVETESS_Z              , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_ADAPTIVETESS_W              , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_ENABLEADAPTIVETESSELLATION  , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_TWOSIDEDSTENCILMODE         , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_CCW_STENCILFAIL             , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_CCW_STENCILZFAIL            , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_CCW_STENCILPASS             , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_CCW_STENCILFUNC             , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_COLORWRITEENABLE1           , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_COLORWRITEENABLE2           , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_COLORWRITEENABLE3           , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_BLENDFACTOR                 , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_SRGBWRITEENABLE             , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_DEPTHBIAS                   , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_WRAP8                       , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_WRAP9                       , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_WRAP10                      , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_WRAP11                      , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_WRAP12                      , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_WRAP13                      , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_WRAP14                      , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_WRAP15                      , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_SEPARATEALPHABLENDENABLE    , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_SRCBLENDALPHA               , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_DESTBLENDALPHA              , &renderStates[dwCount++]);
	pDevice->GetRenderState(D3DRS_BLENDOPALPHA                , &renderStates[dwCount++]);
}

/**
* Sets all Direct3D 9 render states to their default values.
* Use this function only if a game does not want to render.
***/
void StereoView::SetAllRenderStatesDefault(LPDIRECT3DDEVICE9 pDevice)
{
	// set all Direct3D 9 RenderStates to default values
	float fData = 0.0f;
	double dData = 0.0f;

	pDevice->SetRenderState(D3DRS_ZENABLE                     , D3DZB_TRUE);
	pDevice->SetRenderState(D3DRS_FILLMODE                    , D3DFILL_SOLID);
	pDevice->SetRenderState(D3DRS_SHADEMODE                   , D3DSHADE_GOURAUD);
	pDevice->SetRenderState(D3DRS_ZWRITEENABLE                , TRUE);
	pDevice->SetRenderState(D3DRS_ALPHATESTENABLE             , FALSE);
	pDevice->SetRenderState(D3DRS_LASTPIXEL                   , TRUE);
	pDevice->SetRenderState(D3DRS_SRCBLEND                    , D3DBLEND_ONE);
	pDevice->SetRenderState(D3DRS_DESTBLEND                   , D3DBLEND_ZERO);
	pDevice->SetRenderState(D3DRS_CULLMODE                    , D3DCULL_CCW);
	pDevice->SetRenderState(D3DRS_ZFUNC                       , D3DCMP_LESSEQUAL);
	pDevice->SetRenderState(D3DRS_ALPHAREF                    , 0);
	pDevice->SetRenderState(D3DRS_ALPHAFUNC                   , D3DCMP_ALWAYS);
	pDevice->SetRenderState(D3DRS_DITHERENABLE                , FALSE);
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE            , FALSE);
	pDevice->SetRenderState(D3DRS_FOGENABLE                   , FALSE);
	pDevice->SetRenderState(D3DRS_SPECULARENABLE              , FALSE);
	pDevice->SetRenderState(D3DRS_FOGCOLOR                    , 0);
	pDevice->SetRenderState(D3DRS_FOGTABLEMODE                , D3DFOG_NONE);
	fData = 0.0f;
	pDevice->SetRenderState(D3DRS_FOGSTART                    , *((DWORD*)&fData));
	fData = 1.0f;
	pDevice->SetRenderState(D3DRS_FOGEND                      , *((DWORD*)&fData));
	fData = 1.0f;
	pDevice->SetRenderState(D3DRS_FOGDENSITY                  , *((DWORD*)&fData));
	pDevice->SetRenderState(D3DRS_RANGEFOGENABLE              , FALSE);
	pDevice->SetRenderState(D3DRS_STENCILENABLE               , FALSE);
	pDevice->SetRenderState(D3DRS_STENCILFAIL                 , D3DSTENCILOP_KEEP);
	pDevice->SetRenderState(D3DRS_STENCILZFAIL                , D3DSTENCILOP_KEEP);
	pDevice->SetRenderState(D3DRS_STENCILPASS                 , D3DSTENCILOP_KEEP);
	pDevice->SetRenderState(D3DRS_STENCILFUNC                 , D3DCMP_ALWAYS);
	pDevice->SetRenderState(D3DRS_STENCILREF                  , 0);
	pDevice->SetRenderState(D3DRS_STENCILMASK                 , 0xFFFFFFFF);
	pDevice->SetRenderState(D3DRS_STENCILWRITEMASK            , 0xFFFFFFFF);
	pDevice->SetRenderState(D3DRS_TEXTUREFACTOR               , 0xFFFFFFFF);
	pDevice->SetRenderState(D3DRS_WRAP0                       , 0);
	pDevice->SetRenderState(D3DRS_WRAP1                       , 0);
	pDevice->SetRenderState(D3DRS_WRAP2                       , 0);
	pDevice->SetRenderState(D3DRS_WRAP3                       , 0);
	pDevice->SetRenderState(D3DRS_WRAP4                       , 0);
	pDevice->SetRenderState(D3DRS_WRAP5                       , 0);
	pDevice->SetRenderState(D3DRS_WRAP6                       , 0);
	pDevice->SetRenderState(D3DRS_WRAP7                       , 0);
	pDevice->SetRenderState(D3DRS_CLIPPING                    , TRUE);
	pDevice->SetRenderState(D3DRS_LIGHTING                    , TRUE);
	pDevice->SetRenderState(D3DRS_AMBIENT                     , 0);
	pDevice->SetRenderState(D3DRS_FOGVERTEXMODE               , D3DFOG_NONE);
	pDevice->SetRenderState(D3DRS_COLORVERTEX                 , TRUE);
	pDevice->SetRenderState(D3DRS_LOCALVIEWER                 , TRUE);
	pDevice->SetRenderState(D3DRS_NORMALIZENORMALS            , FALSE);
	pDevice->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE       , D3DMCS_COLOR1);
	pDevice->SetRenderState(D3DRS_SPECULARMATERIALSOURCE      , D3DMCS_COLOR2);
	pDevice->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE       , D3DMCS_MATERIAL);
	pDevice->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE      , D3DMCS_MATERIAL);
	pDevice->SetRenderState(D3DRS_VERTEXBLEND                 , D3DVBF_DISABLE);
	pDevice->SetRenderState(D3DRS_CLIPPLANEENABLE             , 0);
	pDevice->SetRenderState(D3DRS_POINTSIZE                   , 64);
	fData = 1.0f;
	pDevice->SetRenderState(D3DRS_POINTSIZE_MIN               , *((DWORD*)&fData));
	pDevice->SetRenderState(D3DRS_POINTSPRITEENABLE           , FALSE);
	pDevice->SetRenderState(D3DRS_POINTSCALEENABLE            , FALSE);
	fData = 1.0f;
	pDevice->SetRenderState(D3DRS_POINTSCALE_A                , *((DWORD*)&fData));
	fData = 0.0f;
	pDevice->SetRenderState(D3DRS_POINTSCALE_B                , *((DWORD*)&fData));
	fData = 0.0f;
	pDevice->SetRenderState(D3DRS_POINTSCALE_C                , *((DWORD*)&fData));
	pDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS        , TRUE);
	pDevice->SetRenderState(D3DRS_MULTISAMPLEMASK             , 0xFFFFFFFF);
	pDevice->SetRenderState(D3DRS_PATCHEDGESTYLE              , D3DPATCHEDGE_DISCRETE);
	pDevice->SetRenderState(D3DRS_DEBUGMONITORTOKEN           , D3DDMT_ENABLE);
	dData = 64.0;
	pDevice->SetRenderState(D3DRS_POINTSIZE_MAX               , *((DWORD*)&dData));
	pDevice->SetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE    , FALSE);
	pDevice->SetRenderState(D3DRS_COLORWRITEENABLE            , 0x0000000F);
	fData = 0.0f;
	pDevice->SetRenderState(D3DRS_TWEENFACTOR                 , *((DWORD*)&fData));
	pDevice->SetRenderState(D3DRS_BLENDOP                     , D3DBLENDOP_ADD);
	pDevice->SetRenderState(D3DRS_POSITIONDEGREE              , D3DDEGREE_CUBIC);
	pDevice->SetRenderState(D3DRS_NORMALDEGREE                , D3DDEGREE_LINEAR );
	pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE           , FALSE);
	pDevice->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS         , 0);
	pDevice->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE       , FALSE);
	fData = 1.0f;
	pDevice->SetRenderState(D3DRS_MINTESSELLATIONLEVEL        , *((DWORD*)&fData));
	fData = 1.0f;
	pDevice->SetRenderState(D3DRS_MAXTESSELLATIONLEVEL        , *((DWORD*)&fData));
	fData = 0.0f;
	pDevice->SetRenderState(D3DRS_ADAPTIVETESS_X              , *((DWORD*)&fData));
	fData = 0.0f;
	pDevice->SetRenderState(D3DRS_ADAPTIVETESS_Y              , *((DWORD*)&fData));
	fData = 1.0f;
	pDevice->SetRenderState(D3DRS_ADAPTIVETESS_Z              , *((DWORD*)&fData));
	fData = 0.0f;
	pDevice->SetRenderState(D3DRS_ADAPTIVETESS_W              , *((DWORD*)&fData));
	pDevice->SetRenderState(D3DRS_ENABLEADAPTIVETESSELLATION  , FALSE);
	pDevice->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE         , FALSE);
	pDevice->SetRenderState(D3DRS_CCW_STENCILFAIL             , D3DSTENCILOP_KEEP);
	pDevice->SetRenderState(D3DRS_CCW_STENCILZFAIL            , D3DSTENCILOP_KEEP);
	pDevice->SetRenderState(D3DRS_CCW_STENCILPASS             , D3DSTENCILOP_KEEP);
	pDevice->SetRenderState(D3DRS_CCW_STENCILFUNC             , D3DCMP_ALWAYS);
	pDevice->SetRenderState(D3DRS_COLORWRITEENABLE1           , 0x0000000f);
	pDevice->SetRenderState(D3DRS_COLORWRITEENABLE2           , 0x0000000f);
	pDevice->SetRenderState(D3DRS_COLORWRITEENABLE3           , 0x0000000f);
	pDevice->SetRenderState(D3DRS_BLENDFACTOR                 , 0xffffffff);
	pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE             , 0);
	pDevice->SetRenderState(D3DRS_DEPTHBIAS                   , 0);
	pDevice->SetRenderState(D3DRS_WRAP8                       , 0);
	pDevice->SetRenderState(D3DRS_WRAP9                       , 0);
	pDevice->SetRenderState(D3DRS_WRAP10                      , 0);
	pDevice->SetRenderState(D3DRS_WRAP11                      , 0);
	pDevice->SetRenderState(D3DRS_WRAP12                      , 0);
	pDevice->SetRenderState(D3DRS_WRAP13                      , 0);
	pDevice->SetRenderState(D3DRS_WRAP14                      , 0);
	pDevice->SetRenderState(D3DRS_WRAP15                      , 0);
	pDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE    , FALSE);
	pDevice->SetRenderState(D3DRS_SRCBLENDALPHA               , D3DBLEND_ONE);
	pDevice->SetRenderState(D3DRS_DESTBLENDALPHA              , D3DBLEND_ZERO);
	pDevice->SetRenderState(D3DRS_BLENDOPALPHA                , D3DBLENDOP_ADD);
}

/**
* Restores all Direct3D 9 render states.
* Used for games that do not work with state blocks for some reason.
***/
void StereoView::RestoreAllRenderStates(LPDIRECT3DDEVICE9 pDevice)
{
	// set all Direct3D 9 RenderStates to saved values
	DWORD dwCount = 0;
	pDevice->SetRenderState(D3DRS_ZENABLE                     , renderStates[dwCount++]); 
	pDevice->SetRenderState(D3DRS_FILLMODE                    , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_SHADEMODE                   , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_ZWRITEENABLE                , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_ALPHATESTENABLE             , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_LASTPIXEL                   , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_SRCBLEND                    , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_DESTBLEND                   , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_CULLMODE                    , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_ZFUNC                       , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_ALPHAREF                    , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_ALPHAFUNC                   , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_DITHERENABLE                , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE            , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_FOGENABLE                   , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_SPECULARENABLE              , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_FOGCOLOR                    , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_FOGTABLEMODE                , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_FOGSTART                    , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_FOGEND                      , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_FOGDENSITY                  , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_RANGEFOGENABLE              , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_STENCILENABLE               , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_STENCILFAIL                 , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_STENCILZFAIL                , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_STENCILPASS                 , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_STENCILFUNC                 , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_STENCILREF                  , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_STENCILMASK                 , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_STENCILWRITEMASK            , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_TEXTUREFACTOR               , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_WRAP0                       , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_WRAP1                       , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_WRAP2                       , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_WRAP3                       , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_WRAP4                       , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_WRAP5                       , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_WRAP6                       , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_WRAP7                       , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_CLIPPING                    , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_LIGHTING                    , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_AMBIENT                     , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_FOGVERTEXMODE               , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_COLORVERTEX                 , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_LOCALVIEWER                 , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_NORMALIZENORMALS            , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE       , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_SPECULARMATERIALSOURCE      , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE       , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE      , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_VERTEXBLEND                 , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_CLIPPLANEENABLE             , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_POINTSIZE                   , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_POINTSIZE_MIN               , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_POINTSPRITEENABLE           , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_POINTSCALEENABLE            , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_POINTSCALE_A                , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_POINTSCALE_B                , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_POINTSCALE_C                , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS        , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_MULTISAMPLEMASK             , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_PATCHEDGESTYLE              , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_DEBUGMONITORTOKEN           , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_POINTSIZE_MAX               , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE    , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_COLORWRITEENABLE            , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_TWEENFACTOR                 , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_BLENDOP                     , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_POSITIONDEGREE              , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_NORMALDEGREE                , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE           , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS         , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE       , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_MINTESSELLATIONLEVEL        , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_MAXTESSELLATIONLEVEL        , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_ADAPTIVETESS_X              , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_ADAPTIVETESS_Y              , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_ADAPTIVETESS_Z              , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_ADAPTIVETESS_W              , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_ENABLEADAPTIVETESSELLATION  , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE         , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_CCW_STENCILFAIL             , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_CCW_STENCILZFAIL            , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_CCW_STENCILPASS             , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_CCW_STENCILFUNC             , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_COLORWRITEENABLE1           , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_COLORWRITEENABLE2           , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_COLORWRITEENABLE3           , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_BLENDFACTOR                 , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE             , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_DEPTHBIAS                   , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_WRAP8                       , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_WRAP9                       , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_WRAP10                      , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_WRAP11                      , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_WRAP12                      , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_WRAP13                      , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_WRAP14                      , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_WRAP15                      , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE    , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_SRCBLENDALPHA               , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_DESTBLENDALPHA              , renderStates[dwCount++]);
	pDevice->SetRenderState(D3DRS_BLENDOPALPHA                , renderStates[dwCount++]);
}
