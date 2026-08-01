cmake_minimum_required(VERSION 3.10)
project(MicaListener VERSION 1.0.0)

# Set C++ Standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set strict compiler warnings
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wall -Wextra -Werror -pedantic)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W4 /WX)
endif()

# Get PkgConfig
find_package(PkgConfig REQUIRED)

# Find Avahi Client
pkg_search_module(AVAHI_CLIENT REQUIRED avahi-client)

# Find OpenAL
find_package(OpenAL REQUIRED)
pkg_search_module(OPENAL REQUIRED openal)

add_executable(MicaListener MicaListenerService/main.cpp
        MicaListenerService/Network/servicediscovery.cpp 
        MicaListenerService/Network/deviceregistry.cpp
        MicaListenerService/Audio/audioplayer.cpp 
        MicaListenerService/Network/socketclient.cpp
        MicaListenerService/Lifecycle/launcher.hpp
)

add_executable(MicaPairingService
        MicaPairingService/main.cpp
        MicaPairingService/DeviceSelection/deviceselectionhandler.cpp
)

# Set O2 optimization for the release
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# Set LTO if it's supported
include(CheckIPOSupported)
check_ipo_supported(RESULT lto_supported OUTPUT lto_error)
if(lto_supported)
    set_property(TARGET MicaListener PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

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
