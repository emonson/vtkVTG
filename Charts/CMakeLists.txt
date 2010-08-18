#
# Source files
#
# Here is where you can add the name of your local common classes.
#

SET (Charts_SRCS
  vtkAxisImageItem.cxx
  vtkMyPlotParallelCoordinates.cxx
  vtkMyChartParallelCoordinates.cxx
  vtkMyPlotPoints.cxx
  vtkMyChartXY.cxx
  vtkTooltipImageItem.cxx
)

# --------------------------------------------------------------------------
# You probably do not need to modify anything below this line

# Create the vtkvtgCharts C++ library.
ADD_LIBRARY (vtkvtgCharts ${Charts_SRCS})
TARGET_LINK_LIBRARIES (vtkvtgCharts vtkCharts)

# Create the vtkvtgCharts Tcl library.
IF (VTK_WRAP_TCL AND VTKVTG_WRAP_TCL)
  VTK_WRAP_TCL3 (vtkvtgChartsTCL ChartsTCL_SRCS "${Charts_SRCS}" "")
  ADD_LIBRARY (vtkvtgChartsTCL ${ChartsTCL_SRCS})
  TARGET_LINK_LIBRARIES (vtkvtgChartsTCL vtkvtgCharts vtkChartsTCL)
ENDIF (VTK_WRAP_TCL AND VTKVTG_WRAP_TCL)

# Create the vtkvtgCharts Python library.
IF (VTK_WRAP_PYTHON AND VTKVTG_WRAP_PYTHON)
  VTK_WRAP_PYTHON3 (vtkvtgChartsPython ChartsPython_SRCS "${Charts_SRCS}")
  ADD_LIBRARY (vtkvtgChartsPythonD ${ChartsPython_SRCS})
  ADD_LIBRARY (vtkvtgChartsPython MODULE vtkvtgChartsPythonInit.cxx)
  TARGET_LINK_LIBRARIES(vtkvtgChartsPythonD vtkvtgCharts vtkCharts vtkChartsPythonD)
  TARGET_LINK_LIBRARIES(vtkvtgChartsPython vtkvtgChartsPythonD)
ENDIF (VTK_WRAP_PYTHON AND VTKVTG_WRAP_PYTHON)

# Create the vtkvtgCharts Java library.
IF (VTK_WRAP_JAVA AND VTKVTG_WRAP_JAVA)
  VTK_WRAP_JAVA3 (vtkvtgChartsJava ChartsJava_SRCS "${Charts_SRCS}")
  ADD_LIBRARY (vtkvtgChartsJava SHARED ${ChartsJava_SRCS})
  TARGET_LINK_LIBRARIES (vtkvtgChartsJava vtkvtgCharts vtkChartsJava)
ENDIF (VTK_WRAP_JAVA AND VTKVTG_WRAP_JAVA)
