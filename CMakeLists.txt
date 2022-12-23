cmake_minimum_required(VERSION 3.24.3)
project(VulkanLearning)
add_executable(VulkanLearning ${PROJECT_SOURCE_DIR}/src/main.cpp)

target_compile_features(VulkanLearning PUBLIC cxx_std_20)
# set some basic compiler options
if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4) #/WX)
else()
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall)# -Wextra -pedantic -Werror) writing Werror like this will treat warnings coming from dependencies as errors, we want to use the CMAKE_COMPILE_WARNING_AS_ERROR variable, TODO later
endif()

message("Hello world!")

# ---required setup for GLFW--- #

# turn off docs, tests, examples compilation
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# force GLFW to use wayland instead of X in linux
if(UNIX AND NOT APPLE AND NOT CYGWIN)
#	set(GLFW_BUILD_WAYLAND 1)
	set(GLFW_BUILD_X11 1)
#	set(GLFW_USE_WAYLAND 1)
endif()

# ---setup for vulkan related repositories--- #
# setting up spirv headers for vulkan validation layers
set(SPIRV_HEADERS_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/dependencies/spirv-headers/include)
# add dependencies to project structure hierarchy

# vulkan loaders needs to know where to locate vulkan-headers
set(VULKAN_HEADERS_INSTALL_DIR "${PROJECT_SOURCE_DIR}/dependencies/vulkan-headers")
set(CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/")

# vulkan validation layers uses a faster, third-party implementation of unordered_set and unordered_map
set(robin_hood_DIR "${PROJECT_SOURCE_DIR}/dependencies/robin-hood-hashing")
set(RH_STANDALONE_PROJECT OFF)
# vulkan validation layer tests require more, but I won't use them

target_include_directories(VulkanLearning PUBLIC "${PROJECT_SOURCE_DIR}/dependencies/glfw/include"
						PUBLIC "${PROJECT_SOURCE_DIR}/dependencies/vulkan-headers/include"
						PUBLIC "${PROJECT_SOURCE_DIR}/dependencies/robin-hood-hashing/src/include"
				    	PUBLIC "${PROJECT_SOURCE_DIR}/dependencies/spirv-headers/include"
						PUBLIC "${PROJECT_SOURCE_DIR}/dependencies/spirv-tools/include")
add_subdirectory(
	${PROJECT_SOURCE_DIR}/dependencies/glfw/
)
add_subdirectory(
	${PROJECT_SOURCE_DIR}/dependencies/vulkan-headers
)
add_subdirectory(
	${PROJECT_SOURCE_DIR}/dependencies/robin-hood-hashing
)
# execute_process("python3 update_deps.py" WORKING_DIRECTORY="${PROJECT_SOURCE_DIR}/dependencies/vulkan-validationlayers/scripts")
add_subdirectory(
	${PROJECT_SOURCE_DIR}/dependencies/vulkan-validationlayers
)
add_subdirectory(
	${PROJECT_SOURCE_DIR}/dependencies/spirv-headers
)
add_subdirectory(
	${PROJECT_SOURCE_DIR}/dependencies/spirv-tools
)

include(FetchContent)
FetchContent_Declare(
	Vulkan-Loader
	GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Loader.git
	GIT_TAG 09afdf64210b11ec653a1001e013b5af4fce8559 # sdk-1.3.224.0
)
FetchContent_MakeAvailable(Vulkan-Loader)

target_link_libraries(VulkanLearning glfw Vulkan::Vulkan)

