#pragma once
#include "app_listener.h"
#include "utils.h"

class c_app_post_effect : public c_dx11_demo_app_listener
{
public:
    HRESULT on_create_device11(ID3D11Device *d3d11_device, const DXGI_SURFACE_DESC *backbuf_surface_desc);
    void on_destroy_device11(); 

    void on_frame_render(ID3D11Device *d3d11_device, ID3D11DeviceContext *d3d11_device_context, double time, float elapsed_time);

private:
    post_effect_logic_vector m_post_effect_logics; 
    
    
};

typedef boost::shared_ptr<c_app_post_effect> app_post_effect_ptr;