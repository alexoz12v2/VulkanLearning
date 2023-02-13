cmake_minimum_required(VERSION 3.24.3)
project(VulkanLearning)
add_executable(VulkanLearning ${PROJECT_SOURCE_DIR}/src/main.cpp)

target_compile_features(VulkanLearning PUBLIC cxx_std_20)

message("Hello world!")

# ---required setup for GLFW--- #
# turn off docs, tests, examples compilation
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# ---setup for vulkan related repositories--- #
find_package(Vulkan REQUIRED)

target_include_directories(VulkanLearning PUBLIC "${PROJECT_SOURCE_DIR}/dependencies/glfw/include"
					  PUBLIC "${PROJECT_SOURCE_DIR}/dependencies/Eigen")
add_subdirectory(
	${PROJECT_SOURCE_DIR}/dependencies/glfw/
)
add_subdirectory(
	${PROJECT_SOURCE_DIR}/dependencies/Eigen
)

if (MSVC)
	target_compile_options(VulkanLearning PRIVATE /W4 /WX)
else()
	target_compile_options(VulkanLearning PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

target_sources(VulkanLearning PUBLIC "./src/main.cpp")
target_link_libraries(VulkanLearning glfw Vulkan::Vulkan)

# copy shaders files in the build directory
add_custom_command(
	TARGET VulkanLearning POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/shaders ${CMAKE_CURRENT_BINARY_DIR}/shaders
)
