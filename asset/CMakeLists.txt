set(SETTING_ASSET_FILEPATH "${CMAKE_CURRENT_BINARY_DIR}/data.bin" PARENT_SCOPE)

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/data.zip"
                          "${CMAKE_CURRENT_BINARY_DIR}/sha1.txt"
    COMMAND ${CMAKE_COMMAND} -E tar cf "${CMAKE_CURRENT_BINARY_DIR}/data.zip"
                --format=zip dlc templates worldgen Asubworlds.json
    COMMAND ${CMAKE_COMMAND} -E copy sha1.txt "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS "${CMAKE_SOURCE_DIR}/asset/sha1.txt"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/asset"
    VERBATIM
)

# the neocities can not upload .zip file, so add a header.
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/data.bin"
    COMMAND ${CMAKE_COMMAND} -E cat "sha1.txt" "data.zip" > "data.bin"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/sha1.txt"
            "${CMAKE_CURRENT_BINARY_DIR}/data.zip"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    VERBATIM
)

add_custom_target(asset DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/data.bin")
