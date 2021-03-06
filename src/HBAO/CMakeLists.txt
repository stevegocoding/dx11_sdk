set(TARGET HBAO)

# Dependencies
set(DEP_LIBS_DEBUG	d3d11.lib 
			d3dx11d.lib 
			effects11_d.lib 
			d3dcompiler.lib 
			d3d9.lib 
			d3dx9d.lib 
			dxgi.lib 
			dxguid.lib 
			dxerr.lib 
			winmm.lib 
			comctl32.lib 
			dxut_d.lib 
			dxutopt_d.lib 
			CEGUIBase_d.lib 
			CEGUIDirect3D11Renderer_d.lib 
			CEGUIExpatParser_d.lib 
			CEGUIFalagardWRBase_d.lib 
			CEGUILuaScriptModule_d.lib 
			CEGUISILLYImageCodec_d.lib
            gtestd_dll.lib
            )
	
set(DEP_LIBS_RELEASE	d3d11.lib 
			d3dx11.lib 
			effects11.lib 
			d3dcompiler.lib 
			d3d9.lib 
			d3dx9.lib 
			dxgi.lib 
			dxguid.lib 
			dxerr.lib 
			winmm.lib 
			comctl32.lib 
			dxut.lib 
			dxutopt.lib
			CEGUIBase.lib
			CEGUIDirect3D11Renderer.lib
			CEGUIExpatParser.lib
			CEGUIFalagardWRBase.lib
			CEGUILuaScriptModule.lib
			CEGUISILLYImageCodec.lib
            gtest_dll.lib
            )

add_definitions(-DUNICODE)


set(SOURCE_FILES
	pch.h
	prerequisites.h
	app_listener.h
	app_event.h
	hbao_app.h
	hbao_app.cpp
	hbao_scene.h
	hbao_scene.cpp
	hbao_renderer.h
	hbao_renderer.cpp
	hbao_bilateral_blur.h
	hbao_bilateral_blur.cpp
	hbao_gui.h
	hbao_gui.cpp
	render_system_context.h
	render_system_context.cpp
	utils.h
	utils.cpp
	hbao_main.cpp
	)

set(FX_FILES
	scene.fx
	hbao.fx
	bilateral_blur.fx
	linearize_depth_non_msaa.fx
	resolve_linearize_nd_msaa.fx
	fullscreen_triangle_vs.hlsl
	hbao_params_cb.hlsl
	)
    
set(TEST_FILES
	test/tests.h
	test/tests.cpp
	test/test_scene.h
	test/test_scene.cpp
	test/test_hbao.h
	test/test_hbao.cpp
	test/test_bilateral_blur.h
	test/test_bilateral_blur.cpp
	test/test_hud_gui.h
	test/test_hud_gui.cpp
	test/dxut_test_fixture.h
	test/dxut_test_fixture.cpp)

set(SRC_FILES ${SOURCE_FILES} ${FX_FILES} ${TEST_FILES})

source_group("source" FILES ${SOURCE_FILES})
source_group("shaders" FILES ${FX_FILES})
source_group("test" FILES ${TEST_FILES})

add_executable(${TARGET} WIN32 ${SRC_FILES})
SET_TARGET_PROPERTIES(${TARGET} PROPERTIES DEBUG_POSTFIX "_debug")
SET_TARGET_PROPERTIES(${TARGET} PROPERTIES RELEASE_POSTFIX "_release")

target_link_libraries(${TARGET} debug "${DEP_LIBS_DEBUG}" optimized "${DEP_LIBS_RELEASE}")

# Installation
install(TARGETS ${TARGET} RUNTIME DESTINATION bin/${TARGET})
install(FILES ${FX_FILES} DESTINATION bin/${TARGET})
install(DIRECTORY ${DEP_DLL_DEBUG_DIR} ${DEP_DLL_REL_DIR} DESTINATION bin/${TARGET})

MESSAGE ("${DEP_LIBS_DEBUG}")