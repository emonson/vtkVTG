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

#include "vtkChartParallelCoordinates.h"
#include <vector>

class vtkIdTypeArray;

class VTK_CHARTS_EXPORT vtkMyChartParallelCoordinates : public vtkChartParallelCoordinates
{
public:
  vtkTypeMacro(vtkMyChartParallelCoordinates, vtkChartParallelCoordinates);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a parallel coordinates chart
  static vtkMyChartParallelCoordinates* New();

  // Description:
  // Paint event for the chart, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);

  
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

  // Description
  // This controls annotations for which axes are being plotted in a separate XY
  // plot. References indices within "current scale".
  vtkSetMacro(XYcurrentX, int);
  vtkSetMacro(XYcurrentY, int);

  // Description:
  // Set the vtkHighlightLink for the chart.
  virtual void SetHighlightLink(vtkAnnotationLink *link);

  // Description:
  // Get the vtkHighlightLink for the chart.
  vtkGetObjectMacro(HighlightLink, vtkAnnotationLink);

  // Description:
  // Set the number of scales which will be represented by boxes in the
  // chart. This needs to be set before calling SetScaleDim(index, dim_size)
  virtual void SetNumberOfScales(int num_scales);

  // Description:
  // Set the number of dimensions for each individual scale so the decorations
  // can be drawn properly. Call SetNumberOfScales(int num) before this.
  virtual void SetScaleDim(vtkIdType index, int dim_size);

  // Description:
  // Set all columns to invisible in one shot so don't have to run through
  // all of them when totally switching data
  virtual void SetAllColumnsInvisible();

//BTX
protected:
  vtkMyChartParallelCoordinates();
  ~vtkMyChartParallelCoordinates();

  // Description:
  // Selected indices for the table the plot is rendering 
  // coming back from outside this chart
  vtkIdTypeArray *HighlightSelection;

  // Description:
  // Whether set-grouping rectangles get drawn, and how big those sets are.
  bool DrawSets;
  int NumPerSet;
  int CurrentScale;
  int XYcurrentX;
  int XYcurrentY;
  std::vector<int> ScaleDims;
  
  // Description:
  // Link back into chart to highlight selections made in other plots
  vtkAnnotationLink *HighlightLink;

private:
  vtkMyChartParallelCoordinates(const vtkMyChartParallelCoordinates &); // Not implemented.
  void operator=(const vtkMyChartParallelCoordinates &);   // Not implemented.
//ETX
};

#endif //__vtkMyChartParallelCoordinates_h
