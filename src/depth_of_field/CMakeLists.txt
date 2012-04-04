set(TARGET depth_of_field)

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
			CEGUISILLYImageCodec_d.lib)
	
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
			CEGUISILLYImageCodec.lib)

add_definitions(-DUNICODE)

set(SOURCE_FILES
	pch.h
	prerequisites.h
	app_listener.h
	scene.h
	scene.cpp
	depth_of_field_solution.h
	depth_of_field_solution.cpp
	utils.h
	utils.cpp
	dx11_dof_demo.cpp
	)

set(FX_FILES
	scene.fx
	dof.fx)

set(SRC_FILES ${SOURCE_FILES} ${FX_FILES})

source_group("source" FILES ${SOURCE_FILES})
source_group("shaders" FILES ${FX_FILES})

add_executable(${TARGET} WIN32 ${SRC_FILES})

target_link_libraries(${TARGET} debug  "${DEP_LIBS_DEBUG}" optimized "${DEP_LIBS_RELEASE}")

# Installation
install(TARGETS ${TARGET} RUNTIME DESTINATION bin/${TARGET})
install(FILES ${FX_FILES} DESTINATION bin/${TARGET})
install(DIRECTORY ${DEP_DLL_DEBUG_DIR} ${DEP_DLL_REL_DIR} DESTINATION bin/${TARGET})