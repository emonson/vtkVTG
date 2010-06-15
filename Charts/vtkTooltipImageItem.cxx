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
  this->ScalingFactor = 1.0;
  this->ShowImage = false;
  this->ImageWidth = 0.0;
  this->ImageHeight = 0.0;
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

  if (!this->ShowImage)
    {
		painter->ComputeStringBounds(this->Text, bounds[0].GetData());
		bounds[0] = vtkVector2f(this->PositionVector.X()-5,
														this->PositionVector.Y()-3);
		
		bounds[1].Set(bounds[1].X()+10, bounds[1].Y()+10);
		// Pull the tooltip back in if it will go off the edge of the screen.
		if (int(bounds[0].X()+bounds[1].X()) >= this->Scene->GetViewWidth())
			{
			bounds[0].SetX(this->Scene->GetViewWidth()-bounds[1].X());
			}
    }
  // For now just recompute if image instead of text
  if (this->ShowImage && this->TipImage)
    {
		bounds[0] = vtkVector2f(this->PositionVector.X()-3,
														this->PositionVector.Y()-2);
    bounds[1].Set(this->ImageWidth, this->ImageHeight);
		if (int(bounds[0].X()+bounds[1].X()) >= this->Scene->GetViewWidth())
			{
			bounds[0].SetX(this->Scene->GetViewWidth()-bounds[1].X());
			}
		if (int(bounds[0].Y()+bounds[1].Y()) >= this->Scene->GetViewHeight())
			{
			bounds[0].SetY(this->Scene->GetViewHeight()-bounds[1].Y());
			}
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
    painter->DrawImage(bounds[0].X(), bounds[0].Y(), this->ScalingFactor, this->TipImage);
    }

  return true;
}

//-----------------------------------------------------------------------------
void vtkTooltipImageItem::SetTipImage(vtkImageData* image)
{
	this->TipImage = image;
	this->TipImage->UpdateInformation();
	int extent[6];

	this->TipImage->GetWholeExtent(extent);
	
	// Z should be zero...
	this->ImageWidth = this->ScalingFactor*(float)extent[1];
	this->ImageHeight = this->ScalingFactor*(float)extent[3];
}

//-----------------------------------------------------------------------------
void vtkTooltipImageItem::SetScalingFactor(float factor)
{
	if (!this->TipImage)
	  {
	  return;
	  }
	
	this->TipImage->UpdateInformation();
	int extent[6];
	float scaling;

	this->TipImage->GetWholeExtent(extent);
	
	// Z should be zero...
	this->ScalingFactor = factor;
	this->ImageWidth = factor*(float)extent[1];
	this->ImageHeight = factor*(float)extent[3];
}

//-----------------------------------------------------------------------------
void vtkTooltipImageItem::SetTargetSize(int pixels)
{
	if (!this->TipImage)
	  {
	  return;
	  }
	
	this->TipImage->UpdateInformation();
	int extent[6];
	float scaling;

	this->TipImage->GetWholeExtent(extent);
	
	// Z should be zero...
	int maxdim = (extent[1] > extent[3]) ? extent[1] : extent[3];
	scaling = (float)pixels/(float)maxdim;
	
	// Z should be zero...
	this->ScalingFactor = scaling;
	this->ImageWidth = scaling*(float)extent[1];
	this->ImageHeight = scaling*(float)extent[3];
}

//-----------------------------------------------------------------------------
void vtkTooltipImageItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
