set(timyxml2_source_directory "${CMAKE_CURRENT_LIST_DIR}/src")
set(timyxml2_include_directory "${CMAKE_CURRENT_LIST_DIR}/include")

add_library(tinyxml2
    ${timyxml2_include_directory}/tinyxml2/tinyxml2.h

    ${timyxml2_source_directory}/tinyxml2.cpp
)

target_include_directories(tinyxml2
    INTERFACE
        ${timyxml2_include_directory}
    PRIVATE
        ${timyxml2_include_directory}/tinyxml2
)

set_target_properties(tinyxml2
    PROPERTIES
        FOLDER "extern"
)
