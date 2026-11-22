add_library(tnp_engine STATIC
    tnp_engine.c
)

if(WIN32)
    target_sources(tnp_engine PRIVATE tnp_platform_win.c)
else()
    target_sources(tnp_engine PRIVATE tnp_platform_posix.c)
endif()

target_include_directories(tnp_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WIN32)
    target_link_libraries(tnp_engine PUBLIC ws2_32)
endif()
