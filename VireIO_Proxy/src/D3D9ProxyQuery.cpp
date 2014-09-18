/********************************************************************
Vireio Perception: Open-Source Stereoscopic 3D Driver
Copyright (C) 2012 Andres Hernandez

File <Direct3DQuery9.cpp> and
Class <D3D9ProxyQuery> :
Copyright (C) 2013 Chris Drain

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

#include "D3D9ProxyQuery.h"
#include <assert.h>

/**
* Constructor. 
* @param pActualQuery Imbed actual query. 
* @param pOwningDevice Pointer to the device that owns the query. 
***/
D3D9ProxyQuery::D3D9ProxyQuery(IDirect3DQuery9* pActualQuery, IDirect3DDevice9* pOwningDevice) :
	m_pActualQuery(pActualQuery),
	m_pOwningDevice(pOwningDevice),
	m_nRefCount(1)
{
	assert (pActualQuery != NULL);
	assert (pOwningDevice != NULL);

	pOwningDevice->AddRef();
}

/**
* Destructor. 
* Releases embedded query. 
***/
D3D9ProxyQuery::~D3D9ProxyQuery()
{
	if(m_pActualQuery) {
		m_pActualQuery->Release();
	}

	if (m_pOwningDevice) {
		m_pOwningDevice->Release();
	}
}

/**
* Base QueryInterface functionality. 
***/
HRESULT WINAPI D3D9ProxyQuery::QueryInterface(REFIID riid, LPVOID* ppv)
{
	return m_pActualQuery->QueryInterface(riid, ppv);
}

/**
* Base AddRef functionality.
***/
ULONG WINAPI D3D9ProxyQuery::AddRef()
{
	return ++m_nRefCount;
}

/**
* Base Release functionality.
***/
ULONG WINAPI D3D9ProxyQuery::Release()
{
	if(--m_nRefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_nRefCount;
}

/**
* Base GetDevice functionality.
***/
HRESULT WINAPI D3D9ProxyQuery::GetDevice(IDirect3DDevice9** ppDevice)
{
	if (!m_pOwningDevice)
		return D3DERR_INVALIDCALL;
	else {
		*ppDevice = m_pOwningDevice;
		return D3D_OK;
	}
}

/**
* Base GetType functionality.
***/
D3DQUERYTYPE WINAPI D3D9ProxyQuery::GetType()
{
	return m_pActualQuery->GetType();
}

/**
* Base GetDataSize functionality.
***/
DWORD WINAPI D3D9ProxyQuery::GetDataSize()
{
	return m_pActualQuery->GetDataSize();
}

/**
* Base Issue functionality.
***/
HRESULT WINAPI D3D9ProxyQuery::Issue(DWORD dwIssueFlags)
{
	return m_pActualQuery->Issue(dwIssueFlags);
}

/**
* Base GetData functionality.
***/
HRESULT WINAPI D3D9ProxyQuery::GetData(void* pData, DWORD dwSize, DWORD dwGetDataFlags)
{
	return m_pActualQuery->GetData(pData, dwSize, dwGetDataFlags);
}