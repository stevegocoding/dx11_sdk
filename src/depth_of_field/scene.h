#pragma once

#include "app_listener.h"

struct mesh_descriptor
{
    std::wstring name; 
    std::wstring path; 
};

class c_app_scene  : public c_dx11_demo_app_listener
{
public:
    c_app_scene();

    virtual HRESULT on_d3d11_create_device(ID3D11Device *d3d11_device, const DXGI_SURFACE_DESC *backbuf_surface_desc);
    virtual void on_d3d11_destroy_device();
    virtual HRESULT on_d3d11_resized_swap_chain(IDXGISwapChain *swap_chain, const DXGI_SURFACE_DESC *backbuf_surface_desc);
    virtual void on_d3d11_releasing_swap_chain() {}
    
    virtual void on_frame_render(ID3D11Device *d3d11_device, ID3D11DeviceContext *d3d11_device_context, double time, float elapsed_time);
    virtual void on_frame_update(double time, double elapsed_time);
    
    virtual void on_win_msg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext); 
    
    modelview_camera_ptr get_camera() const { return m_camera; }

protected:

    typedef boost::shared_ptr<CDXUTSDKMesh> sdkmesh_ptr;
    typedef boost::shared_ptr<CModelViewerCamera> modelview_camera_ptr;
    sdkmesh_ptr m_city_mesh; 
    sdkmesh_ptr m_scanner_mesh;
    sdkmesh_ptr m_column_mesh;
    d3dx11_effect_ptr m_scene_effect;
    d3d11_input_layout_ptr m_mesh_input_layout;
    
    //////////////////////////////////////////////////////////////////////////
    // Effect Variables
    d3d11_buffer_ptr m_cb0;
    ID3DX11EffectTechnique *m_render_scene_tech;
    ID3DX11EffectConstantBuffer *m_cb0_var; 
    ID3DX11EffectShaderResourceVariable *m_tex_diffuse;
    
    modelview_camera_ptr m_camera;
};

typedef boost::shared_ptr<c_app_scene> scene_ptr;