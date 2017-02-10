cmake_minimum_required(VERSION 3.6)
project(ServerWithPyhsX)

set(CMAKE_CXX_STANDARD 11)

link_directories(lib)

set(CMAKE_CXX_STANDARD 11)
add_compile_options(-std=c++11)
find_package(Protobuf REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
include_directories(include ${OPENGL_INCLUDE_DIRS}  ${GLUT_INCLUDE_DIRS} )
FILE(GLOB protofiles "src/*.proto")
message(STATUS ${protofiles})
PROTOBUF_GENERATE_CPP(PROTOSRCS PROTOHDRS ${protofiles})
message(STATUS ${PROTOSRCS})
add_library(proto STATIC ${PROTOSRCS} ${PROTOHDRS})
target_link_libraries(proto protobuf)
target_include_directories(proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
aux_source_directory(src DIR_SRCS)
add_executable(ServerWithPyhsX ${DIR_SRCS})
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS "-Wall")
target_link_libraries(ServerWithPyhsX
        ${OPENGL_LIBRARIES}
        ${GLUT_LIBRARY}
        libLowLevelClothDEBUG.a
        libLowLevelDEBUG.a
        libPhysX3CharacterKinematicDEBUG.a
        libPhysX3CommonDEBUG.a
        libPhysX3CookingDEBUG.a
        libPhysX3DEBUG.a
        libPhysX3ExtensionsDEBUG.a
        libPhysX3VehicleDEBUG.a
        libPhysXProfileSDKDEBUG.a
        libPhysXVisualDebuggerSDKDEBUG.a
        libPvdRuntimeDEBUG.a
        libPxTaskDEBUG.a
        libSceneQueryDEBUG.a
        libSimulationControllerDEBUG.a proto protobuf)