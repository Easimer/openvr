cmake_minimum_required(VERSION 3.0.2)

project(openvr_samples)

# -----------------------------------------------------------------------------
## SYSTEM PROPERTIES ##

# If not set, determines the running platform architecture.
if(NOT PLATFORM)
  if(CMAKE_SIZEOF_VOID_P MATCHES 8)
    set(PLATFORM 64)
  else()
    set(PLATFORM 32)
  endif()
endif()
message(STATUS "Compilation set for ${PLATFORM}bits architectures.")

set(SDL_REQUIRED_LIBRARIES SDL2)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  add_definitions(-DLINUX -DPOSIX)
  set(ARCH_TARGET linux64)

  if(${PLATFORM} MATCHES 32)
    message(WARNING "OpenVR x86 binaries not provided on GNU/Linux.")
  endif()
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(CMAKE_MACOSX_RPATH 0)
  add_definitions(-DOSX -DPOSIX)
  set(ARCH_TARGET osx32)

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(SDL_REQUIRED_LIBRARIES ${SDL_REQUIRED_LIBRARIES} SDL2main)
  add_definitions(-D_WIN32)
  set(ARCH_TARGET win${PLATFORM})

  # Binaries path for thirdparties are not generics so we try to guess their suffixes.
  set(WINDOWS_PATH_SUFFIXES win${PLATFORM} Win${PLATFORM} x${PLATFORM})

  if(${PLATFORM} MATCHES 64)
    message(WARNING "SDL x64 runtime binaries not provided on Windows.")
  endif()
endif()

# -----------------------------------------------------------------------------
## PATHS ##

set(THIRDPARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty)
set(SHARED_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shared)

# Check that the steamVR SDK is installed 
# (needed to prevent a segfault in OpenVR).
if(CMAKE_HOST_UNIX)
  find_file(OPENVRPATHS openvrpaths.vrpath PATHS $ENV{HOME}/.config/openvr "$ENV{HOME}/Library/Application Support/OpenVR/.openvr")
  if(${OPENVRPATHS} MATCHES OPENVRPATHS-NOTFOUND)
    message(FATAL_ERROR "${OPENVRPATHS} Please install SteamVR SDK to continue..")
  endif()
endif()

# Default output directory.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/${ARCH_TARGET})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/${ARCH_TARGET})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/${ARCH_TARGET})

# Force output directory destination, especially for MSVC (@so7747857).
function(setTargetOutputDirectory target)
  foreach(type RUNTIME LIBRARY ARCHIVE)
    set_target_properties(${target} PROPERTIES
      ${type}_OUTPUT_DIRECTORY         ${CMAKE_${type}_OUTPUT_DIRECTORY}
      ${type}_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_${type}_OUTPUT_DIRECTORY}
      ${type}_OUTPUT_DIRECTORY_RELEASE ${CMAKE_${type}_OUTPUT_DIRECTORY}
    )
  endforeach()
endfunction()

# -----------------------------------------------------------------------------
## COMPILER DETECTION ##
if(   (${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
   OR (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang"))
  # Better to use the prebuilt GNU preprocessor define __GNUC__,
  # kept for legacy reason with the sample code.
  add_definitions(-DGNUC)

  set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} -std=c++11 -include ${SHARED_SRC_DIR}/compat.h")
  set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -pedantic -g")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

  # Handles x86 compilation support on x64 arch.
  if(${PLATFORM} MATCHES 32)
    set(CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS} -m32")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
  endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} /W2 /DEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MP /INCREMENTAL:NO")
else()
  message(FATAL_ERROR "Unsupported compiler '${CMAKE_CXX_COMPILER_ID}'")
endif()

# -----------------------------------------------------------------------------
## LIBRARIES ##

## OpenVR API path
find_library(OPENVR_LIBRARIES
  NAMES
    openvr_api
  PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}/../bin
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib
  PATH_SUFFIXES
    osx32
    linux64
    ${WINDOWS_PATH_SUFFIXES}
  NO_DEFAULT_PATH
  NO_CMAKE_FIND_ROOT_PATH
)
set(OPENVR_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../headers)


# -----------------------------------------------------------------------------
## SHARED SOURCES ##

file(GLOB SHARED_SRC_FILES
  ${SHARED_SRC_DIR}/*.cpp
  ${SHARED_SRC_DIR}/*.h
)

include_directories(
  .
  ${OPENVR_INCLUDE_DIR}
)

if(NOT (${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
  include_directories(${VULKAN_INCLUDE_DIR})
endif()

# -----------------------------------------------------------------------------
## SUBDIRECTORIES ##

add_subdirectory(${THIRDPARTY_DIR}/steam_controller)
add_subdirectory(${THIRDPARTY_DIR}/lua-5.3.5)
add_subdirectory(driver_easimer)

# -----------------------------------------------------------------------------
