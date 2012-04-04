#pragma once

#include "prerequisites.h"

inline XMMATRIX convert_d3dxmat_to_xnamat(const D3DXMATRIX& d3dx_mat)
{
    return XMMatrixSet( d3dx_mat._11, d3dx_mat._12, d3dx_mat._13, d3dx_mat._14,
                        d3dx_mat._21, d3dx_mat._22, d3dx_mat._23, d3dx_mat._24,
                        d3dx_mat._31, d3dx_mat._32, d3dx_mat._33, d3dx_mat._34,
                        d3dx_mat._41, d3dx_mat._42, d3dx_mat._43, d3dx_mat._44);
}

template <typename T>
d3d11_buffer_ptr make_default_cb(ID3D11Device *d3d11_device)
{
    HRESULT hr;

    d3d11_buffer_ptr buf_ptr; 
    D3D11_BUFFER_DESC buf_desc; 
    memset(&buf_desc, 0, sizeof(D3D11_BUFFER_DESC));
    buf_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    buf_desc.Usage = D3D11_USAGE_DEFAULT;
    buf_desc.ByteWidth = sizeof(T);
    buf_desc.CPUAccessFlags = 0;
    buf_desc.MiscFlags = 0; 
    V(d3d11_device->CreateBuffer(&buf_desc, NULL, &buf_ptr));
    
    return buf_ptr;
}

class c_texture2D 
{
public:
    c_texture2D(render_sys_context_ptr& render_context, const D3D11_TEXTURE2D_DESC *tex_desc, const D3D11_SUBRESOURCE_DATA *init_data);
    
    HRESULT bind_rt_view(const D3D11_RENDER_TARGET_VIEW_DESC *rtv_desc);
    HRESULT bind_sr_view(const D3D11_SHADER_RESOURCE_VIEW_DESC *srv_desc);
    HRESULT bind_ds_view(const D3D11_DEPTH_STENCIL_VIEW_DESC *dsv_desc);
    
    d3d11_render_target_view_ptr& get_rtv() { return m_rtv; }
    d3d11_shader_resourse_view_ptr& get_srv() { return m_srv; }
    d3d11_depth_stencil_view_ptr& get_dsv() { return m_dsv; }
    
    d3d11_texture2d_ptr& get_d3d11_texture_2d() { return m_texture; }
    
protected:
    d3d11_texture2d_ptr m_texture; 
    d3d11_render_target_view_ptr m_rtv;
    d3d11_shader_resourse_view_ptr m_srv;
    d3d11_depth_stencil_view_ptr m_dsv;
    
    bool m_bind_rtv_flag, m_bind_srv_flag, m_bind_dsv_flag; 
    
    render_sys_context_ptr m_render_context; 
};

texture_2d_ptr make_simple_rt(render_sys_context_ptr& render_context, 
                              unsigned int width, 
                              unsigned int height, 
                              DXGI_FORMAT format, 
                              unsigned int formatSizeInBytes = 0);


class c_render_texture
{
public:
    c_render_texture(d3d11_device_ptr& d3d11_device, D3D11_TEXTURE2D_DESC *tex_desc, D3D11_RENDER_TARGET_VIEW_DESC *rtv_desc = NULL, D3D11_SHADER_RESOURCE_VIEW_DESC *srv_desc = NULL, D3D11_DEPTH_STENCIL_VIEW_DESC *dsv_desc = NULL);

    d3d11_texture2d_ptr m_texture;
        d3d11_render_target_view_ptr m_rtv;
        d3d11_shader_resourse_view_ptr m_srv;
        d3d11_depth_stencil_view_ptr m_dsv;
};

//////////////////////////////////////////////////////////////////////////
// Mesh Utilities
struct ground_plane_vertex
{
    XMFLOAT3 pos;
    XMFLOAT3 normal;
};

class c_sdkmesh_wrapper
{
public:
    c_sdkmesh_wrapper(ID3D11Device *device, const std::wstring& _name, const std::wstring& _path, const bool _with_ground, SDKMESH_CALLBACKS11 *sdkmesh_callback = NULL);
    ~c_sdkmesh_wrapper();

private:
    void build_plane_geometry(ID3D11Device *device);

public:
    std::wstring name; 
    std::wstring path; 
    bool with_ground;
    sdkmesh_ptr mesh_ptr;
    D3DXVECTOR3 bb_extents;
    D3DXVECTOR3 bb_center;
    d3d11_buffer_ptr plane_vb;
    d3d11_buffer_ptr plane_ib;
};

//////////////////////////////////////////////////////////////////////////
// Effect Helpers 
enum e_effect_param_type
{
    effect_param_type_cb = 0,
    effect_param_type_sr,
    effect_param_type_dsv, 
    effect_param_type_rtv,
    effect_param_type_scalar,
    effect_param_type_vector,
    effect_param_type_matrix,
    effect_param_type_sampler,
    effect_param_type_rasterizer,
    effect_param_type_annotation,
    effect_param_type_string,
    effect_param_type_unknown
};

void init_effect_meta_info();
e_effect_param_type get_parameter_type(const std::string& type_str);

class c_d3dx11_effect_params_table
{
public:
    explicit c_d3dx11_effect_params_table(render_sys_context_ptr& render_context, d3dx11_effect_ptr& effect);
    
    ID3DX11EffectVariable* get_variable_by_name(const std::string& name);
    ID3DX11EffectTechnique* get_technique_by_name(const std::string& name);

    HRESULT apply_technique(ID3DX11EffectTechnique *tech, int pass = 0); 
    HRESULT bind_shader_resource(ID3DX11EffectShaderResourceVariable *sr, ID3D11ShaderResourceView *srv);
    HRESULT unbind_shader_resource(ID3DX11EffectShaderResourceVariable *sr); 

private:
    void parse(d3dx11_effect_ptr& effect);

    typedef std::map<std::string, ID3DX11EffectVariable*> name_variable_map; 
    typedef name_variable_map::value_type name_variable_pair;
    typedef std::map<std::string, ID3DX11EffectTechnique*> name_technique_map;
    typedef name_technique_map::value_type name_technique_pair;
    
    name_variable_map m_variables;
    name_technique_map m_techniques;

    render_sys_context_ptr m_render_context;
}; 
typedef boost::shared_ptr<c_d3dx11_effect_params_table> effect_params_table_ptr;

//////////////////////////////////////////////////////////////////////////
// PIX debug utility macros and functions
static D3DCOLOR black_color = D3DCOLOR_COLORVALUE(0, 0, 0, 1);
static D3DCOLOR green_color = D3DCOLOR_COLORVALUE(0, 1, 0, 1); 
static D3DCOLOR blue_color = D3DCOLOR_COLORVALUE(0, 0, 1, 1); 

#define PIX_EVENT_BEGIN(str)                        D3DPERF_BeginEvent(black_color, L#str); 
#define PIX_EVENT_END()                             D3DPERF_EndEvent();
#define PIX_EVENT_BEGIN_FRAME()                     D3DPERF_BeginEvent(green_color, L"Begin Frame");
#define PIX_EVENT_END_FRAME()                       D3DPERF_EndEvent();
#define PIX_EVENT_BEGIN_TECHNIQUE(tech_name)        D3DPERF_BeginEvent(black_color, L#tech_name);
#define PIX_EVENT_END_TECHNIQUE(tech_name)          D3DPERF_EndEvent(); 
#define PIX_EVENT_SET_RENDER_TARGETS()              D3DPERF_SetMarker(blue_color, L"OM Set RenderTargets"); 
#define PIX_EVENT_SET_EFFECT_SHADER_RESOUCE()       D3DPERF_SetMarker(black_color, L"Effect Set Shader Resource"); 

//////////////////////////////////////////////////////////////////////////

//class c_event
//{
//public:
//    c_event(const std::string& _name)
//        : name(_name)
//    {}
//
//    std::string name;
//};
//
//typedef boost::function<void (c_event& evt)> event_handler_func; 
//
//class c_event_dispatcher
//{
//    typedef std::map<std::string, event_handler_func> events_handlers_map; 
//public:
//    virtual void add_event_listener(const std::string& name, event_handler_func handler); 
//    virtual void remove_event_listener(const std::string& name); 
//    virtual void dispatch_event(const c_event& evt);
//    
//    events_handlers_map m_event_handler_pairs; 
//};    