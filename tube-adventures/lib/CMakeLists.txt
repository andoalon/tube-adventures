add_library(tube-adventures-lib OBJECT
	src/mainwindow.hh
	src/mainwindow.cc
	src/annotations.hh
	src/annotations.cc
)

target_include_directories(tube-adventures-lib
	INTERFACE
		src
)

set_target_properties(tube-adventures-lib
	PROPERTIES
		AUTOMOC ON
		AUTOUIC ON
		AUTOUIC_SEARCH_PATHS "src/ui/"
)

target_link_libraries(tube-adventures-lib
	PUBLIC
		Qt5::Core
		Qt5::Widgets
		Qt5::Gui
		Qt5::Multimedia
		Qt5::MultimediaWidgets
)

set(ui_files
	src/ui/mainwindow.ui)
source_group("ui" FILES ${ui_files})

if (MSVC)
	target_sources(tube-adventures-lib
		PRIVATE
			${ui_files}
	)
endif()

target_link_libraries(tube-adventures-lib
	PRIVATE
		tinyxml2
)

target_compile_features(tube-adventures-lib
	PUBLIC
		cxx_std_17
)

target_compile_definitions(tube-adventures-lib
	PUBLIC
		$<$<CONFIG:Debug>:TUBE_ADVENTURES_DEBUG>
		TUBE_ADVENTURES_BUILD_TYPE=$<CONFIG>
)

