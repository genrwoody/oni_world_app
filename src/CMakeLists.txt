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

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/GenConfig.cmake" "
file(SIZE \${SETTING_ASSET_FILEPATH} SETTING_ASSET_FILESIZE)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in CONFIG_H_IN_FILE)
file(CONFIGURE OUTPUT \${CMAKE_CURRENT_BINARY_DIR}/config.h
               CONTENT \${CONFIG_H_IN_FILE}
               @ONLY)"
)

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/config.h"
    COMMAND ${CMAKE_COMMAND} -DSETTING_ASSET_FILEPATH=${SETTING_ASSET_FILEPATH}
                             -P ${CMAKE_CURRENT_BINARY_DIR}/GenConfig.cmake
    DEPENDS ${SETTING_ASSET_FILEPATH} asset "config.h.in"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    VERBATIM
)

add_library(libOniWorldApp STATIC
    ${SETTING_HEADER_FILES}
    "${CMAKE_CURRENT_BINARY_DIR}/JsonDeserializeGen.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/config.h"
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
    "Utils/InplaceVector.hpp"
    "WorldGen.hpp"
    "WorldGen.cpp"
    "TemplateSpawning.cpp"
    "App.hpp"
    "App.cpp"
)
target_include_directories(libOniWorldApp
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}"
    PRIVATE "${CMAKE_SOURCE_DIR}/3rdparty/clipper"
    PRIVATE "${CMAKE_SOURCE_DIR}/3rdparty/jsoncpp"
    PRIVATE "${CMAKE_SOURCE_DIR}/3rdparty/miniz"
)

add_executable(oniWorldApp "main.cpp")
target_link_libraries(oniWorldApp PUBLIC libOniWorldApp clipper jsoncpp miniz)

if(EMSCRIPTEN)
target_compile_options(libOniWorldApp PUBLIC "-Wall" "-Wextra" "-Werror")
target_link_options(oniWorldApp
    PUBLIC "-sALLOW_MEMORY_GROWTH" "-sINITIAL_HEAP=33554432"
    PUBLIC "-sMINIMAL_RUNTIME=1" "-sEXPORTED_RUNTIME_METHODS=[HEAP32,HEAPU8]"
    PUBLIC "--pre-js" "${CMAKE_CURRENT_SOURCE_DIR}/jsUtils/prefix.js"
    PUBLIC "--js-library" "${CMAKE_CURRENT_SOURCE_DIR}/jsUtils/getresult.js")
# the neocities can not upload .wasm file, so rename it.
add_custom_command(TARGET oniWorldApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy oniWorldApp.wasm wasm.bin
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    VERBATIM
)
list(APPEND WASM_LINK_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/jsUtils/prefix.js"
    "${CMAKE_CURRENT_SOURCE_DIR}/jsUtils/getresult.js"
)
set_target_properties(oniWorldApp PROPERTIES LINK_DEPENDS "${WASM_LINK_DEPENDS}")
elseif(MSVC)
target_compile_options(libOniWorldApp PUBLIC /MP /W4 /WX /wd4244)
target_compile_options(oniWorldApp
    PUBLIC "/source-charset:utf-8" "/execution-charset:GBK"
)
endif()
