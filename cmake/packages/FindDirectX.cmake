# -*- cmake -*-

if (VIEWER AND WINDOWS)
  find_path(DIRECTX_INCLUDE_DIR dxdiag.h
            "$ENV{DXSDK_DIR}/Include"
            "$ENV{PROGRAMFILES}/Microsoft DirectX SDK (June 2010)/Include"
            )
  if (DIRECTX_INCLUDE_DIR)
    include_directories(${DIRECTX_INCLUDE_DIR})
    if (DIRECTX_FIND_QUIETLY)
      message(STATUS "Found DirectX include: ${DIRECTX_INCLUDE_DIR}")
    endif (DIRECTX_FIND_QUIETLY)
  else (DIRECTX_INCLUDE_DIR)
    message(FATAL_ERROR "Could not find DirectX SDK Include")
  endif (DIRECTX_INCLUDE_DIR)


  find_path(DIRECTX_LIBRARY_DIR dxguid.lib
            "$ENV{DXSDK_DIR}/Lib/x86"
            "$ENV{PROGRAMFILES}/Microsoft DirectX SDK (June 2010)/Lib/x86"
            )
  if (DIRECTX_LIBRARY_DIR)
    if (DIRECTX_FIND_QUIETLY)
      message(STATUS "Found DirectX include: ${DIRECTX_LIBRARY_DIR}")
    endif (DIRECTX_FIND_QUIETLY)
  else (DIRECTX_LIBRARY_DIR)
    message(FATAL_ERROR "Could not find DirectX SDK Libraries")
  endif (DIRECTX_LIBRARY_DIR)

endif (VIEWER AND WINDOWS)

