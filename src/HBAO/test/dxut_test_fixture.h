#pragma once 

#include "../pch.h"
#include "gtest/gtest.h"

class c_dxut_app_test_fixture
{
public:
    void setup()  {}
    void tear_down() {}

protected:
    void setup_dxut_callbacks(LPDXUTCALLBACKD3D11DEVICECREATED device_create_func,
        LPDXUTCALLBACKD3D11DEVICEDESTROYED device_destroy_func,
        LPDXUTCALLBACKD3D11SWAPCHAINRESIZED swap_chain_resize_func,
        LPDXUTCALLBACKD3D11SWAPCHAINRELEASING swap_chain_release_func,
        LPDXUTCALLBACKFRAMEMOVE frame_move_func,
        LPDXUTCALLBACKD3D11FRAMERENDER frame_render_func,
        LPDXUTCALLBACKMSGPROC msg_proc_func
        )
    {   
        DXUTSetCallbackD3D11DeviceCreated( device_create_func );
        DXUTSetCallbackD3D11DeviceDestroyed( device_destroy_func );
        DXUTSetCallbackD3D11SwapChainResized( swap_chain_resize_func );
        DXUTSetCallbackD3D11SwapChainReleasing( swap_chain_release_func );
        DXUTSetCallbackFrameMove( frame_move_func );
        DXUTSetCallbackD3D11FrameRender( frame_render_func );
        DXUTSetCallbackMsgProc( msg_proc_func );
    }

    void enter_main_loop()
    {
        DXUTMainLoop(); 
    }
    
    void init_dxut(const std::wstring& wnd_title, bool is_windows, int width, int height)
    {
        DXUTInit( true, true, NULL ); 
        DXUTSetCursorSettings( true, true ); 
        DXUTCreateWindow( wnd_title.c_str() );
        DXUTCreateDevice( D3D_FEATURE_LEVEL_11_0, is_windows, width, height);
    }


};

