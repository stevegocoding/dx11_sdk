#pragma once

#include "pch.h" 
#include "prerequisites.h" 

struct simple_vertex
{
    XMFLOAT3 pos;
    XMFLOAT3 normal;
    XMFLOAT2 texcoord;
    
    simple_vertex(const XMFLOAT3& _pos, const XMFLOAT3& _normal, const XMFLOAT2& _texcoord)
        : pos(_pos), normal(_normal), texcoord(_texcoord)
    {}
}; 

struct cb_vs
{
    D3DXMATRIX world_view_proj;
};

//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DX11
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut );

