#
# Source files
#
# Here is where you can add the name of your local common classes.
#

SET (Widgets_SRCS
  vtkSlabRepresentation.cxx
  vtkSlabWidget2.cxx
)

# --------------------------------------------------------------------------
# You probably do not need to modify anything below this line

# Create the vtkvtgWidgets C++ library.
ADD_LIBRARY (vtkvtgWidgets ${Widgets_SRCS})
TARGET_LINK_LIBRARIES (vtkvtgWidgets vtkvtgCommon vtkWidgets)

# Create the vtkvtgWidgets Tcl library.
IF (VTK_WRAP_TCL AND VTKVTG_WRAP_TCL)
  VTK_WRAP_TCL3 (vtkvtgWidgetsTCL WidgetsTCL_SRCS "${Widgets_SRCS}" "")
  ADD_LIBRARY (vtkvtgWidgetsTCL ${WidgetsTCL_SRCS})
  TARGET_LINK_LIBRARIES (vtkvtgWidgetsTCL vtkvtgWidgets vtkWidgetsTCL)
ENDIF (VTK_WRAP_TCL AND VTKVTG_WRAP_TCL)

# Create the vtkvtgWidgets Python library.
IF (VTK_WRAP_PYTHON AND VTKVTG_WRAP_PYTHON)
  VTK_WRAP_PYTHON3 (vtkvtgWidgetsPython WidgetsPython_SRCS "${Widgets_SRCS}")
  ADD_LIBRARY (vtkvtgWidgetsPythonD ${WidgetsPython_SRCS})
  ADD_LIBRARY (vtkvtgWidgetsPython MODULE vtkvtgWidgetsPythonInit.cxx)
  TARGET_LINK_LIBRARIES(vtkvtgWidgetsPythonD vtkvtgWidgets vtkWidgets vtkWidgetsPythonD)
  TARGET_LINK_LIBRARIES(vtkvtgWidgetsPython vtkvtgWidgetsPythonD)
ENDIF (VTK_WRAP_PYTHON AND VTKVTG_WRAP_PYTHON)

# Create the vtkvtgWidgets Java library.
IF (VTK_WRAP_JAVA AND VTKVTG_WRAP_JAVA)
  VTK_WRAP_JAVA3 (vtkvtgWidgetsJava WidgetsJava_SRCS "${Widgets_SRCS}")
  ADD_LIBRARY (vtkvtgWidgetsJava SHARED ${WidgetsJava_SRCS})
  TARGET_LINK_LIBRARIES (vtkvtgWidgetsJava vtkvtgWidgets vtkWidgetsJava)
ENDIF (VTK_WRAP_JAVA AND VTKVTG_WRAP_JAVA)
