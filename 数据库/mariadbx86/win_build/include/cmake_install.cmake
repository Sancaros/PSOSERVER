# Install script for directory: D:/PSOBB/mariadb-connector-c-3.2.0-src/include

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/mariadb-connector-c")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xDevelopmentx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/mariadb" TYPE FILE FILES
    "D:/PSOBB/mariadb-connector-c-3.2.0-src/include/mariadb_com.h"
    "D:/PSOBB/mariadb-connector-c-3.2.0-src/include/mysql.h"
    "D:/PSOBB/mariadb-connector-c-3.2.0-src/include/mariadb_stmt.h"
    "D:/PSOBB/mariadb-connector-c-3.2.0-src/include/ma_pvio.h"
    "D:/PSOBB/mariadb-connector-c-3.2.0-src/include/ma_tls.h"
    "D:/PSOBB/mariadb-connector-c-3.2.0-src/win_build/include/mariadb_version.h"
    "D:/PSOBB/mariadb-connector-c-3.2.0-src/include/ma_list.h"
    "D:/PSOBB/mariadb-connector-c-3.2.0-src/include/errmsg.h"
    "D:/PSOBB/mariadb-connector-c-3.2.0-src/include/mariadb_dyncol.h"
    "D:/PSOBB/mariadb-connector-c-3.2.0-src/include/mariadb_ctype.h"
    "D:/PSOBB/mariadb-connector-c-3.2.0-src/include/mariadb_rpl.h"
    "D:/PSOBB/mariadb-connector-c-3.2.0-src/include/mysqld_error.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xDevelopmentx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/mariadb/mysql" TYPE FILE FILES
    "D:/PSOBB/mariadb-connector-c-3.2.0-src/include/mysql/client_plugin.h"
    "D:/PSOBB/mariadb-connector-c-3.2.0-src/include/mysql/plugin_auth_common.h"
    "D:/PSOBB/mariadb-connector-c-3.2.0-src/include/mysql/plugin_auth.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xDevelopmentx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/mariadb/mariadb" TYPE FILE FILES "D:/PSOBB/mariadb-connector-c-3.2.0-src/include/mariadb/ma_io.h")
endif()

