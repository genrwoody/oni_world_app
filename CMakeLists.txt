cmake_minimum_required (VERSION 3.20)

project("oniWorldApp")

set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(EMSCRIPTEN)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
endif()

if("$ENV{EMSDK_PYTHON}" STREQUAL "")
    find_program(PYTHON_CMD "python" REQUIRED)
else()
    set(PYTHON_CMD "$ENV{EMSDK_PYTHON}")
endif()

add_library(clipper STATIC
    "3rdparty/clipper/clipper.cpp"
)
target_include_directories(clipper
    PUBLIC "${CMAKE_SOURCE_DIR}/3rdparty/clipper"
)

add_library(jsoncpp STATIC
    "3rdparty/jsoncpp/json_reader.cpp"
    "3rdparty/jsoncpp/json_writer.cpp"
    "3rdparty/jsoncpp/json_value.cpp"
)
target_include_directories(jsoncpp
    PUBLIC "${CMAKE_SOURCE_DIR}/3rdparty/jsoncpp"
)

add_library(miniz STATIC
    "3rdparty/miniz/miniz.c"
    "3rdparty/miniz/miniz_tdef.c"
    "3rdparty/miniz/miniz_tinfl.c"
    "3rdparty/miniz/miniz_zip.c"
)
target_compile_definitions(miniz PUBLIC MINIZ_NO_TIME MINIZ_NO_STDIO)
target_include_directories(miniz
    PUBLIC "${CMAKE_CURRENT_BINARY_DIR}"
    PUBLIC "${CMAKE_SOURCE_DIR}/3rdparty/miniz"
)
set_target_properties(miniz
    PROPERTIES INTERFACE_COMPILE_DEFINITIONS MINIZ_STATIC_DEFINE)

add_subdirectory(asset)
add_subdirectory(src)

install(FILES ${SETTING_ASSET_FILEPATH} DESTINATION ${CMAKE_INSTALL_PREFIX})
install(TARGETS oniWorldApp DESTINATION ${CMAKE_INSTALL_PREFIX})
