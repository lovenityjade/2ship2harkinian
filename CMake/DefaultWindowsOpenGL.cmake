if(NOT DEFINED source_file)
    message(FATAL_ERROR "source_file is required")
endif()

file(READ "${source_file}" contents)
set(directx_default [[#ifdef ENABLE_DX11
    return WindowBackend::FAST3D_DXGI_DX11;
#endif
]])
string(FIND "${contents}" "${directx_default}" match_position)
if(match_position EQUAL -1)
    message(FATAL_ERROR "Unable to locate the Windows DirectX default backend block")
endif()

string(REPLACE "${directx_default}" "" contents "${contents}")
file(WRITE "${source_file}" "${contents}")
