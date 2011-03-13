#
# Source files
#
# Here is where you can add the name of your local common classes.
#

SET (QVTK_SRCS
  vtkQtWordleView.cxx
)

SET (QVTK_headers
  vtkQtWordleView.h
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
QT4_WRAP_CPP(MOCSrcs ${QVTK_headers} )

# --------------------------------------------------------------------------
# You probably do not need to modify anything below this line

# Create the vtkvtgQVTK C++ library.
ADD_LIBRARY (vtkvtgQVTK ${QVTK_SRCS} ${MOCSrcs})

# original line (no svg functionality)
TARGET_LINK_LIBRARIES (vtkvtgQVTK QVTK)
# with svg requirement
# TARGET_LINK_LIBRARIES (vtkvtgQVTK QVTK ${QT_LIBRARIES})

# Create the vtkvtgQVTK Tcl library.
IF (VTK_WRAP_TCL AND VTKVTG_WRAP_TCL)
  VTK_WRAP_TCL3 (vtkvtgQVTKTCL QVTKTCL_SRCS "${QVTK_SRCS}" "")
  ADD_LIBRARY (vtkvtgQVTKTCL ${QVTKTCL_SRCS})
  TARGET_LINK_LIBRARIES (vtkvtgQVTKTCL vtkvtgQVTK QVTKTCL)
ENDIF (VTK_WRAP_TCL AND VTKVTG_WRAP_TCL)

# Create the vtkvtgQVTK Python library.
IF (VTK_WRAP_PYTHON AND VTKVTG_WRAP_PYTHON)
  VTK_WRAP_PYTHON3 (vtkvtgQVTKPython QVTKPython_SRCS "${QVTK_SRCS}")
  ADD_LIBRARY (vtkvtgQVTKPythonD ${QVTKPython_SRCS})
  ADD_LIBRARY (vtkvtgQVTKPython MODULE vtkvtgQVTKPythonInit.cxx)
  TARGET_LINK_LIBRARIES(vtkvtgQVTKPythonD vtkvtgQVTK QVTK vtkQtPythonD)
  TARGET_LINK_LIBRARIES(vtkvtgQVTKPython vtkvtgQVTKPythonD)
ENDIF (VTK_WRAP_PYTHON AND VTKVTG_WRAP_PYTHON)

# Create the vtkvtgQVTK Java library.
IF (VTK_WRAP_JAVA AND VTKVTG_WRAP_JAVA)
  VTK_WRAP_JAVA3 (vtkvtgQVTKJava QVTKJava_SRCS "${QVTK_SRCS}")
  ADD_LIBRARY (vtkvtgQVTKJava SHARED ${QVTKJava_SRCS})
  TARGET_LINK_LIBRARIES (vtkvtgQVTKJava vtkvtgQVTK vtkQVTKJava)
ENDIF (VTK_WRAP_JAVA AND VTKVTG_WRAP_JAVA)
