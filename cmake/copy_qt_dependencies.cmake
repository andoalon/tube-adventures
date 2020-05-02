include_guard()

function(add_copy_qt_dependencies_post_build_event target)
	if (NOT DEFINED target)
		message(FATAL_ERROR "The 'target' parameter is required")
	endif()

	if (WIN32)
		option(TUBE_ADVENTURES_USE_WINDEPLOYQT_FOR_COPYING_DLLS
			"Use windeployqt (qt's dependency manager) to copy qt DLLs to the build folder"
			TRUE
		)

		function(add_windeployq_post_build_event target)
			find_program(windeployqt "windeployqt") # Should be able to find it by searching in CMAKE_PREFIX_PATH
			if (NOT windeployqt)
				message(FATAL_ERROR "windeployqt not found")
			endif()

			# Downloaded: K-Lite_Codec_Pack_1375_Basic.exe http://downloads.ddigest-dl.com/software/download.php?sid=1089&ssid=0&did=374

			# https://doc.qt.io/qt-5/windows-deployment.html
			# Some of these "--no-*" options might need to be removed in the future
			set(windeployqt_args
				"$<TARGET_FILE:${target}>"
				"--no-patchqt"
				#"--no-plugins" plugins needed for video
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
				TARGET ${target}
				POST_BUILD
				COMMAND "${windeployqt}"
				ARGS ${windeployqt_args}
			)
		endfunction()
	endif()

	if (WIN32 AND TUBE_ADVENTURES_USE_WINDEPLOYQT_FOR_COPYING_DLLS)
		add_windeployq_post_build_event(${target})
	else()
		function(copy_output_dll_post_build from_target to_target)
			if (NOT DEFINED from_target)
				message(FATAL_ERROR "The 'from_target' parameter is required")
			endif()

			if (NOT DEFINED to_target)
				message(FATAL_ERROR "The 'target' parameter is required")
			endif()

			add_custom_command(
				TARGET ${to_target}
				POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different
					$<TARGET_FILE:${from_target}>
					$<TARGET_FILE_DIR:${to_target}>
			)
		endfunction()

		copy_output_dll_post_build(Qt5::Core ${target})
		copy_output_dll_post_build(Qt5::Widgets ${target})
		copy_output_dll_post_build(Qt5::Gui ${target})
		copy_output_dll_post_build(Qt5::Multimedia ${target})
		copy_output_dll_post_build(Qt5::MultimediaWidgets ${target})
		copy_output_dll_post_build(Qt5::Network ${target})
		copy_output_dll_post_build(Qt5::OpenGl ${target})
	endif()
endfunction()