cmake_minimum_required(VERSION 3.20)
project(ISS64Recompiled LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(GENERATED_DIR ${CMAKE_SOURCE_DIR}/build)
if (NOT EXISTS ${GENERATED_DIR}/RecompiledFuncs/lookup.cpp)
    message(FATAL_ERROR "Run tools/recompile.sh first to generate the recompiled code from your ROM.")
endif()

set(RT64_STATIC TRUE)
set(RT64_SDL_WINDOW_VULKAN TRUE)
add_compile_definitions(HLSL_CPU)
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_compile_definitions(PLUME_SDL_VULKAN_ENABLED RT64_SDL_WINDOW_VULKAN)
    set(NFD_PORTAL ON CACHE BOOL "nfd: use xdg-desktop-portal instead of GTK" FORCE)
endif()

if (WIN32)
    add_compile_definitions(NOMINMAX)
    include(FetchContent)
    set(SDL2_VERSION "2.30.3")
    FetchContent_Declare(sdl2
        URL https://github.com/libsdl-org/SDL/releases/download/release-${SDL2_VERSION}/SDL2-devel-${SDL2_VERSION}-VC.zip)
    FetchContent_MakeAvailable(sdl2)
    set(SDL2_INCLUDE_DIRS "${sdl2_SOURCE_DIR}/include")
else()
    find_package(SDL2 REQUIRED)
endif()

add_subdirectory(${CMAKE_SOURCE_DIR}/lib/rt64 ${CMAKE_BINARY_DIR}/rt64)
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime)
target_include_directories(rt64 PRIVATE ${CMAKE_BINARY_DIR}/rt64/src)

set(RUNTIME_INCLUDES
    ${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime/N64Recomp/include
    ${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime/librecomp/include
    ${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime/ultramodern/include
)

# Recompiled game code, generated from the user's ROM by tools/recompile.sh.
add_library(RecompiledFuncs STATIC)
file(GLOB FUNC_SOURCES ${GENERATED_DIR}/RecompiledFuncs/*.c)
target_sources(RecompiledFuncs PRIVATE ${FUNC_SOURCES})
target_include_directories(RecompiledFuncs PRIVATE ${GENERATED_DIR}/RecompiledFuncs ${RUNTIME_INCLUDES})
if (MSVC AND NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(RecompiledFuncs PRIVATE /W0)
else()
    # Calls into the runtime (osXxx_recomp) have no prototypes in the generated code.
    target_compile_options(RecompiledFuncs PRIVATE -fno-strict-aliasing -w -Wno-implicit-function-declaration)
endif()

# DXC settings that recompui needs in this scope to build its shaders.
set(DXC_COMMON_OPTS "-I${PROJECT_SOURCE_DIR}/src")
set(DXC_DXIL_OPTS "-Wno-ignored-attributes")
set(DXC_SPV_OPTS "-spirv" "-fspv-target-env=vulkan1.0" "-fvk-use-dx-layout")
set(DXC_PS_OPTS "${DXC_COMMON_OPTS}" "-E" "PSMain" "-T ps_6_0" "-D DYNAMIC_RENDER_PARAMS")
set(DXC_VS_OPTS "${DXC_COMMON_OPTS}" "-E" "VSMain" "-T vs_6_0" "-D DYNAMIC_RENDER_PARAMS" "-fvk-invert-y")
set(DXC_CS_OPTS "${DXC_COMMON_OPTS}" "-E" "CSMain" "-T cs_6_0")
set(DXC_GS_OPTS "${DXC_COMMON_OPTS}" "-E" "GSMain" "-T gs_6_0")
set(DXC_RT_OPTS "${DXC_COMMON_OPTS}" "-D" "RT_SHADER" "-T" "lib_6_3" "-fspv-target-env=vulkan1.1spirv1.4" "-fspv-extension=SPV_KHR_ray_tracing" "-fspv-extension=SPV_EXT_descriptor_indexing")
if (WIN32)
    set(DXC "${PROJECT_SOURCE_DIR}/lib/rt64/src/contrib/dxc/bin/x64/dxc.exe")
else()
    set(DXC "LD_LIBRARY_PATH=${PROJECT_SOURCE_DIR}/lib/rt64/src/contrib/dxc/lib/x64" "${PROJECT_SOURCE_DIR}/lib/rt64/src/contrib/dxc/bin/x64/dxc-linux")
endif()

set(RECOMP_FRONTEND_N64MODERNRUNTIME_PATH ${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime)
set(RECOMP_FRONTEND_RT64_PATH ${CMAKE_SOURCE_DIR}/lib/rt64)
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/RecompFrontend)

add_executable(ISS64Recompiled
    src/main.cpp
    src/register_overlays.cpp
    src/libultra_stubs.cpp
    ${GENERATED_DIR}/RecompiledFuncs/lookup.cpp
    ${GENERATED_DIR}/rsp/aspMain.cpp
)
target_include_directories(ISS64Recompiled PRIVATE
    ${GENERATED_DIR}/RecompiledFuncs
    ${RUNTIME_INCLUDES}
    ${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime/thirdparty/sse2neon
    ${CMAKE_SOURCE_DIR}/lib/rt64/src/contrib/nativefiledialog-extended/src/include
    ${CMAKE_SOURCE_DIR}/lib/RecompFrontend/recompinput/include
    ${CMAKE_SOURCE_DIR}/lib/RecompFrontend/recompui/include
    ${SDL2_INCLUDE_DIRS}
)

if (CMAKE_SIZEOF_VOID_P EQUAL 8 AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64")
    # The RSP vector unit code needs SSE4.1.
    if (MSVC AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(ISS64Recompiled PRIVATE /arch:AVX)
    else()
        target_compile_options(ISS64Recompiled PRIVATE -march=nehalem -fno-strict-aliasing)
    endif()
endif()

target_link_libraries(ISS64Recompiled PRIVATE
    RecompiledFuncs recompui recompinput librecomp ultramodern rt64 nfd)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_package(X11 REQUIRED)
    find_package(Freetype REQUIRED)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads REQUIRED)
    # lld resolves the circular dependencies between the static libraries.
    target_link_options(ISS64Recompiled PRIVATE -fuse-ld=lld)
    target_link_libraries(ISS64Recompiled PRIVATE
        ${X11_LIBRARIES} ${X11_Xrandr_LIB} ${FREETYPE_LIBRARIES}
        -latomic ${CMAKE_DL_LIBS} Threads::Threads SDL2::SDL2)
endif()

if (WIN32)
    target_link_directories(ISS64Recompiled PRIVATE "${sdl2_SOURCE_DIR}/lib/x64")
    target_link_libraries(ISS64Recompiled PRIVATE SDL2 Winmm.lib)
    if (MSVC)
        target_link_options(ISS64Recompiled PRIVATE /OPT:NOICF)
    endif()
    add_custom_command(TARGET ISS64Recompiled POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${sdl2_SOURCE_DIR}/lib/x64/SDL2.dll"
            "${CMAKE_SOURCE_DIR}/lib/rt64/src/contrib/dxc/bin/x64/dxil.dll"
            "${CMAKE_SOURCE_DIR}/lib/rt64/src/contrib/dxc/bin/x64/dxcompiler.dll"
            $<TARGET_FILE_DIR:ISS64Recompiled>)
endif()

# recompui loads its stylesheets and fonts from assets/ next to the executable.
add_custom_command(TARGET ISS64Recompiled POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/lib/assets $<TARGET_FILE_DIR:ISS64Recompiled>/assets)
