# minimal cmake version
cmake_minimum_required(VERSION 3.13)

# fallback for other versions 
if(${CMAKE_VERSION} VERSION_LESS 3.18)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
    cmake_policy(VERSION 3.18)
endif()

# set the cxx compiler standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# steal the version
set(VERSION_MAJOR_REGEX "#define EXECUTABLE_VERSION_MAJOR[ \t]+\"(.+)\"")
set(VERSION_MINOR_REGEX "#define EXECUTABLE_VERSION_MINOR[ \t]+\"(.+)\"")
set(VERSION_PATCH_REGEX "#define EXECUTABLE_VERSION_PATCH[ \t]+\"(.+)\"")

file(STRINGS "${CMAKE_SOURCE_DIR}/src/egel-bot.cpp"
    VERSION_MAJOR REGEX ${VERSION_MAJOR_REGEX})
file(STRINGS "${CMAKE_SOURCE_DIR}/src/egel-bot.cpp"
    VERSION_MINOR REGEX ${VERSION_MINOR_REGEX})
file(STRINGS "${CMAKE_SOURCE_DIR}/src/egel-bot.cpp"
    VERSION_PATCH REGEX ${VERSION_PATCH_REGEX})

string(REGEX REPLACE ${VERSION_MAJOR_REGEX} "\\1" VERSION_MAJOR "${VERSION_MAJOR}")
string(REGEX REPLACE ${VERSION_MINOR_REGEX} "\\1" VERSION_MINOR "${VERSION_MINOR}")
string(REGEX REPLACE ${VERSION_PATCH_REGEX} "\\1" VERSION_PATCH "${VERSION_PATCH}")

set(PVERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

# the EgelBot IRC bot project
project(EgelBot DESCRIPTION "The EgelBot IRC bot"
        VERSION ${PVERSION} LANGUAGES CXX)

# set the cxx compiler flags
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(("${CMAKE_CXX_COMPILER_ID}" MATCHES "AppleClang"))

  message("AppleClang detected")
  set(CMAKE_CXX_FLAGS                     "-Wall -Wextra -Werror -pedantic -Wno-unused-parameter -Wno-return-type-c-linkage")
  set(CMAKE_CXX_FLAGS_DEBUG               "-g")
  set(CMAKE_CXX_FLAGS_RELEASE             "-Ofast")

else()

  message("Other compiler")
  set(CMAKE_CXX_FLAGS                     "-Wall -Wextra -Werror -pedantic -Wno-unused-parameter")
  set(CMAKE_CXX_FLAGS_DEBUG               "-g")
  set(CMAKE_CXX_FLAGS_RELEASE             "-O3")

endif()

SET(CMAKE_CXX_FLAGS_PROFILE             "-O3 -pg")
SET(CMAKE_EXE_LINKER_FLAGS_PROFILE      "-O3 -pg")
SET(CMAKE_SHARED_LINKER_FLAGS_PROFILE   "-O3 -pg")


# for the moment, glob sources and headers
file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.hpp")
file(GLOB SOURCE_LIST CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")

# check for ICU, Threads, fmt
# ICU components: data, i18n, io, le, lx, test, tu and uc.
find_package(ICU REQUIRED COMPONENTS data i18n io tu uc)
find_package(Threads REQUIRED)
find_package(fmt REQUIRED)

# check for egellib  (should be moved to a findEgel cmake once)
include( FindPackageHandleStandardArgs )

if( DEFINED ENV{EGELLIB_DIR} )
  set( EGELLIB_DIR "$ENV{EGELLIB_DIR}" )
endif()

find_path(
  EGELLIB_INCLUDE_DIR
    egel/utils.hpp
  HINTS
    ${EGELLIB_DIR}
)

find_library( EGELLIB_LIBRARY
  NAMES egel
  HINTS ${EGELLIB_DIR}
)

find_package_handle_standard_args( EGELLIB DEFAULT_MSG
  EGELLIB_INCLUDE_DIR
  EGELLIB_LIBRARY
)

if( EGELLIB_FOUND )
  set( EGELLIB_INCLUDE_DIRS ${EGELLIB_INCLUDE_DIR} )
  set( EGELLIB_LIBRARIES ${EGELLIB_LIBRARY} )

  mark_as_advanced(
    EGELLIB_LIBRARY
    EGELLIB_INCLUDE_DIR
    EGELLIB_DIR
  )
else()
  set( EGELLIB_DIR "" CACHE STRING
    "An optional hint to a directory for finding `bar`"
  )
endif()


include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories("${EGELLIB_INCLUDE_DIRS}")
include_directories("${ICU_INCLUDE_DIRS}")


# objects
add_library(objlib OBJECT ${SOURCE_LIST})
set_property(TARGET objlib PROPERTY POSITION_INDEPENDENT_CODE 1)
target_link_libraries(objlib fmt::fmt Threads::Threads ICU::uc ICU::i18n ICU::io ${EGELLIB_LIBRARIES})

# the EgelBot 
add_executable(egel-bot $<TARGET_OBJECTS:objlib>)
target_link_libraries(egel-bot ${CMAKE_DL_LIBS} ${EGELLIB_LIBRARIES} fmt::fmt Threads::Threads ICU::uc ICU::i18n ICU::io)

# compiler dependent stuff
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(egel-bot stdc++fs)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
endif()

# installation
include(GNUInstallDirs)

install(TARGETS egel-bot
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

