#!/bin/bash

COMPDIR=/Users/emonson/Programming/VTK_git/VTK
MID="git"

diff -u $COMPDIR/Charts/vtkChartParallelCoordinates.cxx vtkMyChartParallelCoordinates.cxx > ChartPC_${MID}_cxx.diff
diff -u $COMPDIR/Charts/vtkChartParallelCoordinates.h vtkMyChartParallelCoordinates.h > ChartPC_${MID}_h.diff
diff -u $COMPDIR/Charts/vtkChartXY.h vtkMyChartXY.h > ChartXY_${MID}_h.diff
diff -u $COMPDIR/Charts/vtkChartXY.cxx vtkMyChartXY.cxx > ChartXY_${MID}_cxx.diff
diff -u $COMPDIR/Charts/vtkPlotPoints.cxx vtkMyPlotPoints.cxx > PlotPoints_${MID}_cxx.diff
diff -u $COMPDIR/Charts/vtkPlotPoints.h vtkMyPlotPoints.h > PlotPoints_${MID}_h.diff
diff -u $COMPDIR/Charts/vtkPlotParallelCoordinates.h vtkMyPlotParallelCoordinates.h > PlotPC_${MID}_h.diff
diff -u $COMPDIR/Charts/vtkPlotParallelCoordinates.cxx vtkMyPlotParallelCoordinates.cxx > PlotPC_${MID}_cxx.diff
diff -u $COMPDIR/Charts/vtkImageItem.h vtkTooltipImageItem.h > II_${MID}_h.diff
diff -u $COMPDIR/Charts/vtkImageItem.cxx vtkTooltipImageItem.cxx > II_${MID}_cxx.diff
