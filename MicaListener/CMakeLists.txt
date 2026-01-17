cmake_minimum_required(VERSION 3.5.0)
project(MicaListener VERSION 0.1.0)

# Set C++ Standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Get PkgConfig
find_package(PkgConfig REQUIRED)

# Find Avahi Client
pkg_search_module(AVAHI_CLIENT REQUIRED avahi-client)

# Find OpenAL
find_package(OpenAL REQUIRED)
pkg_search_module(OPENAL REQUIRED openal)

add_executable(MicaListener main.cpp servicediscovery.cpp
        shutdownhandler.cpp)

# Include Directories
target_include_directories(MicaListener PUBLIC
    ${AVAHI_CLIENT_INCLUDE_DIRS}
    ${OPENAL_INCLUDE_DIRS}
)

# Link Libraries
target_link_libraries(MicaListener PUBLIC
    ${AVAHI_CLIENT_LIBRARIES}
    ${OPENAL_LIBRARIES}
)
