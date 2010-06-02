/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMyChartParallelCoordinates.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkMyChartParallelCoordinates - Factory class for drawing 2D charts
//
// .SECTION Description
// This defines the interface for a parallel coordinates chart.

#ifndef __vtkMyChartParallelCoordinates_h
#define __vtkMyChartParallelCoordinates_h

#include "vtkChart.h"

class vtkIdTypeArray;
class vtkStringArray;

class VTK_CHARTS_EXPORT vtkMyChartParallelCoordinates : public vtkChart
{
public:
  vtkTypeMacro(vtkMyChartParallelCoordinates, vtkChart);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a parallel coordinates chart
  static vtkMyChartParallelCoordinates* New();

  // Description:
  // Perform any updates to the item that may be necessary before rendering.
  // The scene should take care of calling this on all items before their
  // Paint function is invoked.
  virtual void Update();

  // Description:
  // Paint event for the chart, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Set the visibility of the specified column.
  void SetColumnVisibility(const char* name, bool visible);

  // Description:
  // Get the visibility of the specified column.
  bool GetColumnVisibility(const char* name);

  // Description:
  // Get a list of the columns, and the order in which they are displayed.
  vtkGetObjectMacro(VisibleColumns, vtkStringArray);

  // Description:
  // Add a plot to the chart, defaults to using the name of the y column
  virtual vtkPlot* AddPlot(int type);

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
  // Get the axis specified by axisIndex.
  virtual vtkAxis* GetAxis(int axisIndex);

  // Description:
  // Get the number of axes in the current chart.
  virtual vtkIdType GetNumberOfAxes();

  // Description:
  // Request that the chart recalculates the range of its axes. Especially
  // useful in applications after the parameters of plots have been modified.
  virtual void RecalculateBounds();
  
  // Description
  // When axes come in sets, draw rectangles between them to show groupings
  vtkSetMacro(DrawSets, bool);

  // Description
  // When axes come in sets, draw rectangles between them to show groupings
  vtkSetMacro(NumPerSet, int);

  // Description
  // When axes come in sets, draw an extra dark box at the current scale and
  // a semi-transparent box over "non-valid" scales
  vtkSetMacro(CurrentScale, int);

  // Description:
  // Set the vtkHighlightLink for the chart.
  virtual void SetHighlightLink(vtkAnnotationLink *link);

  // Description:
  // Get the vtkHighlightLink for the chart.
  vtkGetObjectMacro(HighlightLink, vtkAnnotationLink);


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

//BTX
protected:
  vtkMyChartParallelCoordinates();
  ~vtkMyChartParallelCoordinates();

  // Description:
  // Private storage object - where we hide all of our STL objects...
  class Private;
  Private *Storage;

  bool GeometryValid;

  // Description:
  // Selected indices for the table the plot is rendering
  vtkIdTypeArray *Selection;

  // Description:
  // Selected indices for the table the plot is rendering 
  // coming back from outside this chart
  vtkIdTypeArray *HighlightSelection;

  // Description:
  // A list of the visible columns in the chart.
  vtkStringArray *VisibleColumns;

  // Description:
  // The point cache is marked dirty until it has been initialized.
  vtkTimeStamp BuildTime;

  void ResetSelection();
  void UpdateGeometry();
  void CalculatePlotTransform();
  
  // Description:
  // Whether set-grouping rectangles get drawn, and how big those sets are.
  bool DrawSets;
  int NumPerSet;
  int CurrentScale;
  
  // Description:
  // Link back into chart to highlight selections made in other plots
  vtkAnnotationLink *HighlightLink;

private:
  vtkMyChartParallelCoordinates(const vtkMyChartParallelCoordinates &); // Not implemented.
  void operator=(const vtkMyChartParallelCoordinates &);   // Not implemented.
//ETX
};

#endif //__vtkMyChartParallelCoordinates_h
