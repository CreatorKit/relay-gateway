# Project Properties
####################
CMAKE_MINIMUM_REQUIRED (VERSION 2.6.2)
PROJECT(relay_toggle)
# This will be extended to take the version from SCM
# Version config can be simplified from 3.0+
SET(relay_control_VERSION_MAJOR 0)
SET(relay_control_VERSION_MINOR 3)
SET(relay_control_VERSION_PATCH 0)
SET(relay_control_VERSION
    ${relay_control_VERSION_MAJOR}.${relay_control_VERSION_MINOR}.${relay_control_VERSION_PATCH})

# Options
###################
SET(CMAKE_VERBOSE_MAKEFILE 1)
SET(CMAKE_BUILD_TYPE DEBUG) # Options MINSIZEREL, RELEASE, DEBUG
SET(DOCS_INTERNAL 1 CACHE BOOL "enable internal docs generation")

# Includes
##########
INCLUDE_DIRECTORIES(${STAGING_DIR}/usr/include/flow_dm ${STAGING_DIR}/usr/include/)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
IF(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
#  IF(DEFINED ENV{CASKEID_COMMON})
#    LIST(APPEND CMAKE_MODULE_PATH "$ENV{CASKEID_COMMON}/cmake")
#  ELSE()
#    MESSAGE(WARNING "Environment variable CASKEID_COMMON is not set; \nexport CASKEID_COMMON=<common path>")
#  ENDIF()
ENDIF()

# Paths
########
ADD_SUBDIRECTORY(src)
#ADD_SUBDIRECTORY(docs)
