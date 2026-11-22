set(TNE_GUI_SOURCES
    TnpApp.cpp
    MainFrame.cpp
    ChangeDialog.cpp
    AboutDialog.cpp
    AppIcon.cpp
)

if(WIN32)
    add_executable(tnexpl WIN32 ${TNE_GUI_SOURCES})
else()
    add_executable(tnexpl ${TNE_GUI_SOURCES})
endif()

target_include_directories(tnexpl
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(tnexpl
    PRIVATE
        tnp_engine
        ${wxWidgets_LIBRARIES}
)

target_compile_definitions(tnexpl PRIVATE ${wxWidgets_DEFINITIONS})
target_compile_options(tnexpl PRIVATE ${wxWidgets_CXX_FLAGS})

if(WIN32)
    target_sources(tnexpl PRIVATE tne.rc)
    target_link_libraries(tnexpl PRIVATE uxtheme)

    set_source_files_properties(
        tne.rc
        PROPERTIES
            OBJECT_DEPENDS
                "${CMAKE_CURRENT_SOURCE_DIR}/resources/app.ico;${CMAKE_CURRENT_SOURCE_DIR}/resources/nutchip.bmp;${CMAKE_CURRENT_SOURCE_DIR}/resources/nutchips.bmp;${CMAKE_CURRENT_SOURCE_DIR}/version.rc;${CMAKE_CURRENT_SOURCE_DIR}/version.h;${CMAKE_CURRENT_SOURCE_DIR}/wx_resources.rc")

    add_custom_command(
        TARGET tnexpl
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:tnexpl>/resources"
        COMMENT "Remove stale resources directory (assets are embedded in tnexpl.exe)"
    )
else()
    # Linux: icons/bitmaps are loaded from resources/ next to the tnexpl binary.
    file(GLOB TNE_RESOURCE_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/*")

    set(TNE_RESOURCES_STAMP "${CMAKE_CURRENT_BINARY_DIR}/resources.stamp")
    add_custom_command(
        OUTPUT "${TNE_RESOURCES_STAMP}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/resources"
            "${CMAKE_CURRENT_BINARY_DIR}/resources"
        COMMAND ${CMAKE_COMMAND} -E touch "${TNE_RESOURCES_STAMP}"
        DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/tnexpl.desktop.in"
            ${TNE_RESOURCE_FILES}
        COMMENT "Copy GUI resources next to tnexpl binary"
        VERBATIM
    )

    add_custom_target(tnexpl_resources DEPENDS "${TNE_RESOURCES_STAMP}")
    add_dependencies(tnexpl tnexpl_resources)

    set(TNE_DESKTOP_EXEC "${CMAKE_CURRENT_BINARY_DIR}/tnexpl")
    set(TNE_DESKTOP_ICON "${CMAKE_CURRENT_BINARY_DIR}/resources/app.png")
    set(TNE_DESKTOP_PATH "${CMAKE_CURRENT_BINARY_DIR}")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/tnexpl.desktop.in"
        "${CMAKE_CURRENT_BINARY_DIR}/tnexpl.desktop"
        @ONLY
    )
endif()
