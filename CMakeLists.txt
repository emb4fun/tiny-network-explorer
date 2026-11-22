cmake_minimum_required(VERSION 3.16)

project(tne
    VERSION 1.10
    DESCRIPTION "Tiny Network Explorer"
    LANGUAGES C
)

if(APPLE)
    message(FATAL_ERROR "macOS builds are not supported. Use Windows or Linux.")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(TNE_BUILD_CONSOLE "Build console poll tool for tnp_engine" ON)
option(TNE_BUILD_WX "Build wxWidgets GUI" ON)

if(MINGW)
    add_link_options(-static)
endif()

add_subdirectory(core)

if(TNE_BUILD_CONSOLE)
    add_subdirectory(tools)
endif()

if(TNE_BUILD_WX)
    enable_language(CXX)
    set(CMAKE_CXX_STANDARD 14)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    if(WIN32)
        set(wxWidgets_USE_STATIC ON)

        if(NOT wxWidgets_ROOT_DIR AND EXISTS "C:/msys64/ucrt64")
            set(wxWidgets_ROOT_DIR "C:/msys64/ucrt64")
        endif()
    else()
        set(wxWidgets_USE_STATIC OFF)
    endif()

    find_package(wxWidgets REQUIRED COMPONENTS core base)
    include(${wxWidgets_USE_FILE})

    add_subdirectory(src)
endif()
