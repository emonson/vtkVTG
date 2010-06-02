#
# Source files
#
# Here is where you can add the name of your local common classes.
#

SET (Filtering_SRCS
  vtkMyPolyVertex.cxx
  vtkBezierSurfaceSource.cxx
)

# --------------------------------------------------------------------------
# You probably do not need to modify anything below this line

# Create the vtkvtgFiltering C++ library.
ADD_LIBRARY (vtkvtgFiltering ${Filtering_SRCS})
TARGET_LINK_LIBRARIES (vtkvtgFiltering vtkFiltering)

# Create the vtkvtgFiltering Tcl library.
IF (VTK_WRAP_TCL AND VTKVTG_WRAP_TCL)
  VTK_WRAP_TCL3 (vtkvtgFilteringTCL FilteringTCL_SRCS "${Filtering_SRCS}" "")
  ADD_LIBRARY (vtkvtgFilteringTCL ${FilteringTCL_SRCS})
  TARGET_LINK_LIBRARIES (vtkvtgFilteringTCL vtkvtgFiltering vtkFilteringTCL)
ENDIF (VTK_WRAP_TCL AND VTKVTG_WRAP_TCL)

# Create the vtkvtgFiltering Python library.
IF (VTK_WRAP_PYTHON AND VTKVTG_WRAP_PYTHON)
  VTK_WRAP_PYTHON3 (vtkvtgFilteringPython FilteringPython_SRCS "${Filtering_SRCS}")
  ADD_LIBRARY (vtkvtgFilteringPythonD ${FilteringPython_SRCS})
  ADD_LIBRARY (vtkvtgFilteringPython MODULE vtkvtgFilteringPythonInit.cxx)
  TARGET_LINK_LIBRARIES(vtkvtgFilteringPythonD vtkvtgFiltering vtkFiltering vtkFilteringPythonD)
  TARGET_LINK_LIBRARIES(vtkvtgFilteringPython vtkvtgFilteringPythonD)
ENDIF (VTK_WRAP_PYTHON AND VTKVTG_WRAP_PYTHON)

# Create the vtkvtgFiltering Java library.
IF (VTK_WRAP_JAVA AND VTKVTG_WRAP_JAVA)
  VTK_WRAP_JAVA3 (vtkvtgFilteringJava FilteringJava_SRCS "${Filtering_SRCS}")
  ADD_LIBRARY (vtkvtgFilteringJava SHARED ${FilteringJava_SRCS})
  TARGET_LINK_LIBRARIES (vtkvtgFilteringJava vtkvtgFiltering vtkFilteringJava)
ENDIF (VTK_WRAP_JAVA AND VTKVTG_WRAP_JAVA)
