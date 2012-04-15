#include "utils.h"
#include "render_system_context.h"

c_texture2D::c_texture2D(render_sys_context_ptr& render_context, const D3D11_TEXTURE2D_DESC *tex_desc, const D3D11_SUBRESOURCE_DATA *init_data)
    : m_bind_rtv_flag(false)
    , m_bind_srv_flag(false)
    , m_bind_dsv_flag(false)
    , m_render_context(render_context)
{
    HRESULT hr; 
    V(render_context->get_d3d11_device()->CreateTexture2D(tex_desc, init_data, &m_texture));
    
    if (tex_desc->BindFlags & D3D11_BIND_RENDER_TARGET)
    {
        m_bind_rtv_flag = true;
    }
    if (tex_desc->BindFlags & D3D11_BIND_SHADER_RESOURCE)
    {
        m_bind_srv_flag = true; 
    }
    if (tex_desc->BindFlags & D3D11_BIND_DEPTH_STENCIL)
    {
        m_bind_dsv_flag = true; 
    }

	
}

HRESULT c_texture2D::bind_rt_view(const D3D11_RENDER_TARGET_VIEW_DESC *rtv_desc)
{
    HRESULT hr;
    
    assert(m_bind_rtv_flag); 
    hr = m_render_context->get_d3d11_device()->CreateRenderTargetView(m_texture, rtv_desc, &m_rtv);
    return hr;
}

HRESULT c_texture2D::bind_sr_view(const D3D11_SHADER_RESOURCE_VIEW_DESC *srv_desc)
{
    HRESULT hr; 
    
    assert(m_bind_srv_flag);
    hr = m_render_context->get_d3d11_device()->CreateShaderResourceView(m_texture, srv_desc, &m_srv);
    return hr; 
}

HRESULT c_texture2D::bind_ds_view(const D3D11_DEPTH_STENCIL_VIEW_DESC *dsv_desc)
{
    HRESULT hr;
    
    assert(m_bind_dsv_flag);
    hr = m_render_context->get_d3d11_device()->CreateDepthStencilView(m_texture, dsv_desc, &m_dsv);
    return hr; 
}

c_render_texture::c_render_texture(d3d11_device_ptr& d3d11_device, D3D11_TEXTURE2D_DESC *tex_desc, D3D11_RENDER_TARGET_VIEW_DESC *rtv_desc /* = NULL */, D3D11_SHADER_RESOURCE_VIEW_DESC *srv_desc /* = NULL */, D3D11_DEPTH_STENCIL_VIEW_DESC *dsv_desc /* = NULL */)
{
    HRESULT hr;
    V(d3d11_device->CreateTexture2D( tex_desc, NULL, &m_texture ));   

    if (tex_desc->BindFlags & D3D11_BIND_RENDER_TARGET)
    {
        V(d3d11_device->CreateRenderTargetView(m_texture, rtv_desc, &m_rtv));
    }
    if (tex_desc->BindFlags & D3D11_BIND_DEPTH_STENCIL)
    {
        V(d3d11_device->CreateDepthStencilView(m_texture, dsv_desc, &m_dsv));
    }
    if (tex_desc->BindFlags & D3D11_BIND_SHADER_RESOURCE)
    {
        V(d3d11_device->CreateShaderResourceView(m_texture, srv_desc, &m_srv));
    }
}

texture_2d_ptr make_simple_rt(render_sys_context_ptr& render_context, 
                              unsigned int width, 
                              unsigned int height, 
                              DXGI_FORMAT format, 
                              unsigned int formatSizeInBytes)
{
    D3D11_TEXTURE2D_DESC desc; 
    memset(&desc, 0, sizeof(desc)); 
    desc.Width = width; 
    desc.Height = height; 
    desc.Format = format; 
    desc.MipLevels = 1; 
    desc.ArraySize = 1; 
    desc.SampleDesc.Count = 1; 
    desc.SampleDesc.Quality = 0; 
    desc.Usage = D3D11_USAGE_DEFAULT; 
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET; 
    desc.CPUAccessFlags = 0; 
    desc.MiscFlags = 0; 

    texture_2d_ptr tex;
    tex.reset(new c_texture2D(render_context, &desc, NULL)); 
	tex->bind_rt_view(NULL); 
	tex->bind_sr_view(NULL); 
    return tex; 
}

texture_2d_ptr make_simple_ds(render_sys_context_ptr& render_context, unsigned int width, unsigned int height, DXGI_FORMAT format)
{
	D3D11_TEXTURE2D_DESC desc; 
	memset(&desc, 0, sizeof(desc)); 
	desc.Width = width; 
	desc.Height = height; 
	desc.Format = format; 
	desc.MipLevels = 1; 
	desc.ArraySize = 1; 
	desc.SampleDesc.Count = 1; 
	desc.SampleDesc.Quality = 0; 
	desc.Usage = D3D11_USAGE_DEFAULT; 
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL; 
	desc.CPUAccessFlags = 0; 
	desc.MiscFlags = 0; 

	texture_2d_ptr tex;
	tex.reset(new c_texture2D(render_context, &desc, NULL)); 

	D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc; 
	D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
	::memset(&dsv_desc, 0, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
	::memset(&srv_desc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	
	switch (format)
	{
	case DXGI_FORMAT_R32_TYPELESS:
		dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
		srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
		break;
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
		dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		srv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		break;
	case DXGI_FORMAT_R24G8_TYPELESS:
		dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; 
		srv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		break;
	}
	
	dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D; 
	dsv_desc.Texture2D.MipSlice = 0; 
	
	tex->bind_ds_view(&dsv_desc);
	
	if (format == DXGI_FORMAT_R32_TYPELESS || format == DXGI_FORMAT_R24G8_TYPELESS)
	{
		srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D; 
		srv_desc.Texture2D.MostDetailedMip = 0; 
		srv_desc.Texture2D.MipLevels = 1;
		tex->bind_sr_view(&srv_desc); 
	}
	
	return tex; 
}

//////////////////////////////////////////////////////////////////////////
// Mesh Utilities

void CALLBACK load_texture_from_file(ID3D11Device *pdevice, char *file_name, ID3D11ShaderResourceView **ppsrv, void *user_context)
{
    HRESULT hr;

    d3d11_texture2d_ptr tex;
    std::string texture_path = std::string("../data/MicroscopeCity/") + std::string(file_name);
    D3DX11_IMAGE_LOAD_INFO img_load_info;
    D3DX11_IMAGE_INFO img_info;
    D3DX11GetImageInfoFromFileA(file_name, NULL, &img_info, NULL);
    img_load_info.pSrcInfo = &img_info; 
    V(D3DX11CreateTextureFromFileA(pdevice, texture_path.c_str(), &img_load_info, NULL, (ID3D11Resource**)&tex, &hr));

    // Create shader resource view for .fx 
    D3D11_TEXTURE2D_DESC tex_desc;
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc; 
    tex->GetDesc(&tex_desc);
    srv_desc.Format = tex_desc.Format;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MostDetailedMip = 0;
    srv_desc.Texture2D.MipLevels = tex_desc.MipLevels;

    V(pdevice->CreateShaderResourceView(tex, &srv_desc, ppsrv));
}

c_sdkmesh_wrapper::c_sdkmesh_wrapper(ID3D11Device *device, const std::wstring& _name, const std::wstring& _path, const bool _with_ground, SDKMESH_CALLBACKS11 *sdkmesh_callback /* = NULL */)
    : name(_name)
    , path(_path)
    , with_ground(_with_ground)
{
    HRESULT hr;

    mesh_ptr = sdkmesh_ptr(new CDXUTSDKMesh());
    V(mesh_ptr->Create(device, path.c_str(), true, sdkmesh_callback));

    if (with_ground)
        build_plane_geometry(device);

    bb_extents = mesh_ptr->GetMeshBBoxExtents(0);
    bb_center = mesh_ptr->GetMeshBBoxCenter(0);
}

c_sdkmesh_wrapper::~c_sdkmesh_wrapper()
{
    if (mesh_ptr)
        mesh_ptr->Destroy();    
}

void c_sdkmesh_wrapper::build_plane_geometry(ID3D11Device *device)
{
    HRESULT hr;

    //////////////////////////////////////////////////////////////////////////
    // Vertex Buffer
    float w = 1.0f;
    ground_plane_vertex vertices[] = 
    {
        { XMFLOAT3( -w, 0,  w ), XMFLOAT3( 0.0f, 1.0f, 0.0f ) },
        { XMFLOAT3(  w, 0,  w ), XMFLOAT3( 0.0f, 1.0f, 0.0f ) },
        { XMFLOAT3(  w, 0, -w ), XMFLOAT3( 0.0f, 1.0f, 0.0f ) },
        { XMFLOAT3( -w, 0, -w ), XMFLOAT3( 0.0f, 1.0f, 0.0f ) },
    };
    D3D11_BUFFER_DESC buf_desc;
    buf_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    buf_desc.Usage = D3D11_USAGE_DEFAULT;
    buf_desc.ByteWidth = sizeof(ground_plane_vertex) * 4;
    buf_desc.CPUAccessFlags = 0;
    buf_desc.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA init_data; 
    init_data.pSysMem = vertices;
    hr = device->CreateBuffer(&buf_desc, &init_data, &plane_vb); 
    assert(SUCCEEDED(hr));

    //////////////////////////////////////////////////////////////////////////
    // Index Buffer
    DWORD indices[] =
    {
        0,1,2,
        0,2,3,
    };
    buf_desc.Usage = D3D11_USAGE_DEFAULT;
    buf_desc.ByteWidth = sizeof( DWORD ) * 6;
    buf_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    buf_desc.CPUAccessFlags = 0;
    buf_desc.MiscFlags = 0;
    init_data.pSysMem = indices;
    hr = device->CreateBuffer(&buf_desc, &init_data, &plane_ib);
    assert(SUCCEEDED(hr));
}

//////////////////////////////////////////////////////////////////////////

typedef std::map<std::string, e_effect_param_type> param_name_type_map;

param_name_type_map g_param_types;

void init_effect_meta_info()
{
    using namespace boost::assign;
    g_param_types.clear(); 

    insert(g_param_types)
        ("constant_buffer", effect_param_type_cb)
        ("shader_resource", effect_param_type_sr)
        ("depth_stencil_view", effect_param_type_dsv)
        ("render_target_view", effect_param_type_rtv)
        ("scalar", effect_param_type_scalar)
        ("vector", effect_param_type_vector)
        ("matrix", effect_param_type_matrix)
        ("sampler", effect_param_type_sampler)
        ("rasterizer", effect_param_type_rasterizer)
        ("annotation", effect_param_type_annotation)
        ("string", effect_param_type_string);
}

e_effect_param_type get_parameter_type(const std::string& type_str)
{
    param_name_type_map::iterator it = g_param_types.find(type_str);
    if (it != g_param_types.end())
        return it->second; 
    else 
        return effect_param_type_unknown;
}

c_d3dx11_effect_params_table::c_d3dx11_effect_params_table(render_sys_context_ptr& render_context, d3dx11_effect_ptr& effect)
    : m_render_context(render_context)
{
    assert(effect);
    parse(effect);
}

void c_d3dx11_effect_params_table::parse(d3dx11_effect_ptr& effect)
{
    D3DX11_EFFECT_DESC effect_desc;
    effect->GetDesc(&effect_desc);
    unsigned int num_cbs = effect_desc.ConstantBuffers;
    unsigned int num_techniques = effect_desc.Techniques;
    unsigned int num_global_vars = effect_desc.GlobalVariables;
    
    for (unsigned int i = 0; i < num_global_vars; ++i)
    {
        ID3DX11EffectVariable *var = effect->GetVariableByIndex(i);
        D3DX11_EFFECT_VARIABLE_DESC var_desc;
        var->GetDesc(&var_desc);
        std::string name = std::string(var_desc.Name);
        m_variables.insert(name_variable_pair(name, var));
    }
    
    for (unsigned int i = 0; i < num_cbs; ++i)
    {
        ID3DX11EffectConstantBuffer *cb = effect->GetConstantBufferByIndex(i);
        D3DX11_EFFECT_VARIABLE_DESC var_desc;
        cb->GetDesc(&var_desc);
        std::string name = std::string(var_desc.Name);
        m_variables.insert(name_variable_pair(name, cb));
    }

    for (unsigned int i = 0; i < num_techniques; ++i)
    {
        ID3DX11EffectTechnique *tech = effect->GetTechniqueByIndex(i);
        D3DX11_TECHNIQUE_DESC tech_desc;
        tech->GetDesc(&tech_desc);
        std::string name = std::string(tech_desc.Name);
        m_techniques.insert(name_technique_pair(name, tech));
    }
}

ID3DX11EffectVariable* c_d3dx11_effect_params_table::get_variable_by_name(const std::string& name)
{
    name_variable_map::iterator it = m_variables.find(name);
    assert(it != m_variables.end());
    return it->second;
}

ID3DX11EffectTechnique* c_d3dx11_effect_params_table::get_technique_by_name(const std::string& name)
{
    name_technique_map::iterator it = m_techniques.find(name);
    assert(it != m_techniques.end());
    return it->second;
}

HRESULT c_d3dx11_effect_params_table::apply_technique(ID3DX11EffectTechnique *tech, int pass)
{
    HRESULT hr; 
    
    assert(tech);
    hr = tech->GetPassByIndex(pass)->Apply(0, m_render_context->get_d3d11_device_context()); 

    return hr; 
}

HRESULT c_d3dx11_effect_params_table::bind_shader_resource(ID3DX11EffectShaderResourceVariable *sr, ID3D11ShaderResourceView *srv)
{
    assert(sr);
    assert(srv); 
    return sr->SetResource(srv); 
}

HRESULT c_d3dx11_effect_params_table::unbind_shader_resource(ID3DX11EffectShaderResourceVariable *sr)
{
    assert(sr);
    return sr->SetResource(NULL);
}




//////////////////////////////////////////////////////////////////////////
// Events
//
//void c_event_dispatcher::add_event_listener(const std::string& name, event_handler_func handler)
//{
//    
//}
//
//void c_event_dispatcher::remove_event_listener(const std::string& name)
//{
//    
//}
//
//void c_event_dispatcher::dispatch_event(const c_event& evt)
//{
//    
//}

