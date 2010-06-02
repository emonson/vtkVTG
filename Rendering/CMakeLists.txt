#
# Source files
#
# Here is where you can add the name of your local common classes.
#

SET (Rendering_SRCS
  vtkConvertIds.cxx
)

# --------------------------------------------------------------------------
# You probably do not need to modify anything below this line

# Create the vtkvtgRendering C++ library.
ADD_LIBRARY (vtkvtgRendering ${Rendering_SRCS})
TARGET_LINK_LIBRARIES (vtkvtgRendering vtkRendering)

# Create the vtkvtgRendering Tcl library.
IF (VTK_WRAP_TCL AND VTKVTG_WRAP_TCL)
  VTK_WRAP_TCL3 (vtkvtgRenderingTCL RenderingTCL_SRCS "${Rendering_SRCS}" "")
  ADD_LIBRARY (vtkvtgRenderingTCL ${RenderingTCL_SRCS})
  TARGET_LINK_LIBRARIES (vtkvtgRenderingTCL vtkvtgRendering vtkRenderingTCL)
ENDIF (VTK_WRAP_TCL AND VTKVTG_WRAP_TCL)

# Create the vtkvtgRendering Python library.
IF (VTK_WRAP_PYTHON AND VTKVTG_WRAP_PYTHON)
  VTK_WRAP_PYTHON3 (vtkvtgRenderingPython RenderingPython_SRCS "${Rendering_SRCS}")
  ADD_LIBRARY (vtkvtgRenderingPythonD ${RenderingPython_SRCS})
  ADD_LIBRARY (vtkvtgRenderingPython MODULE vtkvtgRenderingPythonInit.cxx)
  TARGET_LINK_LIBRARIES(vtkvtgRenderingPythonD vtkvtgRendering vtkRendering vtkRenderingPythonD)
  TARGET_LINK_LIBRARIES(vtkvtgRenderingPython vtkvtgRenderingPythonD)
ENDIF (VTK_WRAP_PYTHON AND VTKVTG_WRAP_PYTHON)

# Create the vtkvtgRendering Java library.
IF (VTK_WRAP_JAVA AND VTKVTG_WRAP_JAVA)
  VTK_WRAP_JAVA3 (vtkvtgRenderingJava RenderingJava_SRCS "${Rendering_SRCS}")
  ADD_LIBRARY (vtkvtgRenderingJava SHARED ${RenderingJava_SRCS})
  TARGET_LINK_LIBRARIES (vtkvtgRenderingJava vtkvtgRendering vtkRenderingJava)
ENDIF (VTK_WRAP_JAVA AND VTKVTG_WRAP_JAVA)
