#pragma once

#include "pch.h"

class c_texture2D;
class c_render_texture;
class c_render_system_context; 

// Device
typedef CComPtr<ID3D11Device> d3d11_device_ptr;
typedef CComPtr<ID3D11DeviceContext> d3d11_device_context_ptr;
typedef CComPtr<IDXGISwapChain> dxgi_swap_chain_ptr;

// Resources
typedef CComPtr<ID3D11Resource> d3d11_resource_ptr;
typedef CComPtr<ID3D11Texture2D> d3d11_texture2d_ptr;
typedef CComPtr<ID3D11Buffer> d3d11_buffer_ptr;
typedef CComPtr<ID3D11VertexShader> d3d11_vertex_shader_ptr;
typedef CComPtr<ID3D11PixelShader> d3d11_pixel_shader_ptr;
typedef CComPtr<ID3D11GeometryShader> d3d11_geometry_shader_ptr;
typedef CComPtr<ID3D11InputLayout> d3d11_input_layout_ptr;

// Resource View
typedef CComPtr<ID3D11RenderTargetView> d3d11_render_target_view_ptr;
typedef CComPtr<ID3D11ShaderResourceView> d3d11_shader_resourse_view_ptr;
typedef CComPtr<ID3D11DepthStencilView> d3d11_depth_stencil_view_ptr;

// States
typedef CComPtr<ID3D11DepthStencilState> d3d11_ds_state_ptr;

// Effect 11
typedef CComPtr<ID3DX11Effect> d3dx11_effect_ptr; 
typedef CComPtr<ID3DX11EffectTechnique> d3dx11_effect_tech_ptr;
typedef CComPtr<ID3DX11EffectConstantBuffer> d3dx11_effect_cb_var_ptr; 
typedef CComPtr<ID3DX11EffectVectorVariable> d3dx11_effect_vec_var_ptr; 
typedef CComPtr<ID3DX11EffectShaderResourceVariable> d3dx11_effect_sr_var_ptr;

// Utilities
typedef CComPtr<ID3DBlob> d3d11_blob_ptr;
typedef CComPtr<ID3DXBuffer> d3dx_buffer_ptr;

typedef boost::shared_ptr<c_render_system_context> render_sys_context_ptr; 
typedef boost::shared_ptr<CBaseCamera> base_camera_ptr;
typedef boost::shared_ptr<CModelViewerCamera> modelview_camera_ptr;
typedef boost::shared_ptr<CFirstPersonCamera> firstperson_camera_ptr;
typedef boost::shared_ptr<CDXUTSDKMesh> sdkmesh_ptr;
typedef boost::shared_ptr<c_texture2D> texture_2d_ptr;
typedef boost::shared_ptr<c_render_texture> render_texture_ptr;

// 
typedef std::vector<ID3DX11EffectTechnique*> effect_tech_vector;
typedef std::vector<ID3DX11EffectConstantBuffer*> effect_cb_param_vector;
typedef std::vector<ID3DX11EffectShaderResourceVariable*> effect_sr_param_vector;
typedef std::vector<ID3DX11EffectVectorVariable*> effect_vec_param_vector;
typedef std::vector<d3d11_buffer_ptr> res_buf_vector;
typedef std::vector<d3d11_texture2d_ptr> res_tex2d_vector;
typedef std::vector<render_texture_ptr> render_texture_vector;

// GUI