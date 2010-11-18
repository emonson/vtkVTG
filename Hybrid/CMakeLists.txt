#
# Source files
#
# Here is where you can add the name of your local common classes.
#

SET (Hybrid_SRCS
  vtkVRMLSource.cxx
)

# --------------------------------------------------------------------------
# You probably do not need to modify anything below this line

# Create the vtkvtgHybrid C++ library.
ADD_LIBRARY (vtkvtgHybrid ${Hybrid_SRCS})
TARGET_LINK_LIBRARIES (vtkvtgHybrid vtkHybrid)

# Create the vtkvtgHybrid Tcl library.
IF (VTK_WRAP_TCL AND VTKVTG_WRAP_TCL)
  VTK_WRAP_TCL3 (vtkvtgHybridTCL HybridTCL_SRCS "${Hybrid_SRCS}" "")
  ADD_LIBRARY (vtkvtgHybridTCL ${HybridTCL_SRCS})
  TARGET_LINK_LIBRARIES (vtkvtgHybridTCL vtkvtgHybrid vtkHybridTCL)
ENDIF (VTK_WRAP_TCL AND VTKVTG_WRAP_TCL)

# Create the vtkvtgHybrid Python library.
IF (VTK_WRAP_PYTHON AND VTKVTG_WRAP_PYTHON)
  VTK_WRAP_PYTHON3 (vtkvtgHybridPython HybridPython_SRCS "${Hybrid_SRCS}")
  ADD_LIBRARY (vtkvtgHybridPythonD ${HybridPython_SRCS})
  ADD_LIBRARY (vtkvtgHybridPython MODULE vtkvtgHybridPythonInit.cxx)
  TARGET_LINK_LIBRARIES(vtkvtgHybridPythonD vtkvtgHybrid vtkHybrid vtkHybridPythonD)
  TARGET_LINK_LIBRARIES(vtkvtgHybridPython vtkvtgHybridPythonD)
ENDIF (VTK_WRAP_PYTHON AND VTKVTG_WRAP_PYTHON)

# Create the vtkvtgHybrid Java library.
IF (VTK_WRAP_JAVA AND VTKVTG_WRAP_JAVA)
  VTK_WRAP_JAVA3 (vtkvtgHybridJava HybridJava_SRCS "${Hybrid_SRCS}")
  ADD_LIBRARY (vtkvtgHybridJava SHARED ${HybridJava_SRCS})
  TARGET_LINK_LIBRARIES (vtkvtgHybridJava vtkvtgHybrid vtkHybridJava)
ENDIF (VTK_WRAP_JAVA AND VTKVTG_WRAP_JAVA)
