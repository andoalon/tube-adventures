add_executable(tube-adventures
	src/main.cc
)

set_target_properties(tube-adventures
	PROPERTIES
		AUTOMOC ON
		AUTOUIC ON
		AUTOUIC_SEARCH_PATHS "src/ui/"
)

find_package(Qt5 COMPONENTS Core Widgets Gui) # not required (not yet)
if (Qt5_FOUND)
	target_compile_definitions(tube-adventures
		PRIVATE
			TUBE_ADVENTURES_HAVE_QT)

	target_link_libraries(tube-adventures
		PRIVATE
			Qt5::Core
			Qt5::Widgets
			Qt5::Gui
	)

	target_sources(tube-adventures
		PRIVATE
			src/mainwindow.hh
			src/mainwindow.cc
	)

	set(ui_files
		src/ui/mainwindow.ui)
	source_group("ui" FILES ${ui_files})

	if (MSVC)
		target_sources(tube-adventures
			PRIVATE
				${ui_files}
		)
	endif()
	
	# For when I use windeployqt.exe, remember to use '--no-compiler-runtime'
	# unless it's meant for generating an installer
	if (TRUE)
		function(copy_output_dll_post_build from_target to_target)
			add_custom_command(
				TARGET tube-adventures
				POST_BUILD
    			COMMAND ${CMAKE_COMMAND} -E copy_if_different
    		    $<TARGET_FILE:${from_target}>
    		    $<TARGET_FILE_DIR:${to_target}>
			)
		endfunction()

		copy_output_dll_post_build(Qt5::Core tube-adventures)
		copy_output_dll_post_build(Qt5::Widgets tube-adventures)
		copy_output_dll_post_build(Qt5::Gui tube-adventures)
	endif()
endif()

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
