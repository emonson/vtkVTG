/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPCAxis.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkPCAxis - takes care of drawing 2D axes
//
// .SECTION Description
// The vtkPCAxis is drawn in screen coordinates. It is usually one of the last
// elements of a chart to be drawn. It renders the axis label, tick marks and
// tick labels.

#ifndef __vtkPCAxis_h
#define __vtkPCAxis_h

#include "vtkAxis.h"

class vtkContext2D;

class VTK_CHARTS_EXPORT vtkPCAxis : public vtkAxis
{
public:
  vtkTypeMacro(vtkPCAxis, vtkAxis);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a 2D Chart object.
  static vtkPCAxis *New();

  // Description:
  // Paint event for the axis, called whenever the axis needs to be drawn.
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Paint event for the axis, called whenever the axis needs to be drawn.
  virtual bool PaintNoLines(vtkContext2D *painter);

//BTX
protected:
  vtkPCAxis();
  ~vtkPCAxis();

private:
  vtkPCAxis(const vtkPCAxis &); // Not implemented.
  void operator=(const vtkPCAxis &);   // Not implemented.
//ETX
};

#endif //__vtkPCAxis_h
