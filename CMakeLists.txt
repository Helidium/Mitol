cmake_minimum_required(VERSION 3.2)
project(MNS)

include_directories(./src /usr/include)
link_directories(/usr/lib)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -fno-builtin -march=native -mtune=native" )
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -luv -g3 -Wall" )

set(SOURCE_FILES main.cpp src/Http.cpp src/Http.h src/Server.cpp src/Server.h src/Process.h src/Socket.h src/Request.cpp src/Request.h src/Response.cpp src/Response.h src/Variables.h src/Socket.cpp)
add_executable(MNS ${SOURCE_FILES})
target_link_libraries(MNS uv)