cmake_minimum_required(VERSION 3.21)

project(Terra
    LANGUAGES CXX
)

set(PROJECTDIR "${CMAKE_CURRENT_SOURCE_DIR}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
else()
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/")
endif()

file(GLOB_RECURSE SRC src/*.cpp includes/*.hpp templates/*.hpp exports/*.hpp)

if(PLATFORM STREQUAL "WINDOWS")
    file(GLOB_RECURSE SRCOPT Win32/*.hpp Win32/*.cpp)
endif()

add_library(${PROJECT_NAME} SHARED
    ${SRC} ${SRCOPT} ${IMGUI}
)

find_package(Vulkan REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Vulkan::Vulkan
)

target_include_directories(${PROJECT_NAME} PRIVATE includes/ includes/VK/ includes/Exceptions/ templates/ exports/ DirectXMath/Inc/ DirectXMath/Extensions/)

target_compile_definitions(${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:_DEBUG>" "$<$<CONFIG:RELEASE>:NDEBUG>" BUILD_TERRA)

if(PLATFORM STREQUAL "WINDOWS")
    target_include_directories(${PROJECT_NAME} PRIVATE Win32/includes/)
    target_compile_definitions(${PROJECT_NAME} PRIVATE TERRA_WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        dxgi.lib
    )
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /fp:fast /MP /EHa /Ot /W4)
endif()

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
