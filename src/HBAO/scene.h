#pragma once

#include "app_listener.h"

class c_sdkmesh_wrapper;
typedef boost::shared_ptr<c_sdkmesh_wrapper> mesh_desc_ptr;

class c_app_scene  : public c_dx11_demo_app_listener
{
public:
    c_app_scene();
    
    virtual HRESULT on_create_device11(ID3D11Device *d3d11_device, const DXGI_SURFACE_DESC *backbuf_surface_desc);
    virtual void on_destroy_device11();
    virtual HRESULT on_resized_swap_chain11(IDXGISwapChain *swap_chain, const DXGI_SURFACE_DESC *backbuf_surface_desc);
    virtual void on_releasing_swap_chain11() {}
    virtual void on_frame_render(ID3D11Device *d3d11_device, ID3D11DeviceContext *d3d11_device_context, double time, float elapsed_time);
    virtual void on_frame_update(double time, double elapsed_time);
    virtual void on_win_msg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext); 
    
    base_camera_ptr get_camera() { return m_camera; }
    void set_current_model(const int model) { m_current_model = model; }
    
private:
    void render_mesh(ID3D11DeviceContext *device_context);
    void render_ground_plane(ID3D11DeviceContext *device_context);
    void update_transform_params(ID3D11DeviceContext *device_context, const XMMATRIX& mat_world, const XMMATRIX& mat_view, const XMMATRIX& mat_proj); 
    void update_global_params(ID3D11DeviceContext *device_context, const bool is_ground_plane); 

protected:
    
    //////////////////////////////////////////////////////////////////////////
    // Resources
    typedef std::vector<mesh_desc_ptr> meshes_vector;
    meshes_vector m_meshes; 

    res_buf_vector m_res_bufs;
    d3dx11_effect_ptr m_scene_effect;
    d3d11_input_layout_ptr m_input_layout;
    
    //////////////////////////////////////////////////////////////////////////
    // Effect techs and variables
    effect_tech_vector m_tech_vec;
    effect_cb_param_vector m_cb_param_vec; 
    
    //////////////////////////////////////////////////////////////////////////
    // Camera
    modelview_camera_ptr m_camera;
    
    //////////////////////////////////////////////////////////////////////////
    int m_current_model;
    int m_backbuf_width;
    int m_backbuf_height;

    D3D11_VIEWPORT m_vp;
};

typedef boost::shared_ptr<c_app_scene> scene_ptr;