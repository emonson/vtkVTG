/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTooltipImageItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTooltipImageItem.h"

#include "vtkContext2D.h"
#include "vtkContextScene.h"
#include "vtkPen.h"
#include "vtkBrush.h"
#include "vtkTextProperty.h"
#include "vtkImageData.h"

#include "vtkStdString.h"
#include "vtksys/ios/sstream"

#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkTooltipImageItem);

//-----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkTooltipImageItem, TipImage, vtkImageData);

//-----------------------------------------------------------------------------
vtkTooltipImageItem::vtkTooltipImageItem()
{
  this->Position = this->PositionVector.GetData();
  this->Text = NULL;
  this->TextProperties = vtkTextProperty::New();
  this->TextProperties->SetVerticalJustificationToBottom();
  this->TextProperties->SetJustificationToLeft();
  this->TextProperties->SetColor(0.0, 0.0, 0.0);
  this->Pen = vtkPen::New();
  this->Pen->SetColor(0, 0, 0);
  this->Pen->SetWidth(1.0);
  this->Brush = vtkBrush::New();
  this->Brush->SetColor(242, 242, 242);
  this->TipImage = NULL;
}

//-----------------------------------------------------------------------------
vtkTooltipImageItem::~vtkTooltipImageItem()
{

  this->SetText(NULL);
  this->Pen->Delete();
  this->Brush->Delete();
  this->TextProperties->Delete();
}

//-----------------------------------------------------------------------------
void vtkTooltipImageItem::Update()
{

}

//-----------------------------------------------------------------------------
bool vtkTooltipImageItem::Paint(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called in vtkTooltipImageItem.");

  if (!this->Visible || !this->Text)
    {
    return false;
    }

  painter->ApplyPen(this->Pen);
  painter->ApplyBrush(this->Brush);
  painter->ApplyTextProp(this->TextProperties);

  // Compute the bounds, then make a few adjustments to the size we will use
  vtkVector2f bounds[2];
  // TODO: Still need to set better bounds for TipImage if showing that...
  
  painter->ComputeStringBounds(this->Text, bounds[0].GetData());
  bounds[0] = vtkVector2f(this->PositionVector.X()-5,
                          this->PositionVector.Y()-3);
  bounds[1].Set(bounds[1].X()+10, bounds[1].Y()+10);
  // Pull the tooltip back in if it will go off the edge of the screen.
  if (int(bounds[0].X()+bounds[1].X()) >= this->Scene->GetViewWidth())
    {
    bounds[0].SetX(this->Scene->GetViewWidth()-bounds[1].X());
    }
  
  if (!this->ShowImage)
    {
		// Draw a rectangle as background, and then center our text in there
		painter->DrawRect(bounds[0].X(), bounds[0].Y(), bounds[1].X(), bounds[1].Y());
    painter->DrawString(bounds[0].X()+5, bounds[0].Y()+3, this->Text);
    }
  else
    {
    // painter->DrawString(bounds[0].X()+5, bounds[0].Y()+3, this->Text);
    // painter->DrawImage(bounds[0].X(), bounds[0].Y()+20, this->TipImage);
    painter->DrawImage(bounds[0].X()+2, bounds[0].Y()+1, this->TipImage);
    }

  return true;
}

//-----------------------------------------------------------------------------
void vtkTooltipImageItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
