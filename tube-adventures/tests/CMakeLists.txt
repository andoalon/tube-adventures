if (MSVC AND MSVC_VERSION LESS 1910) # If using visual studio older than 2019
	# #pragma warning: there is no warning number '4800'
	# catch tries to disable a warning introduced in visual studio 2019
	# also in previous versions
	add_compile_options(/wd4619)
endif()

# automatically enable catch2 to generate ctest targets
if(CONAN_CATCH2_ROOT_DEBUG)
  include(${CONAN_CATCH2_ROOT_DEBUG}/lib/cmake/Catch2/Catch.cmake)
else()
  include(${CONAN_CATCH2_ROOT}/lib/cmake/Catch2/Catch.cmake)
endif()

add_library(catch_main OBJECT catch_main.cc)
target_link_libraries(catch_main PUBLIC CONAN_PKG::catch2)
target_compile_features(catch_main PUBLIC cxx_std_17)
target_compile_definitions(catch_main PUBLIC CATCH_CONFIG_ENABLE_ALL_STRINGMAKERS)

add_executable(tests
    tests/annotations.tests.cc
)
target_link_libraries(tests
	PRIVATE
        catch_main
        tube-adventures-lib
)
add_copy_qt_dependencies_post_build_event(tests)

# automatically discover tests that are defined in catch based test files you
# can modify the unittests. TEST_PREFIX to whatever you want, or use different
# for different binaries
catch_discover_tests(
  tests
  TEST_PREFIX "tests."
  EXTRA_ARGS
  	--reporter=xml
	--out=tests.xml
)

set(constexpr_test_files
    constexpr_tests/constexpr_tests.cc
)

# Add a file containing a set of constexpr tests
add_executable(constexpr_tests ${constexpr_test_files})
target_link_libraries(constexpr_tests
	PRIVATE
        catch_main
        tube-adventures-lib
)
add_copy_qt_dependencies_post_build_event(constexpr_tests)

catch_discover_tests(
  constexpr_tests
  TEST_PREFIX "constexpr."
  EXTRA_ARGS
  	--reporter=xml
	--out=constexpr.xml
)

# Disable the constexpr portion of the test, and build again this allows us to
# have an executable that we can debug when things go wrong with the constexpr
# testing
add_executable(runtime_constexpr_tests ${constexpr_test_files})
target_link_libraries(runtime_constexpr_tests
	PRIVATE
        catch_main
        tube-adventures-lib
)
add_copy_qt_dependencies_post_build_event(runtime_constexpr_tests)

target_compile_definitions(runtime_constexpr_tests
	PRIVATE
		CATCH_CONFIG_RUNTIME_STATIC_REQUIRE
)


catch_discover_tests(
  runtime_constexpr_tests
  TEST_PREFIX "runtime_constexpr."
  EXTRA_ARGS
  --reporter=xml
  --out=runtime_constexpr.xml
)

set_target_properties(catch_main tests constexpr_tests runtime_constexpr_tests
    PROPERTIES
        FOLDER "unit tests"
)
