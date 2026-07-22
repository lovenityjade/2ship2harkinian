if(NOT DEFINED source_dir)
    message(FATAL_ERROR "source_dir is required")
endif()

set(patches
    "websocketpp/endpoint.hpp|endpoint<connection,config>(|endpoint("
    "websocketpp/logger/basic.hpp|basic<concurrency,names>(|basic("
    "websocketpp/roles/server_endpoint.hpp|server<config>(|server("
)

foreach(patch IN LISTS patches)
    string(REPLACE "|" ";" fields "${patch}")
    list(GET fields 0 relative_path)
    list(GET fields 1 before)
    list(GET fields 2 after)
    set(file_path "${source_dir}/${relative_path}")
    file(READ "${file_path}" contents)
    string(REPLACE "${before}" "${after}" contents "${contents}")
    file(WRITE "${file_path}" "${contents}")
endforeach()
