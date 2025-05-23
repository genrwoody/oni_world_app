set(SETTING_HEADER_FILES
    "Setting/ClusterLayout.hpp"
    "Setting/DefaultSettings.hpp"
    "Setting/SampleDescriber.hpp"
    "Setting/SimHashes.hpp"
    "Setting/TemplateContainer.hpp"
    "Setting/World.hpp"
    "Setting/WorldGenClasses.hpp"
)

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/JsonDeserializeGen.hpp"
    COMMAND ${PYTHON_CMD} "${CMAKE_CURRENT_SOURCE_DIR}/JsonDeserializeGen.py"
                          "${CMAKE_CURRENT_SOURCE_DIR}/Setting"
    DEPENDS ${SETTING_HEADER_FILES} "JsonDeserializeGen.py"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    VERBATIM
)

if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/data.bin")
execute_process(COMMAND ${CMAKE_COMMAND} -E echo_append "keep"
    OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/keep.txt"
)
execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar cf "${CMAKE_CURRENT_BINARY_DIR}/data.zip"
                --format=zip dlc templates worldgen Asubworlds.json
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/asset"
)
execute_process(COMMAND ${CMAKE_COMMAND} -E cat "keep.txt" "data.zip"
    OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/data.bin"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
endif()

file(SIZE "${CMAKE_CURRENT_BINARY_DIR}/data.zip" SETTING_ASSET_FILESIZE)

add_executable(oniWorldApp
    ${SETTING_HEADER_FILES}
    "${CMAKE_CURRENT_BINARY_DIR}/JsonDeserializeGen.hpp"
    "Setting/ComposableDictionary.hpp"
    "Setting/JsonDeserialize.hpp"
    "Setting/SettingsCache.hpp"
    "Setting/WorldGenClasses.cpp"
    "Setting/SettingsCache.cpp"
    "Utils/ConvexHull/Constants.hpp"
    "Utils/ConvexHull/Algorithm.hpp"
    "Utils/ConvexHull/Algorithm.cpp"
    "Utils/ConvexHull/DataModel.hpp"
    "Utils/ConvexHull/MathHelper.hpp"
    "Utils/ConvexHull/MathHelper.cpp"
    "Utils/ConvexHull.hpp"
    "Utils/ConvexHull.cpp"
    "Utils/KRandom.hpp"
    "Utils/KRandom.cpp"
    "Utils/PointGenerator.hpp"
    "Utils/PointGenerator.cpp"
    "Utils/Vector2f.hpp"
    "Utils/Polygon.hpp"
    "Utils/Polygon.cpp"
    "Utils/Halfedge.hpp"
    "Utils/Halfedge.cpp"
    "Utils/Voronoi.hpp"
    "Utils/Voronoi.cpp"
    "Utils/Diagram.hpp"
    "Utils/Diagram.cpp"
    "WorldGen.hpp"
    "WorldGen.cpp"
    "TemplateSpawning.cpp"
    "main.cpp"
)
if(EMSCRIPTEN)
target_compile_options(oniWorldApp PUBLIC "-Wall" "-Wextra" "-Werror")
target_link_options(oniWorldApp
    PUBLIC "-sALLOW_MEMORY_GROWTH" "-sINITIAL_HEAP=33554432"
    PUBLIC "-sMINIMAL_RUNTIME=1" "-sEXPORTED_RUNTIME_METHODS=[HEAP32,HEAPU8]"
    PUBLIC "--pre-js" "${CMAKE_CURRENT_SOURCE_DIR}/jsUtils/prefix.js"
    PUBLIC "--js-library" "${CMAKE_CURRENT_SOURCE_DIR}/jsUtils/getresult.js")
add_custom_command(TARGET oniWorldApp POST_BUILD
    COMMAND ${PYTHON_CMD} "${CMAKE_CURRENT_SOURCE_DIR}/JoinWasmFilesHash.py"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    VERBATIM
)
list(APPEND WASM_LINK_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/jsUtils/prefix.js"
    "${CMAKE_CURRENT_SOURCE_DIR}/jsUtils/getresult.js"
    "${CMAKE_CURRENT_SOURCE_DIR}/JoinWasmFilesHash.py"
)
set_target_properties(oniWorldApp PROPERTIES LINK_DEPENDS "${WASM_LINK_DEPENDS}")
elseif(MSVC)
target_compile_options(oniWorldApp
    PUBLIC "/source-charset:utf-8" "/execution-charset:GBK")
endif()
target_include_directories(oniWorldApp
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
    PUBLIC "${CMAKE_CURRENT_BINARY_DIR}"
    PUBLIC "${CMAKE_SOURCE_DIR}/3rdparty/clipper"
    PUBLIC "${CMAKE_SOURCE_DIR}/3rdparty/jsoncpp"
    PUBLIC "${CMAKE_SOURCE_DIR}/3rdparty/miniz"
)
target_link_libraries(oniWorldApp PUBLIC clipper jsoncpp miniz)

configure_file("config.h.in" "config.h")
