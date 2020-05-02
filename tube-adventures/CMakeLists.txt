include(qt_helpers)
find_qt()

add_subdirectory(lib) # Needs qt
add_subdirectory(exe)

option(TUBE_ADVENTURES_ENABLE_TESTING "Enable building and running unit tests" TRUE)
if(TUBE_ADVENTURES_ENABLE_TESTING)
	enable_testing()
	add_subdirectory(tests)
endif()
