cmake_minimum_required(VERSION 3.5.1)
include(FetchContent)
project("das_link_creation_agent")

file(GLOB_RECURSE sources ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc)
file(GLOB_RECURSE includes ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
FetchContent_Declare(
  DASNode
  URL https://github.com/singnet/das-node/archive/refs/tags/0.1.5-rc.tar.gz
#   GIT_REPOSITORY https://github.com/singnet/das-node
#   GIT_TAG master # Replace 'main' with a specific tag, branch, or commit hash
)


FetchContent_MakeAvailable(DASNode)


add_executable("das_link_creation_agent" ${sources} ${includes})

target_link_libraries("das_link_creation_agent" PRIVATE DASNodeTarget)
