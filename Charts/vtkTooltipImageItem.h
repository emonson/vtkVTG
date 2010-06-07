/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTooltipImageItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkTooltipImageItem - takes care of drawing 2D axes
//
// .SECTION Description
// The vtkTooltipImageItem is drawn in screen coordinates. It is used to display a
// tooltip on a scene, giving additional information about an element on the
// scene, such as in vtkChartXY. It takes care of ensuring that it draws itself
// within the bounds of the screen.

#ifndef __vtkTooltipImageItem_h
#define __vtkTooltipImageItem_h

#include "vtkContextItem.h"
#include "vtkVector.h" // Needed for vtkVector2f

class vtkPen;
class vtkBrush;
class vtkTextProperty;
class vtkImageData;

class VTK_CHARTS_EXPORT vtkTooltipImageItem : public vtkContextItem
{
public:
  vtkTypeMacro(vtkTooltipImageItem, vtkContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a 2D Chart object.
  static vtkTooltipImageItem *New();

  // Description:
  // Set the position of the tooltip (in pixels).
  vtkSetVector2Macro(Position, float);

  // Description:
  // Get position of the axis (in pixels).
  vtkGetVector2Macro(Position, float);

  // Description:
  // Get/set the text of the item.
  vtkSetStringMacro(Text);
  vtkGetStringMacro(Text);

  // Description:
  // Get a pointer to the vtkTextProperty object that controls the way the
  // text is rendered.
  vtkGetObjectMacro(Pen, vtkPen);

  // Description:
  // Get a pointer to the vtkPen object.
  vtkGetObjectMacro(Brush, vtkBrush);

  // Description:
  // Update the geometry of the tooltip.
  virtual void Update();

  // Description:
  // Paint event for the tooltip.
  virtual bool Paint(vtkContext2D *painter);

  // Description
  // Image to be shown if ShowImage is true
  virtual void SetTipImage(vtkImageData*);
  vtkSetMacro(ShowImage, bool);
  vtkSetMacro(ScalingFactor, float);
  
//BTX
protected:
  vtkTooltipImageItem();
  ~vtkTooltipImageItem();

  vtkVector2f PositionVector;
  float* Position;
  char* Text;
  vtkTextProperty* TextProperties;
  vtkPen* Pen;
  vtkBrush* Brush;
  
  // Description
  // Image to be shown if ShowImage is true
  vtkImageData* TipImage;
  bool ShowImage;
  float ScalingFactor;

private:
  vtkTooltipImageItem(const vtkTooltipImageItem &); // Not implemented.
  void operator=(const vtkTooltipImageItem &);   // Not implemented.
//ETX
};

#endif //__vtkTooltipImageItem_h
