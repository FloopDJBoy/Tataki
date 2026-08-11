cmake_minimum_required(VERSION 4.3)
project(chessEngine)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_PREFIX_PATH "E:\\Qt\\6.11.1\\mingw_64")
find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        Svg
        REQUIRED)

add_executable(chessEngine src/main.cpp
        src/main.cpp
        src/Gui/MainWindow.cpp
        src/Gui/MainWindow.h
        src/Gui/Board.cpp
        src/Gui/Board.h
        src/Types.h
        resources.qrc
        src/ChessCore/Move.h
        src/ChessCore/Board.cpp
        src/ChessCore/Board.h
        src/ChessCore/Position.cpp
        src/ChessCore/Position.h
        src/ChessCore/FenHelper.h
        src/ChessCore/BitBoards.h
        src/ChessCore/PrecomputedMagics.h
        src/ChessCore/MoveGen.cpp
        src/ChessCore/MoveGen.h
        src/ChessCore/preft.cpp
        src/ChessCore/preft.h
        src/Engine/Search.cpp
        src/Engine/Search.h
        src/Uci.cpp
        src/Uci.h
        src/Engine/OpeningBook.cpp
        src/Engine/OpeningBook.h
        src/Engine/Engine.cpp
        src/Engine/Engine.h
        src/Engine/SearchLimits.h
        src/Engine/Eval.cpp
        src/Engine/Eval.h
        src/Engine/TranspositionTable.cpp
        src/Engine/TranspositionTable.h
        src/Engine/Zobrist.h
)
target_include_directories( chessEngine PRIVATE
        ${PROJECT_SOURCE_DIR}/external
        ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(chessEngine
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt6::Svg
)
add_custom_command(TARGET chessEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:chessEngine>/assets"
)
get_filename_component(MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)

add_custom_command(TARGET chessEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${MINGW_BIN_DIR}/libgcc_s_seh-1.dll"
        "$<TARGET_FILE_DIR:chessEngine>"

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${MINGW_BIN_DIR}/libstdc++-6.dll"
        "$<TARGET_FILE_DIR:chessEngine>"

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${MINGW_BIN_DIR}/libwinpthread-1.dll"
        "$<TARGET_FILE_DIR:chessEngine>"
)
if (WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(DEBUG_SUFFIX)
    if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif ()
    set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
    if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
        set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        endif ()
    endif ()
    if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
    endif ()
    foreach (QT_LIB Core Gui Widgets Svg)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach (QT_LIB)
endif ()
