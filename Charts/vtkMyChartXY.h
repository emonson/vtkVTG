/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMyChartXY.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkMyChartXY - Factory class for drawing XY charts
//
// .SECTION Description
// This class implements an XY chart.

#ifndef __vtkMyChartXY_h
#define __vtkMyChartXY_h

#include "vtkChartXY.h"

class vtkPlot;
class vtkAxis;
class vtkPlotGrid;
class vtkTable;
class vtkChartLegend;
class vtkTooltipImageItem;
class vtkContextMouseEvent;
class vtkDataArray;
class vtkMyChartXYPrivate; // Private class to keep my STL vector in...

class VTK_CHARTS_EXPORT vtkMyChartXY : public vtkChartXY
{
public:
  vtkTypeMacro(vtkMyChartXY, vtkChartXY);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a 2D Chart object.
  static vtkMyChartXY *New();

  // Description:
  // Perform any updates to the item that may be necessary before rendering.
  // The scene should take care of calling this on all items before their
  // Paint function is invoked.
  virtual void Update();

  // Description:
  // Paint event for the chart, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Add a plot to the chart, defaults to using the name of the y column
  virtual vtkPlot * AddPlot(int type);

  // Description:
  // Remove the plot at the specified index, returns true if successful,
  // false if the index was invalid.
  virtual bool RemovePlot(vtkIdType index);

  // Description:
  // Remove all plots from the chart.
  virtual void ClearPlots();

  // Description:
  // Get the plot at the specified index, returns null if the index is invalid.
  virtual vtkPlot* GetPlot(vtkIdType index);

  // Description:
  // Get the number of plots the chart contains.
  virtual vtkIdType GetNumberOfPlots();

  // Description:
  // Figure out which quadrant the plot is in.
  int GetPlotCorner(vtkPlot *plot);

  // Description:
  // Figure out which quadrant the plot is in.
  void SetPlotCorner(vtkPlot *plot, int corner);

  // Description:
  // Get the axis specified by axisIndex. This should probably
  // be improved either using a string or enum to select the axis.
  // (0 - left, 1 - bottom, 2 - right, 3 - top).
  virtual vtkAxis* GetAxis(int axisIndex);

  // Description:
  // Get the number of axes in the current chart.
  virtual vtkIdType GetNumberOfAxes();

  // Description:
  // Request that the chart recalculates the range of its axes. Especially
  // useful in applications after the parameters of plots have been modified.
  virtual void RecalculateBounds();

  // Description:
  // If true then the axes will be drawn at the origin (scientific style).
  vtkSetMacro(DrawAxesAtOrigin, bool);
  vtkGetMacro(DrawAxesAtOrigin, bool);

  // Description:
  // Set the width fraction for any bar charts drawn in this chart. It is
  // assumed that all bar plots will use the same array for the X axis, and that
  // this array is regularly spaced. The delta between the first two x values is
  // used to calculated the width of the bars, and subdivided between each bar.
  // The default value is 0.8, 1.0 would lead to bars that touch.
  vtkSetMacro(BarWidthFraction, float);
  vtkGetMacro(BarWidthFraction, float);
  
  // Description
  // Set whether tooltip will show image or text
  virtual void SetTooltipShowImage(bool showImage);

//BTX
  // Description:
  // Return true if the supplied x, y coordinate is inside the item.
  virtual bool Hit(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse enter event.
  virtual bool MouseEnterEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse move event.
  virtual bool MouseMoveEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse leave event.
  virtual bool MouseLeaveEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse button down event
  virtual bool MouseButtonPressEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse button release event.
  virtual bool MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse wheel event, positive delta indicates forward movement of the wheel.
  virtual bool MouseWheelEvent(const vtkContextMouseEvent &mouse, int delta);
//ETX

  // Description:
  // Set the vtkContextScene for the item, always set for an item in a scene.
  virtual void SetScene(vtkContextScene *scene);

  // Description:
  // Return the Stacked plot accumulator so that each vtkPlotStacked can 
  // use it to determine its base and contribute to the position of the next
  // stacked plot.
  vtkDataArray *GetStackedPlotAccumulator(int dataType, int n);

  // Description:
  // Timestamp identifying the last time the participants in a stacked plot
  // have changed (either by being added or having their visibility change)
  vtkTimeStamp GetStackParticipantsChanged();
  void SetStackPartipantsChanged();

//BTX
protected:
  vtkMyChartXY();
  ~vtkMyChartXY();

  // Description:
  // Recalculate the necessary transforms.
  void RecalculatePlotTransforms();

  // Description:
  // Recalculate the supplied transform for the two axes.
  void RecalculatePlotTransform(vtkAxis *x, vtkAxis *y,
                                vtkTransform2D *transform);

  // Description:
  // Calculate the optimal zoom level such that all of the points to be plotted
  // will fit into the plot area.
  void RecalculatePlotBounds();

  // Description:
  // Process a rubber band selection event.
  virtual void ProcessSelectionEvent(vtkObject* caller, void* callData);

  // Description:
  // The grid for the chart.
  vtkPlotGrid *Grid;

  // Description:
  // The legend for the chart.
  vtkChartLegend *Legend;

  // Description:
  // The tooltip item for the chart - can be used to display extra information.
  vtkTooltipImageItem *Tooltip;
  bool TooltipShowImage;

  // Description:
  // Does the plot area transform need to be recalculated?
  bool PlotTransformValid;

  // Description:
  // The origin of the box when selecting a region of the chart.
  float BoxOrigin[2];

  // Description:
  // The width and height of the selection box.
  float BoxGeometry[2];

  // Description:
  // Should the box be drawn (could be selection, zoom etc).
  bool DrawBox;

  // Description:
  // Should we draw the location of the nearest point on the plot?
  bool DrawNearestPoint;

  // Description:
  // Keep the axes drawn at the origin? This will attempt to keep the axes drawn
  // at the origin, i.e. 0.0, 0.0 for the chart. This is often the preferred
  // way of drawing scientific/mathematical charts.
  bool DrawAxesAtOrigin;

  // Description:
  // The fraction of the interval taken up along the x axis by any bars that are
  // drawn on the chart.
  float BarWidthFraction;

private:
  vtkMyChartXY(const vtkMyChartXY &); // Not implemented.
  void operator=(const vtkMyChartXY &);   // Not implemented.

  vtkMyChartXYPrivate *ChartPrivate; // Private class where I hide my STL containers

  // Description:
  // Private functions to render different parts of the chart
  void RenderPlots(vtkContext2D *painter);

  // Description:
  // Figure out the spacing between the bar chart plots, and their offsets.
  void CalculateBarPlots();

  // Description:
  // Try to locate a point within the plots to display in a tooltip
  bool LocatePointInPlots(const vtkContextMouseEvent &mouse);

  // Description:
  // Remove the plot from the plot corners list.
  bool RemovePlotFromConers(vtkPlot *plot);

  void ZoomInAxes(vtkAxis *x, vtkAxis *y, float *orign, float *max);

//ETX
};

#endif //__vtkMyChartXY_h
