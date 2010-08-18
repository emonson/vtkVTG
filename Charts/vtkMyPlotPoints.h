/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMyPlotPoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkMyPlotPoints - Class for drawing an XY plot given two columns from a
// vtkTable.
//
// .SECTION Description
//

#ifndef __vtkMyPlotPoints_h
#define __vtkMyPlotPoints_h

#include "vtkPlotPoints.h"
#include "vtkSmartPointer.h"
// #include "vtkVector.h"

class vtkContext2D;
class vtkTable;
class vtkPoints2D;
class vtkStdString;
class vtkImageData;
class vtkMatrix4x4;
class vtkImageReslice;
class vtkLookupTable;
class vtkImageMapToColors;
class vtkVector3f;

class VTK_CHARTS_EXPORT vtkMyPlotPoints : public vtkPlotPoints
{
public:
  vtkTypeMacro(vtkMyPlotPoints, vtkPlotPoints);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a 2D Chart object.
  static vtkMyPlotPoints *New();

  // Description:
  // Perform any updates to the item that may be necessary before rendering.
  // The scene should take care of calling this on all items before their
  // Paint function is invoked.
  virtual void Update();

  // Description:
  // Paint event for the XY plot, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Function to query a plot for the nearest point to the specified coordinate.
  // This version returns the index of the point as a float in the .Z() vector3f coord.
  virtual int GetNearestPoint(const vtkVector2f& point,
                               const vtkVector2f& tolerance,
                               vtkVector2f* location);

  // Description:
  // Set the list of ids for highlighting (back from outside plot).
  virtual void SetHighlightSelection(vtkIdTypeArray *id);
  vtkGetObjectMacro(HighlightSelection, vtkIdTypeArray);

protected:
  vtkMyPlotPoints();
  ~vtkMyPlotPoints();

  // Description:
  // Update the table cache.
  bool UpdateTableCache(vtkTable *table);

  // Description:
  // Sorted points, used when searching for the nearest point.
  class VectorPIMPL3;
  VectorPIMPL3* Sorted3;

  // Description:
  // Selected indices coming back from outside the chart this plot is associated with.
  vtkIdTypeArray *HighlightSelection;

private:
  vtkMyPlotPoints(const vtkMyPlotPoints &); // Not implemented.
  void operator=(const vtkMyPlotPoints &); // Not implemented.

};

#endif //__vtkMyPlotPoints_h
