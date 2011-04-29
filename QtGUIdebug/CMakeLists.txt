#
# Source files
#
# Here is where you can add the name of your local common classes.
#

SET (QVTKdebug_SRCS
  vtkQtWordleDebugView.cxx
)

SET (QVTKdebug_headers
  vtkQtWordleDebugView.h
)

# use what QVTK built with
SET(QT_MOC_EXECUTABLE ${VTK_QT_MOC_EXECUTABLE} CACHE FILEPATH "")
SET(QT_UIC_EXECUTABLE ${VTK_QT_UIC_EXECUTABLE} CACHE FILEPATH "")
SET(QT_QMAKE_EXECUTABLE ${VTK_QT_QMAKE_EXECUTABLE} CACHE FILEPATH "")

# original line (no svg functionality)
FIND_PACKAGE(Qt4 REQUIRED)
# with svg requirement
# FIND_PACKAGE(Qt4 4.6.2 COMPONENTS QtCore QtGui QtSvg REQUIRED)

INCLUDE(${QT_USE_FILE})

# Use the include path and library for Qt that is used by VTK.
INCLUDE_DIRECTORIES(
  ${QT_INCLUDE_DIR}
)

# The rest should just work (sure...)
QT4_WRAP_CPP(MOCSrcs ${QVTKdebug_headers} )

# --------------------------------------------------------------------------
# You probably do not need to modify anything below this line

# Create the vtkvtgQVTKdebug C++ library.
ADD_LIBRARY (vtkvtgQVTKdebug ${QVTKdebug_SRCS} ${MOCSrcs})

# original line (no svg functionality)
TARGET_LINK_LIBRARIES (vtkvtgQVTKdebug QVTK)
# with svg requirement
# TARGET_LINK_LIBRARIES (vtkvtgQVTKdebug QVTK ${QT_LIBRARIES})

# Create the vtkvtgQVTKdebug Tcl library.
IF (VTK_WRAP_TCL AND VTKVTG_WRAP_TCL)
  VTK_WRAP_TCL3 (vtkvtgQVTKdebugTCL QVTKTCL_SRCS "${QVTKdebug_SRCS}" "")
  ADD_LIBRARY (vtkvtgQVTKdebugTCL ${QVTKTCL_SRCS})
  TARGET_LINK_LIBRARIES (vtkvtgQVTKdebugTCL vtkvtgQVTKdebug QVTKTCL)
ENDIF (VTK_WRAP_TCL AND VTKVTG_WRAP_TCL)

# Create the vtkvtgQVTKdebug Python library.
IF (VTK_WRAP_PYTHON AND VTKVTG_WRAP_PYTHON)
  VTK_WRAP_PYTHON3 (vtkvtgQVTKdebugPython QVTKPython_SRCS "${QVTKdebug_SRCS}")
  ADD_LIBRARY (vtkvtgQVTKdebugPythonD ${QVTKPython_SRCS})
  ADD_LIBRARY (vtkvtgQVTKdebugPython MODULE vtkvtgQVTKdebugPythonInit.cxx)
  TARGET_LINK_LIBRARIES(vtkvtgQVTKdebugPythonD vtkvtgQVTKdebug QVTK vtkQtPythonD)
  TARGET_LINK_LIBRARIES(vtkvtgQVTKdebugPython vtkvtgQVTKdebugPythonD)
ENDIF (VTK_WRAP_PYTHON AND VTKVTG_WRAP_PYTHON)

# Create the vtkvtgQVTKdebug Java library.
IF (VTK_WRAP_JAVA AND VTKVTG_WRAP_JAVA)
  VTK_WRAP_JAVA3 (vtkvtgQVTKdebugJava QVTKJava_SRCS "${QVTKdebug_SRCS}")
  ADD_LIBRARY (vtkvtgQVTKdebugJava SHARED ${QVTKJava_SRCS})
  TARGET_LINK_LIBRARIES (vtkvtgQVTKdebugJava vtkvtgQVTKdebug vtkQVTKJava)
ENDIF (VTK_WRAP_JAVA AND VTKVTG_WRAP_JAVA)
