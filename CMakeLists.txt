cmake_minimum_required(VERSION 3.16)
project(zmq_pub_sub)

set(CMAKE_CXX_STANDARD 20)

find_package(cppzmq)

add_executable(${PROJECT_NAME}
        main.cpp
        messenger.cpp
        pub_sub_interface.cpp
)
# Add the path to the library directory
link_directories(/usr/local/lib64)
target_link_libraries(${PROJECT_NAME} cppzmq)
