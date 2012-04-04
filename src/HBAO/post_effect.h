#pragma once 

#include "prerequisites.h"
#include "render_system_context.h"
#include "utils.h"

typedef boost::shared_ptr<class c_post_effect> post_effect_ptr;
typedef boost::shared_ptr<class c_post_effect_logic> post_effect_logic_ptr;
typedef std::vector<post_effect_logic_ptr> post_effect_logic_vector;

class c_std_quad_logic;
typedef boost::shared_ptr<c_std_quad_logic> std_quad_logic_ptr;

class c_post_effect : public boost::enable_shared_from_this<c_post_effect>
{
public:
    c_post_effect(const render_sys_context_ptr& render_sys_context, const std::wstring& fx_file_name);
    void apply(unsigned int technique);
    void add_logic(const post_effect_logic_ptr& logic)
    {
        if (logic)
            m_tech_logics.push_back(logic);
    }

    d3dx11_effect_ptr get_effect() const { return m_effect; }
    render_sys_context_ptr get_render_sys_context() const { return m_render_sys_context; }

private:
    HRESULT compile_shader(const std::wstring& fx_file_name);

protected:
    render_sys_context_ptr m_render_sys_context;
    d3dx11_effect_ptr m_effect;
    post_effect_logic_vector m_tech_logics;
};

class c_post_effect_logic
{
public:
    c_post_effect_logic(unsigned int tech_index)
        : m_tech_index(tech_index) {}
    
    virtual void create_resource(post_effect_ptr& post_effect) {}
    virtual void pre_apply(post_effect_ptr& post_effect) {}
    virtual void apply(post_effect_ptr& post_effect) {}
    virtual void post_apply(post_effect_ptr& post_effect) {}

protected:
    unsigned int m_tech_index;
};

class c_std_quad_logic : public c_post_effect_logic
{
    typedef c_post_effect_logic super;
public:
    c_std_quad_logic(const render_texture_ptr& rt_src, const render_texture_ptr& rt_target, unsigned int tech_index)
        : super(tech_index)
        , m_rt_src(rt_src)
        , m_rt_target(rt_target)
    {
    }

    virtual void pre_apply(post_effect_ptr& post_effect)
    {
        m_rt_src_sr->SetResource(m_rt_src->m_srv);
    }

    virtual void apply(post_effect_ptr& post_effect)
    {
        d3d11_device_context_ptr device_context = post_effect->get_render_sys_context()->get_d3d11_device_context();
        ID3D11RenderTargetView *rtv[] = {m_rt_target->m_rtv};

        device_context->OMSetRenderTargets(1, rtv, NULL); 
        post_effect->get_effect()->GetTechniqueByIndex(m_tech_index)->GetPassByIndex(0)->Apply(0, device_context);
        device_context->Draw(3, 0);
    }

    virtual void post_apply(post_effect_ptr& post_effect)
    {
        m_rt_src_sr->SetResource(NULL); 
    }

private:
    render_texture_ptr m_rt_src;
    render_texture_ptr m_rt_target;
    ID3DX11EffectShaderResourceVariable *m_rt_src_sr;
};