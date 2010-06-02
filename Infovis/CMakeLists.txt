#
# Source files
#
# Here is where you can add the name of your local common classes.
#

SET (Infovis_SRCS
  vtkTexturedTreeMapToPolyData.cxx	
)

# --------------------------------------------------------------------------
# You probably do not need to modify anything below this line

# Create the vtkvtgInfovis C++ library.
ADD_LIBRARY (vtkvtgInfovis ${Infovis_SRCS})
TARGET_LINK_LIBRARIES (vtkvtgInfovis vtkInfovis)

# Create the vtkvtgInfovis Tcl library.
IF (VTK_WRAP_TCL AND VTKVTG_WRAP_TCL)
  VTK_WRAP_TCL3 (vtkvtgInfovisTCL InfovisTCL_SRCS "${Infovis_SRCS}" "")
  ADD_LIBRARY (vtkvtgInfovisTCL ${InfovisTCL_SRCS})
  TARGET_LINK_LIBRARIES (vtkvtgInfovisTCL vtkvtgInfovis vtkInfovisTCL)
ENDIF (VTK_WRAP_TCL AND VTKVTG_WRAP_TCL)

# Create the vtkvtgInfovis Python library.
IF (VTK_WRAP_PYTHON AND VTKVTG_WRAP_PYTHON)
  VTK_WRAP_PYTHON3 (vtkvtgInfovisPython InfovisPython_SRCS "${Infovis_SRCS}")
  ADD_LIBRARY (vtkvtgInfovisPythonD ${InfovisPython_SRCS})
  ADD_LIBRARY (vtkvtgInfovisPython MODULE vtkvtgInfovisPythonInit.cxx)
  TARGET_LINK_LIBRARIES(vtkvtgInfovisPythonD vtkvtgInfovis vtkInfovis vtkInfovisPythonD)
  TARGET_LINK_LIBRARIES(vtkvtgInfovisPython vtkvtgInfovisPythonD)
ENDIF (VTK_WRAP_PYTHON AND VTKVTG_WRAP_PYTHON)

# Create the vtkvtgInfovis Java library.
IF (VTK_WRAP_JAVA AND VTKVTG_WRAP_JAVA)
  VTK_WRAP_JAVA3 (vtkvtgInfovisJava InfovisJava_SRCS "${Infovis_SRCS}")
  ADD_LIBRARY (vtkvtgInfovisJava SHARED ${InfovisJava_SRCS})
  TARGET_LINK_LIBRARIES (vtkvtgInfovisJava vtkvtgInfovis vtkInfovisJava)
ENDIF (VTK_WRAP_JAVA AND VTKVTG_WRAP_JAVA)
