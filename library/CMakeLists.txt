cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE SRC src/*.cpp src/*.ixx includes/*.hpp templates/*.hpp exports/*.hpp)

if(MSVC)
    file(GLOB_RECURSE SRCOPT Win32/*.hpp Win32/*.cpp)
endif()

add_library(TerraLib SHARED
    ${SRC} ${SRCOPT}
)

find_package(Vulkan REQUIRED)

target_link_libraries(TerraLib PRIVATE
    Vulkan::Vulkan
)

if(MSVC)
    set(TERRA_PRIVATE_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/includes/ ${CMAKE_CURRENT_SOURCE_DIR}/includes/VK/ ${CMAKE_CURRENT_SOURCE_DIR}/includes/Exceptions/ ${CMAKE_CURRENT_SOURCE_DIR}/templates/ ${CMAKE_CURRENT_SOURCE_DIR}/Win32/includes/)
else()
    set(TERRA_PRIVATE_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/includes/ ${CMAKE_CURRENT_SOURCE_DIR}/includes/VK/ ${CMAKE_CURRENT_SOURCE_DIR}/includes/Exceptions/ ${CMAKE_CURRENT_SOURCE_DIR}/templates/)
endif()

set(TERRA_PRIVATE_INCLUDES ${TERRA_PRIVATE_INCLUDES} PARENT_SCOPE)

target_include_directories(TerraLib PRIVATE ${TERRA_PRIVATE_INCLUDES})

target_include_directories(TerraLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/exports/)

target_compile_definitions(TerraLib PRIVATE BUILD_TERRA)

if(MSVC)
    target_compile_definitions(TerraLib PRIVATE TERRA_WIN32)
    target_link_libraries(TerraLib PRIVATE
        dxgi.lib
    )
endif()

if(MSVC)
    target_compile_options(TerraLib PRIVATE /fp:fast /MP /Ot /W4 /Gy)
endif()

include(FetchContent)

FetchContent_Declare(
    Callisto
    GIT_REPOSITORY https://github.com/razerx100/Callisto.git
    GIT_TAG master
)

FetchContent_MakeAvailable(Callisto)

FetchContent_Declare(
    DirectXMath
    GIT_REPOSITORY https://github.com/razerx100/DirectXMath.git
    GIT_TAG master
)

FetchContent_MakeAvailable(DirectXMath)

target_link_libraries(TerraLib PRIVATE razer::callisto razer::DxMath)