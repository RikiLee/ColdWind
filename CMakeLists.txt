cmake_minimum_required(VERSION 3.10)
project(ClodWind VERSION 0.1.0)
set(PROJECT_NAME ClodWind)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

find_package(Vulkan REQUIRED)
find_package(glslang REQUIRED COMPONENTS
    glslang          # 主库
    SPIRV            # SPIR-V 支持
    OGLCompiler      # OpenGL 编译器
    OSDependent      # 系统相关功能
    HLSL             # HLSL 支持（可选）
)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# load model
find_package(assimp CONFIG REQUIRED)
# load video
find_package(FFmpeg REQUIRED COMPONENTS
    avcodec
    avformat
    avutil
    swscale
    swresample
)
# log
find_package(spdlog CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

file(GLOB INC_FILES include/*.h include/*.hpp)
file(GLOB SRC_FILES src/*.c src/*.cpp)

add_executable(${PROJECT_NAME} ${SRC_FILES} ${INC_FILES})

target_compile_definitions(ClodWind 
    PRIVATE
        VULKAN_HPP_NO_EXCEPTIONS
        GLSLANG_ENABLE_KHRONOS_EXTENSIONS
        GLSLANG_TARGET_SPIRV
)

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    target_compile_definitions(${PROJECT_NAME} 
        PRIVATE 
            VK_USE_PLATFORM_WIN32_KHR
            NOMINMAX
    )
endif()

target_include_directories(${PROJECT_NAME}
    PUBLIC
        include
        ${Vulkan_INCLUDE_DIRS}
        ${glfw3_INCLUDE_DIRS}
        ${glm_INCLUDE_DIRS}
        ${glslang_INCLUDE_DIRS}

        ${spdlog_INCLUDE_DIRS}

        ${assimp_INCLUDE_DIRS}
        ${FFMPEG_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} 
    PUBLIC 
        Vulkan::Vulkan
        glm::glm
        glfw
        glslang::glslang
        glslang::SPIRV
        glslang::OSDependent
        GPUOpen::VulkanMemoryAllocator

        spdlog::spdlog
        imgui::imgui

        assimp::assimp
        ${FFMPEG_LIBRARIES}
)
