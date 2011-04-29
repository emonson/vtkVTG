CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

#
# Set the project name.
#

PROJECT (VTKVTG)

#
# Load CMake commands that you probably should not modify.
#

INCLUDE (${VTKVTG_SOURCE_DIR}/CMakeOptions.cmake)

#
# Here is where you can list the sub-directories holding your local
# classes. Sorting classes by 'package' type like VTK does (Common,
# Rendering, Filtering, Imaging, IO, etc.) is a good thing and prevents
# numerous dependencies problems.
#

SUBDIRS (
  Common
  Charts
  Filtering
  Graphics
  QtGUI
  QtGUIdebug
  Hybrid
  Infovis
  Rendering
  Utilities
  Widgets
  Wrapping/Python
)

#
# You can put your include path(s) here
#

INCLUDE_DIRECTORIES (${VTKVTG_SOURCE_DIR}/Common)
INCLUDE_DIRECTORIES (${VTKVTG_SOURCE_DIR}/Charts)
INCLUDE_DIRECTORIES (${VTKVTG_SOURCE_DIR}/Filtering)
INCLUDE_DIRECTORIES (${VTKVTG_SOURCE_DIR}/Graphics)
INCLUDE_DIRECTORIES (${VTKVTG_SOURCE_DIR}/QtGUI)
INCLUDE_DIRECTORIES (${VTKVTG_SOURCE_DIR}/QtGUIdebug)
INCLUDE_DIRECTORIES (${VTKVTG_SOURCE_DIR}/Hybrid)
INCLUDE_DIRECTORIES (${VTKVTG_SOURCE_DIR}/Infovis)
INCLUDE_DIRECTORIES (${VTKVTG_SOURCE_DIR}/Rendering)
INCLUDE_DIRECTORIES (${VTKVTG_SOURCE_DIR}/Widgets)

#
# Build examples too ?
#

OPTION(BUILD_EXAMPLES "Build examples." ON)
IF (BUILD_EXAMPLES)
  SUBDIRS(Examples)
ENDIF (BUILD_EXAMPLES)

#
# Common configuration settings
#
# Do not worry about this one.
#

CONFIGURE_FILE(
  ${VTKVTG_SOURCE_DIR}/vtkvtgConfigure.h.in
  ${VTKVTG_BINARY_DIR}/vtkvtgConfigure.h
)

INCLUDE_DIRECTORIES(${VTKVTG_BINARY_DIR})
if(APPLE)
  INCLUDE_DIRECTORIES(/System/Library/Frameworks/Python.framework/Headers)
endif(APPLE)

if(UNIX AND NOT APPLE)
  INCLUDE_DIRECTORIES(/usr/include/python2.6)
endif(UNIX AND NOT APPLE)

# INCLUDE_DIRECTORIES(${VTKVTG_PYTHON_INCLUDE_DIRS})
