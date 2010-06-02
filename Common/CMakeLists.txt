#
# Source files
#
# Here is where you can add the name of your local common classes.
#

SET (Common_SRCS
  vtkSlab.cxx
)

# --------------------------------------------------------------------------
# You probably do not need to modify anything below this line

# Create the vtkvtgCommon C++ library.
ADD_LIBRARY (vtkvtgCommon ${Common_SRCS})
TARGET_LINK_LIBRARIES (vtkvtgCommon vtkCommon)

# Create the vtkvtgCommon Tcl library.
IF (VTK_WRAP_TCL AND VTKVTG_WRAP_TCL)
  VTK_WRAP_TCL3 (vtkvtgCommonTCL CommonTCL_SRCS "${Common_SRCS}" "")
  ADD_LIBRARY (vtkvtgCommonTCL ${CommonTCL_SRCS})
  TARGET_LINK_LIBRARIES (vtkvtgCommonTCL vtkvtgCommon vtkCommonTCL)
ENDIF (VTK_WRAP_TCL AND VTKVTG_WRAP_TCL)

# Create the vtkvtgCommon Python library.
IF (VTK_WRAP_PYTHON AND VTKVTG_WRAP_PYTHON)
  VTK_WRAP_PYTHON3 (vtkvtgCommonPython CommonPython_SRCS "${Common_SRCS}")
  ADD_LIBRARY (vtkvtgCommonPythonD ${CommonPython_SRCS})
  ADD_LIBRARY (vtkvtgCommonPython MODULE vtkvtgCommonPythonInit.cxx)
  TARGET_LINK_LIBRARIES(vtkvtgCommonPythonD vtkvtgCommon vtkCommon vtkCommonPythonD)
  TARGET_LINK_LIBRARIES(vtkvtgCommonPython vtkvtgCommonPythonD)
ENDIF (VTK_WRAP_PYTHON AND VTKVTG_WRAP_PYTHON)

# Create the vtkvtgCommon Java library.
IF (VTK_WRAP_JAVA AND VTKVTG_WRAP_JAVA)
  VTK_WRAP_JAVA3 (vtkvtgCommonJava CommonJava_SRCS "${Common_SRCS}")
  ADD_LIBRARY (vtkvtgCommonJava SHARED ${CommonJava_SRCS})
  TARGET_LINK_LIBRARIES (vtkvtgCommonJava vtkvtgCommon vtkCommonJava)
ENDIF (VTK_WRAP_JAVA AND VTKVTG_WRAP_JAVA)
