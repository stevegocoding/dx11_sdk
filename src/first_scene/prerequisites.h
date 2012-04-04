#pragma once 

#include <atlbase.h>

#include <d3d11.h>
#include <d3dx11.h> 
#include <d3dx11effect.h>
#include <d3dx9.h>

// Device
typedef CComPtr<ID3D11Device> d3d11_device_ptr; 
typedef CComPtr<ID3D11DeviceContext> d3d11_device_context_ptr;
typedef CComPtr<IDXGISwapChain> dxgi_swap_chain_ptr;

// Resources
typedef CComPtr<ID3D11Texture2D> d3d11_texture2d_ptr; 
typedef CComPtr<ID3D11Buffer> d3d11_buffer_ptr; 
typedef CComPtr<ID3DX11Effect> d3dx11_effect_ptr;
typedef CComPtr<ID3D11VertexShader> d3d11_vertex_shader_ptr; 
typedef CComPtr<ID3D11PixelShader> d3d11_pixel_shader_ptr; 
typedef CComPtr<ID3D11GeometryShader> d3d11_geometry_shader_ptr;
typedef CComPtr<ID3D11InputLayout> d3d11_input_layout_ptr; 

// Resource View
typedef CComPtr<ID3D11RenderTargetView> d3d11_render_target_view_ptr;

// Utilities
typedef CComPtr<ID3DBlob> d3d11_blob_ptr; 
typedef CComPtr<ID3DXBuffer> d3dx_buffer_ptr;