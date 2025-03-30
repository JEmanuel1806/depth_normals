cmake_minimum_required(VERSION 3.19)
project(depth_normals)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB_RECURSE SRC_FILES
    src/*.cpp
    src/*.h
)

add_executable(${PROJECT_NAME} ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    includes
    includes/GL
    includes/glm
    glew/glew-2.2.0/include
    glfw
)

link_directories(
    ${CMAKE_SOURCE_DIR}/lib/glew/glew-2.2.0/lib/x64
    ${CMAKE_SOURCE_DIR}/lib/glfw/x64
)

target_link_libraries(${PROJECT_NAME}
    glfw3.lib
    glew32s.lib
    opengl32.lib
)

add_definitions(-DGLEW_STATIC)
