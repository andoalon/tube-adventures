add_executable(tube-adventures
    main.cc
)

target_link_libraries(tube-adventures
    PRIVATE
        tube-adventures-lib
)

add_copy_qt_dependencies_post_build_event(tube-adventures)
