cmake_minimum_required(VERSION 3.5.0)
project(MicaListener VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)

pkg_search_module(AVAHI_CLIENT REQUIRED avahi-client)

add_executable(MicaListener main.cpp servicediscovery.cpp)

target_link_libraries(MicaListener PUBLIC
    ${AVAHI_CLIENT_LIBRARIES}
)
