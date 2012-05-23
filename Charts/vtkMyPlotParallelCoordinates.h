/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMyPlotParallelCoordinates.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkMyPlotParallelCoordinates - Class for drawing an XY plot given two columns from a
// vtkTable.
//
// .SECTION Description
//

#ifndef __vtkMyPlotParallelCoordinates_h
#define __vtkMyPlotParallelCoordinates_h

#include "vtkPlotParallelCoordinates.h"

class vtkImageData;

class VTK_CHARTS_EXPORT vtkMyPlotParallelCoordinates : public vtkPlotParallelCoordinates
{
public:
  vtkTypeMacro(vtkMyPlotParallelCoordinates, vtkPlotParallelCoordinates);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a parallel coordinates chart
  static vtkMyPlotParallelCoordinates* New();

  // Description:
  // Paint event for the XY plot, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);


  // Description:
  // Set the list of ids for highlighting (back from outside plot).
  virtual void SetHighlightSelection(vtkIdTypeArray *id);
  vtkGetObjectMacro(HighlightSelection, vtkIdTypeArray);


//BTX
  // Description:
  // Enum containing various marker styles that can be used in a plot.
  enum {
    NONE = 0,
    CROSS,
    PLUS,
    SQUARE,
    CIRCLE,
    DIAMOND
  };
//ETX

  // Description:
  // Get/set the marker style that should be used. The default is none, the enum
  // in this class is used as a parameter.
  vtkGetMacro(MarkerStyle, int);
  vtkSetMacro(MarkerStyle, int);

//BTX
protected:
  vtkMyPlotParallelCoordinates();
  ~vtkMyPlotParallelCoordinates();

  // Description:
  // Generate the requested symbol for the plot
  void GeneraterMarker(int width, bool highlight = false);

  // Description:
  // The marker style that should be used
  int MarkerStyle;
  vtkImageData* Marker;
  vtkImageData* HighlightMarker;

  // Description:
  // Selected indices coming back from outside the chart this plot is associated with.
  vtkIdTypeArray *HighlightSelection;


private:
  vtkMyPlotParallelCoordinates(const vtkMyPlotParallelCoordinates &); // Not implemented.
  void operator=(const vtkMyPlotParallelCoordinates &); // Not implemented.

//ETX
};

#endif //__vtkMyPlotParallelCoordinates_h
