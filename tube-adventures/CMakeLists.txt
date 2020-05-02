add_subdirectory(lib)

include(copy_qt_dependencies)

add_subdirectory(exe)

option(TUBE_ADVENTURES_ENABLE_TESTING "Enable building and running unit tests" TRUE)
if(TUBE_ADVENTURES_ENABLE_TESTING)
	enable_testing()
	message(STATUS
		"Building Tests. Be sure to check out test/constexpr_tests for constexpr testing"
	)
	add_subdirectory(tests)
endif()
