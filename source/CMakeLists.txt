################################################################
cmake_minimum_required(VERSION 2.6)
################################################################
project(libict-boost)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_ICT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../libict/)
set(CMAKE_ICT_SOURCE ${CMAKE_ICT_DIR}/source/)
set(CMAKE_ICT_BIN ${CMAKE_ICT_DIR}/build/)
add_subdirectory(${CMAKE_ICT_SOURCE} ${CMAKE_ICT_BIN})

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME ON)
find_package(Boost 1.45.0 COMPONENTS system)
include_directories(${Boost_INCLUDE_DIRS})

set(CMAKE_SOURCE_FILES
  asio.cpp
  resolver.cpp
  connection-string.cpp
  connection-http.cpp
  connection.cpp
  client.cpp
  server.cpp
)

add_library(ict-boost-static STATIC ${CMAKE_SOURCE_FILES})
target_link_libraries(ict-boost-static pthread ${Boost_LIBRARIES} ict-static)
set_target_properties(ict-boost-static  PROPERTIES OUTPUT_NAME ict-boost)

#add_library(ict-boost-shared SHARED ${CMAKE_SOURCE_FILES})
#target_link_libraries(ict-boost-shared pthread ict-static ${Boost_LIBRARIES})
#set_target_properties(ict-boost-shared  PROPERTIES OUTPUT_NAME ict-boost)

add_executable(libict-boost-test test.cpp ${CMAKE_SOURCE_FILES})
target_link_libraries(libict-boost-test pthread ${Boost_LIBRARIES} ict-static)
target_compile_definitions(libict-boost-test PUBLIC -DENABLE_TESTING)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../.git)
  find_package(Git)
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} describe
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      OUTPUT_VARIABLE "GIT_VERSION"
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    message( STATUS "Git version: ${GIT_VERSION}" )
  else(GIT_FOUND)
    set(GIT_VERSION unknown)
  endif(GIT_FOUND)
else(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../.git)
 set(GIT_VERSION unknown)
endif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../.git)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/git_version.h.in ${CMAKE_CURRENT_SOURCE_DIR}/git_version.h @ONLY)

install(TARGETS ict-boost-static DESTINATION lib COMPONENT libraries)
install(FILES 
  asio.hpp
  resolver.hpp
  connection.hpp
  client.hpp
  server.hpp
  all.hpp
DESTINATION include/libict-boost COMPONENT headers)
################################################################
#enable_testing()

#add_test(NAME ict-utf8-tc1 COMMAND libict-test ict utf8 tc1)
#add_test(NAME ict-utf8-tc2 COMMAND libict-test ict utf8 tc2)
################################################################
#set(CPACK_GENERATOR "TZ;TGZ;STGZ;DEB;RPM") # Linux
#set(CPACK_GENERATOR "TZ;TGZ;STGZ")         # MacOS

set(CPACK_PACKAGE_NAME "libict-boost")
set(CPACK_PACKAGE_VENDOR "ict-project.pl")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Mariusz Ornowski <mariusz.ornowski@ict-project.pl>")
set(CPACK_RPM_PACKAGE_LICENSE "BSD 3-clause")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "libict-boost - C++11 wrapper for libboost.")
set(CPACK_PACKAGE_VERSION ${GIT_VERSION})

set(CPACK_COMPONENTS_ALL libraries headers)

set(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "Libraries")
set(CPACK_COMPONENT_LIBRARIES_DESCRIPTION "Libraries for libict-boost - C++11 wrapper for libboost.")

set(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "C++ Headers")
set(CPACK_COMPONENT_HEADERS_DESCRIPTION  "C++ Headers for libict-boost - C++11 wrapper for libboost.")
set(CPACK_COMPONENT_HEADERS_DEPENDS libraries)

include(CPack)
################################################################
