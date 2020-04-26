add_executable(tube-adventures
	src/main.cc
)

target_link_libraries(tube-adventures
	PRIVATE
		#CONAN_PKG::clara
		tinyxml2
)

target_compile_features(tube-adventures
	PUBLIC
		cxx_std_17
)

target_compile_definitions(tube-adventures
	PRIVATE
		$<$<CONFIG:Debug>:ANDONI_DEBUG>
		ANDONI_BUILD_TYPE=$<CONFIG>
)
