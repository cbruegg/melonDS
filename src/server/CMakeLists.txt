project(server)

SET(SOURCES_SERVER main.cpp PlatformConfig.cpp PlatformConfig.h OutSocket.cpp OutSocket.h InSocket.cpp InSocket.h Semaphore.cpp Semaphore.h CommandHandler.cpp CommandHandler.h)


if (WIN32)
    set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -i <SOURCE> -o <OBJECT>")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")

find_package (Threads)
add_executable(melonDS-server ${SOURCES_SERVER})
target_link_libraries (melonDS-server Threads::Threads)
target_link_libraries(melonDS-server core)

if (UNIX)
    option(UNIX_PORTABLE "Make a portable build that looks for its configuration in the current directory" OFF)
    if (UNIX_PORTABLE)
        add_definitions(-DUNIX_PORTABLE)
    endif()

    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_libraries(melonDS-server dl)
    endif ()
elseif (WIN32)
    target_sources(melonDS-server PUBLIC "${CMAKE_SOURCE_DIR}/melon.rc")
    target_include_directories(melonDS-server PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
    target_link_libraries(melonDS-server comctl32 d2d1 dwrite uxtheme ws2_32 iphlpapi gdi32)
endif ()

install(FILES ../../net.kuribo64.melonDS.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
install(FILES ../../icon/melon_16x16.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/16x16/apps RENAME net.kuribo64.melonDS.png)
install(FILES ../../icon/melon_32x32.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/32x32/apps RENAME net.kuribo64.melonDS.png)
install(FILES ../../icon/melon_48x48.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/48x48/apps RENAME net.kuribo64.melonDS.png)
install(FILES ../../icon/melon_64x64.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/64x64/apps RENAME net.kuribo64.melonDS.png)
install(FILES ../../icon/melon_128x128.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/128x128/apps RENAME net.kuribo64.melonDS.png)
install(FILES ../../icon/melon_256x256.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/256x256/apps RENAME net.kuribo64.melonDS.png)
install(FILES ../../romlist.bin DESTINATION ${CMAKE_INSTALL_PREFIX}/share/melonDS)
install(TARGETS melonDS-server RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
