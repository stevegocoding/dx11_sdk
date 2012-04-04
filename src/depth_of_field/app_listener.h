#pragma once

#include "pch.h"
#include "prerequisites.h"
#include "utils.h"

class c_dx11_demo_app_listener
{
public:
    virtual HRESULT on_d3d11_create_device(ID3D11Device *d3d11_device, const DXGI_SURFACE_DESC *backbuf_surface_desc) { return S_OK; }
    virtual void on_d3d11_destroy_device() {};
    virtual HRESULT on_d3d11_resized_swap_chain(IDXGISwapChain *swap_chain, const DXGI_SURFACE_DESC *backbuf_surface_desc) { return S_OK; }
    virtual void on_d3d11_releasing_swap_chain() {}
    virtual void on_frame_render(ID3D11Device *d3d11_device, ID3D11DeviceContext *d3d11_device_context, double time, float elapsed_time) {}
    virtual void on_frame_update(double time, double elapsed_time) {}
    virtual void on_win_msg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext) {}
};
