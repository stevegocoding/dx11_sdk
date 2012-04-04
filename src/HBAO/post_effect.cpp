#include "post_effect.h"

c_post_effect::c_post_effect(const render_sys_context_ptr& render_sys_context, const std::wstring& fx_file_name, const post_effect_logic_vector& logics)
    : m_tech_logics(logics)
    , m_render_sys_context(render_sys_context)
{
    HRESULT hr;

    V(compile_shader(fx_file_name)); 

    D3DX11_EFFECT_DESC effect_desc;
    m_effect->GetDesc(&effect_desc);
    assert(effect_desc.Techniques == logics.size());
}

HRESULT c_post_effect::compile_shader(const std::wstring& fx_file_name)
{
    HRESULT hr;

    d3d11_blob_ptr effect_blob, error_blob;
    V(D3DX11CompileFromFile(fx_file_name.c_str(),           // effect file name
        NULL,                           // defines
        NULL,                           // includes
        NULL,                           // function name
        "fx_5_0",                       // profile
        D3D10_SHADER_DEBUG,             // compile flag
        NULL,                           // compile flag
        NULL,                           // thread pump
        &effect_blob,                   // effect buffer
        &error_blob,                    // error buffer
        NULL));

    if (m_effect)
        m_effect = NULL;
    V(D3DX11CreateEffectFromMemory(effect_blob->GetBufferPointer(),         // effect buffer pointer
        effect_blob->GetBufferSize(),
        0,
        m_render_sys_context->get_d3d11_device(),
        &m_effect));

    return S_OK;
}

void c_post_effect::apply(unsigned int technique)
{
    assert(technique < m_tech_logics.size());

    m_tech_logics[technique]->pre_apply(shared_from_this());
    m_tech_logics[technique]->apply(shared_from_this());
    m_tech_logics[technique]->post_apply(shared_from_this());
}