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
protected:
  vtkMyPlotParallelCoordinates();
  ~vtkMyPlotParallelCoordinates();

  // Description:
  // Selected indices coming back from outside the chart this plot is associated with.
  vtkIdTypeArray *HighlightSelection;


private:
  vtkMyPlotParallelCoordinates(const vtkMyPlotParallelCoordinates &); // Not implemented.
  void operator=(const vtkMyPlotParallelCoordinates &); // Not implemented.

//ETX
};

#endif //__vtkMyPlotParallelCoordinates_h
