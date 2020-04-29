add_executable(tube-adventures
	src/main.cc
)

if (WIN32)
	option(TUBE_ADVENTURES_USE_WINDEPLOYQT_FOR_COPYING_DLLS
		"Use windeployqt (qt's depdency manager) to copy qt DLLs to the build folder"
		FALSE
	)

	function(add_windeployq_post_build_event)
		find_program(windeployqt "windeployqt") # Should be able to find it by searching in CMAKE_PREFIX_PATH
		if (NOT windeployqt)
			message(FATAL_ERROR "windeployqt not found")
		endif()

		# https://doc.qt.io/qt-5/windows-deployment.html
		# Some of these "--no-*" options might need to be removed in the future
		set(windeployqt_args
			"$<TARGET_FILE:tube-adventures>"
			"--no-patchqt"
			"--no-plugins"
			#"--no-libraries" copy main qt DLLs
			"--no-quick-import"
			"--no-translations"
			"--no-system-d3d-compiler"
			"--no-virtualkeyboard"
			"--no-compiler-runtime"
			"--no-webkit2"
			"--no-angle"
			"--no-opengl-sw"
		)
		if (MSVC)
			list(APPEND windeployqt_args "--pdb")
		endif()

		add_custom_command(
			TARGET tube-adventures
			POST_BUILD
			COMMAND "${windeployqt}"
			ARGS ${windeployqt_args}
		)
	endfunction()
endif()

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
	if (WIN32 AND TUBE_ADVENTURES_USE_WINDEPLOYQT_FOR_COPYING_DLLS)
		add_windeployq_post_build_event()
	else()
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
		$<$<CONFIG:Debug>:TUBE_ADVENTURES_DEBUG>
		TUBE_ADVENTURES_BUILD_TYPE=$<CONFIG>
)
