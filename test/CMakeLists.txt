cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE SRC src/*.cc)

if(MSVC)
    file(GLOB_RECURSE SRCOPT Win32/*.hpp Win32/*.cpp)
endif()

add_executable(TerraTest
    ${SRC} ${SRCOPT}
)

if(MSVC)
    target_include_directories(TerraTest PRIVATE ${TERRA_PRIVATE_INCLUDES} Win32/)
else()
    target_include_directories(TerraTest PRIVATE ${TERRA_PRIVATE_INCLUDES})
endif()

unset(TERRA_PRIVATE_INCLUDES)

if(MSVC)
    target_compile_definitions(TerraTest PRIVATE TERRA_WIN32)
    target_compile_options(TerraTest PRIVATE /fp:fast /MP /Ot /W4 /Gy)
endif()

include(FetchContent)

set(GOOGLE_TEST_COMMIT_ID 12a5852e451baabc79c63a86c634912c563d57bc CACHE STRING "Supply the latest commit ID from the GitHub repository.")

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/${GOOGLE_TEST_COMMIT_ID}.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

if(MSVC)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

FetchContent_MakeAvailable(googletest)

find_package(Vulkan REQUIRED)

target_link_libraries(TerraTest PRIVATE
    GTest::gtest_main TerraLib Vulkan::Vulkan razer::callisto razer::DxMath razer::venus
)

include(GoogleTest)

gtest_discover_tests(TerraTest)
