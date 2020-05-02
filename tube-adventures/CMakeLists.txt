include(qt_helpers)
find_qt()

add_subdirectory(lib) # Needs qt
add_subdirectory(exe)

if(TUBE_ADVENTURES_ENABLE_TESTING)
	add_subdirectory(tests)
endif()
