#
# Source files
#
# Here is where you can add the name of your local common classes.
#

SET (Graphics_SRCS
  vtkMyGlyph3D.cxx
  vtkMyClipDataSet.cxx
)

# --------------------------------------------------------------------------
# You probably do not need to modify anything below this line

# Create the vtkvtgGraphics C++ library.
ADD_LIBRARY (vtkvtgGraphics ${Graphics_SRCS})
TARGET_LINK_LIBRARIES (vtkvtgGraphics vtkGraphics)

# Create the vtkvtgGraphics Tcl library.
IF (VTK_WRAP_TCL AND VTKVTG_WRAP_TCL)
  VTK_WRAP_TCL3 (vtkvtgGraphicsTCL GraphicsTCL_SRCS "${Graphics_SRCS}" "")
  ADD_LIBRARY (vtkvtgGraphicsTCL ${GraphicsTCL_SRCS})
  TARGET_LINK_LIBRARIES (vtkvtgGraphicsTCL vtkvtgGraphics vtkGraphicsTCL)
ENDIF (VTK_WRAP_TCL AND VTKVTG_WRAP_TCL)

# Create the vtkvtgGraphics Python library.
IF (VTK_WRAP_PYTHON AND VTKVTG_WRAP_PYTHON)
  VTK_WRAP_PYTHON3 (vtkvtgGraphicsPython GraphicsPython_SRCS "${Graphics_SRCS}")
  ADD_LIBRARY (vtkvtgGraphicsPythonD ${GraphicsPython_SRCS})
  ADD_LIBRARY (vtkvtgGraphicsPython MODULE vtkvtgGraphicsPythonInit.cxx)
  TARGET_LINK_LIBRARIES(vtkvtgGraphicsPythonD vtkvtgGraphics vtkGraphics vtkGraphicsPythonD)
  TARGET_LINK_LIBRARIES(vtkvtgGraphicsPython vtkvtgGraphicsPythonD)
ENDIF (VTK_WRAP_PYTHON AND VTKVTG_WRAP_PYTHON)

# Create the vtkvtgGraphics Java library.
IF (VTK_WRAP_JAVA AND VTKVTG_WRAP_JAVA)
  VTK_WRAP_JAVA3 (vtkvtgGraphicsJava GraphicsJava_SRCS "${Graphics_SRCS}")
  ADD_LIBRARY (vtkvtgGraphicsJava SHARED ${GraphicsJava_SRCS})
  TARGET_LINK_LIBRARIES (vtkvtgGraphicsJava vtkvtgGraphics vtkGraphicsJava)
ENDIF (VTK_WRAP_JAVA AND VTKVTG_WRAP_JAVA)
