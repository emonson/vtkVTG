# this file is used directly from VTK/CMakeLists.txt, not from VTK/Wrapping/CMakeLists.txt

IF(UNIX)
  SET(PYTHON_MODULE_PREFIX "lib")
ELSE(UNIX)
  SET(PYTHON_MODULE_PREFIX "")
ENDIF(UNIX)


# Handle out-of-source builds correctly.
#
#  1. Create a list of Python files to be installed/copied.
#  2. Copy them to VTK_BINARY_DIR if it is different from VTK_SOURCE_DIR.
#  3. Use Python's compileall to compile the copied files.
#
# *** Step 1 has to be done carefully to avoid missing out files ***

# Setting these variables for non-VTK build
# SET(VTKVTG_BINARY_DIR ${CMAKE_BINARY_DIR})
# SET(VTKVTG_SOURCE_DIR ${CMAKE_SOURCE_DIR})
SET(VTKVTG_LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
SET(VTKVTG_EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
if(APPLE)
  SET(PYTHON_EXECUTABLE "/System/Library/Frameworks/Python.framework/Versions/2.6/bin/python2.6")
endif(APPLE)
if(UNIX AND NOT APPLE)
  SET(PYTHON_EXECUTABLE "/usr/bin/python2.6")
endif(UNIX AND NOT APPLE)

IF(1)
# Make the necessary directories.
    FILE(MAKE_DIRECTORY  ${VTKVTG_BINARY_DIR}/Wrapping/Python/vtkvtg
                         # ${VTKVTG_BINARY_DIR}/Wrapping/Python/vtkvtg/qt4 
                         )

# Now create a list of Python files.

# Wrapping/Python/vtk/*.py
  SET(VTKVTG_PYTHON_FILES 
    vtkvtg/__init__
    vtkvtg/charts
    vtkvtg/common
    vtkvtg/filtering
    vtkvtg/graphics
    vtkvtg/hybrid
    vtkvtg/infovis
    vtkvtg/rendering
    vtkvtg/widgets
    )

# PyQt v4 related files
#  SET(VTKVTG_PYTHON_FILES 
#    ${VTKVTG_PYTHON_FILES}
#    vtkvtg/qt4/__init__
#    vtkvtg/qt4/QVTKRenderWindowInteractor
#    )

# Done listing of files.

# Now copy these files if necessary.
  SET(VTKVTG_PYTHON_SOURCE_FILES)
  SET(VTKVTG_PYTHON_OUTPUT_FILES)
  FOREACH(file ${VTKVTG_PYTHON_FILES})
    SET(src "${VTKVTG_BINARY_DIR}/Wrapping/Python/${file}.py")
    SET(VTKVTG_PYTHON_SOURCE_FILES ${VTKVTG_PYTHON_SOURCE_FILES} ${src})
  ENDFOREACH(file)

  IF (NOT "${VTKVTG_BINARY_DIR}" MATCHES "^${VTKVTG_SOURCE_DIR}$")
    FOREACH(file ${VTKVTG_PYTHON_FILES})
      SET(src "${VTKVTG_SOURCE_DIR}/Wrapping/Python/${file}.py")
      SET(tgt "${VTKVTG_BINARY_DIR}/Wrapping/Python/${file}.py")
      ADD_CUSTOM_COMMAND(DEPENDS ${src}
                         COMMAND ${CMAKE_COMMAND} -E copy ${src} ${tgt}
                         OUTPUT ${tgt}
                         COMMENT "source copy to ${tgt}")
      # SET(tgt "${VTKVTG_BINARY_DIR}/Wrapping/Python/vtkvtg")
      # FILE(COPY ${src} DESTINATION ${tgt})
    ENDFOREACH(file)
  ENDIF (NOT "${VTKVTG_BINARY_DIR}" MATCHES "^${VTKVTG_SOURCE_DIR}$")

  # handle the different configuration types (each will have identical files)
  SET(VTKVTG_PYTHON_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES})
  IF(VTKVTG_PYTHON_CONFIGURATION_TYPES)
    FOREACH(config ${VTKVTG_PYTHON_CONFIGURATION_TYPES})
      FOREACH(file ${VTKVTG_PYTHON_FILES})
        SET(src "${VTKVTG_SOURCE_DIR}/Wrapping/Python/${file}.py")
        SET(tgt "${VTKVTG_BINARY_DIR}/Wrapping/Python/${config}/${file}.py")
        ADD_CUSTOM_COMMAND(DEPENDS ${src}
                           COMMAND ${CMAKE_COMMAND}
                           ARGS -E copy ${src} ${tgt}
                           OUTPUT ${tgt}
                           COMMENT "source copy")
        # SET(tgt "${VTKVTG_BINARY_DIR}/Wrapping/Python/${config}/vtkvtg")
        # FILE(COPY ${src} DESTINATION ${tgt})
      ENDFOREACH(file)
    ENDFOREACH(config)
  ENDIF(VTKVTG_PYTHON_CONFIGURATION_TYPES)

  # Byte compile the Python files.
  CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/compile_all_vtk.py.in
                 ${CMAKE_CURRENT_BINARY_DIR}/compile_all_vtk.py
                 @ONLY IMMEDIATE)
  ADD_CUSTOM_COMMAND(
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/compile_all_vtk.py
    DEPENDS ${VTKVTG_PYTHON_SOURCE_FILES} ${CMAKE_CURRENT_BINARY_DIR}/compile_all_vtk.py
    OUTPUT "${VTKVTG_BINARY_DIR}/Wrapping/Python/vtk_compile_complete"
    COMMENT "byte-compiling python sources"
    )

  # Create a target on which custom commands can depend.  When this
  # is up-to-date, it is safe to run VTK python code.  Therefore
  # this should also depend on the vtkpython executable.
  ADD_CUSTOM_TARGET(vtgpython_compiles ALL echo "...custom_target..."
    DEPENDS "${VTKVTG_BINARY_DIR}/Wrapping/Python/vtk_compile_complete")

  # Add a rule to use python distutils to install the python wrappers.
  IF(NOT VTK_INSTALL_NO_PYTHON)
    SET(DOLLAR "$")
 
    # Create default python setup arguments if they are not set.
    IF(NOT DEFINED VTKVTG_PYTHON_SETUP_ARGS)
      SET(VTKVTG_PYTHON_SETUP_ARGS "--prefix=\"${DOLLAR}{CMAKE_INSTALL_PREFIX}\""
        CACHE STRING "Arguments passed to \"python setup.py install ...\" during installation.")
      MARK_AS_ADVANCED(VTKVTG_PYTHON_SETUP_ARGS)
    ENDIF(NOT DEFINED VTKVTG_PYTHON_SETUP_ARGS)

    # Change dollar sign to DOLLARSIGN
    STRING(REGEX REPLACE "\\${DOLLAR}" "DOLLARSIGN"
      PYTHON_SETUP_ARGS "${VTKVTG_PYTHON_SETUP_ARGS}")

    # Get the install path for python modules
    EXEC_PROGRAM("${PYTHON_EXECUTABLE}" "${VTKVTG_SOURCE_DIR}/Wrapping/Python"
      ARGS "setup_install_paths.py" "purelib" ${PYTHON_SETUP_ARGS}
      OUTPUT_VARIABLE PYTHON_MODULE_INSTALL_DIR)

    # Get the install path for binary python modules
    EXEC_PROGRAM("${PYTHON_EXECUTABLE}" "${VTKVTG_SOURCE_DIR}/Wrapping/Python"
      ARGS "setup_install_paths.py" "platlib" ${PYTHON_SETUP_ARGS}
      OUTPUT_VARIABLE PYTHON_MODULE_PLATFORM_INSTALL_DIR)

    # Change DOLLARSIGN to dollar sign
    STRING(REGEX REPLACE "DOLLARSIGN" "${DOLLAR}"
      PYTHON_MODULE_INSTALL_DIR "${PYTHON_MODULE_INSTALL_DIR}")
    STRING(REGEX REPLACE "DOLLARSIGN" "${DOLLAR}"
      PYTHON_MODULE_PLATFORM_INSTALL_DIR
      "${PYTHON_MODULE_PLATFORM_INSTALL_DIR}")

    # Replace backslashes with forward slashes
    IF (WIN32)
      STRING(REGEX REPLACE "\\\\" "/" PYTHON_MODULE_INSTALL_DIR
        "${PYTHON_MODULE_INSTALL_DIR}")
      STRING(REGEX REPLACE "\\\\" "/" PYTHON_MODULE_PLATFORM_INSTALL_DIR
        "${PYTHON_MODULE_PLATFORM_INSTALL_DIR}")
    ENDIF (WIN32)

    SET(VTKVTG_PYTHON_MODULE_INSTALL_DIR "${PYTHON_MODULE_INSTALL_DIR}"
      CACHE INTERNAL "Install directory for Python .py and .pyc files")

    SET(VTKVTG_PYTHON_MODULE_PLATFORM_INSTALL_DIR
      "${PYTHON_MODULE_PLATFORM_INSTALL_DIR}"
      CACHE INTERNAL "Install directory for Python binary modules")

    # If there are multiple configurations then add a BUILD_TYPE=...
    # argument to the python setup.py call.  The build type to use is set
    # in the CMake variable BUILD_TYPE while running the install script.
    IF(CMAKE_CONFIGURATION_TYPES)
      SET(VTKVTG_PYTHON_SETUP_BUILD_TYPE "BUILD_TYPE=${DOLLAR}{BUILD_TYPE}")
    ELSE(CMAKE_CONFIGURATION_TYPES)
      SET(VTKVTG_PYTHON_SETUP_BUILD_TYPE)
    ENDIF(CMAKE_CONFIGURATION_TYPES)

    # Configure the post-install script to run python on setup.py.
    CONFIGURE_FILE(${VTKVTG_SOURCE_DIR}/Wrapping/Python/PythonInstall.cmake.in
                   ${VTKVTG_BINARY_DIR}/Wrapping/Python/PythonInstall.cmake
                   @ONLY IMMEDIATE)
    SET_TARGET_PROPERTIES(vtgpython_compiles PROPERTIES POST_INSTALL_SCRIPT
      ${VTK_BINARY_DIR}/Wrapping/Python/PythonInstall.cmake
      )
  ENDIF(NOT VTK_INSTALL_NO_PYTHON)
ENDIF(1)

# Create the setup.py file.
IF(CMAKE_CONFIGURATION_TYPES)
  # The build has multiple configuration types.  If CMAKE_BUILD_TYPE
  # is set use it as the default BUILD_TYPE for setup.py to install.
  SET(VTKVTG_PYTHON_HAS_CONFIG_TYPES 1)
  IF(CMAKE_BUILD_TYPE)
    SET(VTKVTG_PYTHON_BUILD_TYPE "\"${CMAKE_BUILD_TYPE}\"")
  ELSE(CMAKE_BUILD_TYPE)
    SET(VTKVTG_PYTHON_BUILD_TYPE "[]")
  ENDIF(CMAKE_BUILD_TYPE)
ELSE(CMAKE_CONFIGURATION_TYPES)
  # The build has one configuration type.  The build type does not
  # affect installation.
  SET(VTKVTG_PYTHON_HAS_CONFIG_TYPES 0)
  SET(VTKVTG_PYTHON_BUILD_TYPE "[]")
ENDIF(CMAKE_CONFIGURATION_TYPES)

CONFIGURE_FILE(${VTKVTG_SOURCE_DIR}/Wrapping/Python/setup.py.in
               ${VTKVTG_BINARY_DIR}/Wrapping/Python/setup.py @ONLY IMMEDIATE)
               
# Need to have a custom target so above custom commands will be executed with build
add_custom_target(vtkvtgPythonWrap ALL
                  DEPENDS ${VTKVTG_PYTHON_SOURCE_FILES} ${CMAKE_CURRENT_BINARY_DIR}/compile_all_vtk.py
                  )

# Allow the user to customize their build with some local options
#
INCLUDE (${VTKVTG_BINARY_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTKVTG_SOURCE_DIR}/Wrapping/Tcl/LocalUserOptions.cmake OPTIONAL)
