cmake_minimum_required(VERSION 3.19)
project(fuzzy_sorter LANGUAGES CXX)

option(ENABLE_PROFILING "Enable profiling-friendly build" OFF)

if(ENABLE_PROFILING)
  add_compile_options(-g -fno-omit-frame-pointer -fno-optimize-sibling-calls)
  add_link_options(-g -fno-omit-frame-pointer)
endif()

add_library(${PROJECT_NAME} SHARED "src/simple_fuzzy_sorter.cpp")

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>)
target_compile_options(${PROJECT_NAME}
  PRIVATE
    $<$<CXX_COMPILER_ID:MSC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
        -Wunreachable-code
        -Wfloat-equal
        -Wcast-align
        -Wundef
        -Wno-shorten-64-to-32
        -Wnon-virtual-dtor
        # -fsanitize=address,undefined,leak
        -Werror > )
target_compile_definitions(${PROJECT_NAME}
  PRIVATE
    $<$<PLATFORM_ID:Windows>:_CRT_NONSTDC_NO_DEPRECATE>
    $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_DEPRECATE>
    $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS>)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    PREFIX lib)

if (UNIX AND NOT APPLE)
  target_compile_options(${PROJECT_NAME} PRIVATE -fPIC)
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_BINARY_DIR})

add_executable(fuzzy_sorter_test test/fuzzy_sorter_test.cpp)
# sanitize checks
# if (NOT MSVC)
#   target_link_options(${PROJECT_NAME} PRIVATE
#     -fsanitize=address,undefined,leak )
#   target_link_options(fuzzy_sorter_test PRIVATE
#     -fsanitize=address,undefined,leak )
# endif()



# GoogleTest einbinden
target_link_libraries(fuzzy_sorter_test
    PRIVATE
        fuzzy_sorter     # deine Library
        gtest            # GoogleTest selbst
        gtest_main       # enthält main()
        pthread          # erforderlich für gtest
)

# Tests aktivieren
enable_testing()
add_test(NAME FuzzySorterTests COMMAND fuzzy_sorter_test)
