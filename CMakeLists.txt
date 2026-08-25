cmake_minimum_required(VERSION 4.3)
project(chessEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTORCC ON)
#set(CMAKE_AUTOUIC ON)

#set(CMAKE_PREFIX_PATH "E:/Qt/6.11.1/msvc2022_64")
#find_package(Qt6 COMPONENTS
#        Core
#        Gui
#        Widgets
#        Svg
#        REQUIRED)

#src/Gui/MainWindow.cpp
#src/Gui/MainWindow.h
#src/Gui/Board.cpp
#src/Gui/Board.h
#resources.qrc
set(ENGINE_NAME "Sashimi")
set(ENGINE_VERSION "v0.13.2")

add_executable(chessEngine
        src/main.cpp
        src/Types.h
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
        src/misc/preft.cpp
        src/misc/preft.h
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
        src/main.cpp
        src/misc/pgn_extract.h
        src/Engine/PawnHash.h
        src/misc/LazyStats.h
        src/misc/LazyStats.cpp
        src/Engine/PawnTT.cpp
        src/Engine/PawnTT.h
        src/misc/SearchStats.h
        src/Engine/History.h
        src/Engine/MovePicker.cpp
        src/Engine/MovePicker.h
)
target_compile_definitions(chessEngine PRIVATE
        ENGINE_NAME="${ENGINE_NAME}"
        ENGINE_VERSION="${ENGINE_VERSION}"
)
target_include_directories( chessEngine PRIVATE
        ${PROJECT_SOURCE_DIR}/external
        ${PROJECT_SOURCE_DIR}/src
)
set_target_properties(chessEngine PROPERTIES
        OUTPUT_NAME ${ENGINE_NAME}_${ENGINE_VERSION}
)
#target_link_libraries(chessEngine
#        Qt6::Core
#        Qt6::Gui
#        Qt6::Widgets
#        Qt6::Svg
#)
add_custom_command(TARGET chessEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:chessEngine>/assets"
)
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(chessEngine PRIVATE
            $<$<CONFIG:RelWithDebInfo>:/fsanitize=address>
            $<$<CONFIG:RelWithDebInfo>:/Oy->
    )

    target_link_options(chessEngine PRIVATE
            $<$<CONFIG:RelWithDebInfo>:/INCREMENTAL:NO>
    )

    target_link_libraries(chessEngine PRIVATE
            $<$<CONFIG:RelWithDebInfo>:E:/clang+llvm-22.1.8-x86_64-pc-windows-msvc/clang+llvm-22.1.8-x86_64-pc-windows-msvc/lib/clang/22/lib/windows/clang_rt.asan_dynamic-x86_64.lib>
            $<$<CONFIG:RelWithDebInfo>:E:/clang+llvm-22.1.8-x86_64-pc-windows-msvc/clang+llvm-22.1.8-x86_64-pc-windows-msvc/lib/clang/22/lib/windows/clang_rt.asan_dynamic_runtime_thunk-x86_64.lib>
    )
endif()
#get_filename_component(MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)

#if (WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
#    set(DEBUG_SUFFIX)
#    if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
#        set(DEBUG_SUFFIX "d")
#    endif ()
#    set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
#    if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
#        set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
#        if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
#            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
#        endif ()
#    endif ()
#    if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
#        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#                COMMAND ${CMAKE_COMMAND} -E make_directory
#                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
#        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#                COMMAND ${CMAKE_COMMAND} -E copy
#                "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
#                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
#    endif ()
#    foreach (QT_LIB Core Gui Widgets Svg)
#        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#                COMMAND ${CMAKE_COMMAND} -E copy
#                "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}${DEBUG_SUFFIX}.dll"
#                "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
#    endforeach (QT_LIB)
#endif ()
