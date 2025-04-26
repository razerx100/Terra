cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE SRC src/*.cpp src/*.ixx includes/*.hpp templates/*.hpp exports/*.hpp)

if(MSVC)
    file(GLOB_RECURSE SRCOPT Win32/*.hpp Win32/*.cpp)
endif()

add_library(TerraLib STATIC ${SRC} ${SRCOPT})

find_package(Vulkan REQUIRED)

target_link_libraries(TerraLib PUBLIC
    Vulkan::Vulkan
)

set(TERRA_PRIVATE_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/templates/)

set(TERRA_PRIVATE_INCLUDES ${TERRA_PRIVATE_INCLUDES} PARENT_SCOPE)

target_include_directories(TerraLib PRIVATE ${TERRA_PRIVATE_INCLUDES})

if(MSVC)
    target_include_directories(TerraLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/exports/ ${CMAKE_CURRENT_SOURCE_DIR}/includes/ ${CMAKE_CURRENT_SOURCE_DIR}/includes/VK/ ${CMAKE_CURRENT_SOURCE_DIR}/Win32/includes/)
else()
    target_include_directories(TerraLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/exports/ ${CMAKE_CURRENT_SOURCE_DIR}/includes/ ${CMAKE_CURRENT_SOURCE_DIR}/includes/VK/)
endif()

if(MSVC)
    target_compile_definitions(TerraLib PUBLIC TERRA_WIN32)
    target_link_libraries(TerraLib PRIVATE
        dxgi.lib
    )
endif()

if(MSVC)
    target_compile_options(TerraLib PRIVATE /fp:fast /MP /Ot /W4 /Gy /std:c++latest /Zc:__cplusplus)
endif()

include(FetchContent)

FetchContent_Declare(
    Callisto
    GIT_REPOSITORY https://github.com/razerx100/Callisto.git
    GIT_TAG master
)

FetchContent_MakeAvailable(Callisto)

FetchContent_Declare(
    Venus
    GIT_REPOSITORY https://github.com/razerx100/Venus.git
    GIT_TAG master
)

FetchContent_MakeAvailable(Venus)

FetchContent_Declare(
    DirectXMath
    GIT_REPOSITORY https://github.com/razerx100/DirectXMath.git
    GIT_TAG master
)

FetchContent_MakeAvailable(DirectXMath)

target_link_libraries(TerraLib PRIVATE razer::callisto razer::DxMath razer::venus)
