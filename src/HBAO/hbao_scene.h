#pragma once 

#include "app_listener.h"
#include "render_system_context.h"
#include "utils.h"

class c_sdkmesh_wrapper;
typedef boost::shared_ptr<c_sdkmesh_wrapper> mesh_desc_ptr;

class c_hbao_scene_component : public c_demo_app_listener
{
    typedef c_demo_app_listener super;

public:
    c_hbao_scene_component();    
    virtual HRESULT on_create_resource(const render_sys_context_ptr& render_sys_context);
    virtual void on_release_resource();
    virtual void on_frame_render(double time, float elapsed_time);
    virtual void on_frame_update(double time, float elapsed_time); 
    virtual LRESULT msg_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext);
    
public:
    HRESULT setup_geometry(const render_sys_context_ptr& render_sys_context);
    HRESULT compile_effects(const render_sys_context_ptr& render_sys_context);
    HRESULT create_buffers(const render_sys_context_ptr& render_sys_context);
    void setup_camera(const render_sys_context_ptr& render_sys_context);

    void render_mesh();
    void render_ground_plane();
    void update_transform_params(const XMMATRIX& mat_world, const XMMATRIX& mat_view, const XMMATRIX& mat_proj); 
    
private:
   
    //////////////////////////////////////////////////////////////////////////
    // Resources
    typedef std::vector<mesh_desc_ptr> meshes_vector;
    meshes_vector m_meshes; 

    d3d11_buffer_ptr m_cb0;
    d3dx11_effect_ptr m_scene_effect;
    d3d11_input_layout_ptr m_input_layout;

    //////////////////////////////////////////////////////////////////////////
    // Effect techs and variables
    ID3DX11EffectTechnique *m_render_scene_tech; 
    ID3DX11EffectConstantBuffer *m_param_cb0;
    ID3DX11EffectScalarVariable *m_param_is_ground; 
    
    effect_params_table_ptr m_effect_params_table;

    //////////////////////////////////////////////////////////////////////////
    // Camera
    modelview_camera_ptr m_camera;

    int m_current_model;
    D3D11_VIEWPORT m_vp;
};
typedef boost::shared_ptr<c_hbao_scene_component> hbao_scene_ptr; 