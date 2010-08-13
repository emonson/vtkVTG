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
#include "vtkMatrix4x4.h"
#include "vtkImageReslice.h"
#include "vtkLookupTable.h"
#include "vtkImageMapToColors.h"
#include "vtkPointData.h"

#include "vtkStdString.h"
#include "vtksys/ios/sstream"

#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkTooltipImageItem);

//-----------------------------------------------------------------------------
vtkTooltipImageItem::vtkTooltipImageItem()
{
  this->TipImage = NULL;
  this->ScalingFactor = 1.0;
  this->ShowImage = false;
  this->ImageWidth = 0.0;
  this->ImageHeight = 0.0;

  this->ImageStack = NULL;
  this->NumImages = 0;

  // ImageSlicing for TooltipImageItem
  // Always slicing in the Z direction
  static double axialElements[16] = {
           1, 0, 0, 0,
           0, 1, 0, 0,
           0, 0, 1, 0,
           0, 0, 0, 1 };

  // Set the slice orientation
  this->resliceAxes = vtkSmartPointer<vtkMatrix4x4>::New();
  this->resliceAxes->DeepCopy(axialElements);

  // Extract a slice in the desired orientation
  this->reslice = vtkSmartPointer<vtkImageReslice>::New();
  this->reslice->SetOutputDimensionality(2);
  this->reslice->SetResliceAxes(this->resliceAxes);
  this->reslice->SetInterpolationModeToNearestNeighbor();
  // Need to set the Input when ImageStack is assigned

  // Create a greyscale lookup table
  this->lut = vtkSmartPointer<vtkLookupTable>::New();
  this->lut->SetRange(0, 1); 			// image intensity range
  this->lut->SetValueRange(0.0, 1.0); 		// from black to white
  this->lut->SetSaturationRange(0.0, 0.0); 	// no color saturation
  this->lut->SetRampToLinear();
  this->lut->Build();

  // Map the image through the lookup table
  this->color = vtkSmartPointer<vtkImageMapToColors>::New();
  this->color->SetLookupTable(this->lut);
  this->color->SetInputConnection(this->reslice->GetOutputPort());
}

//-----------------------------------------------------------------------------
vtkTooltipImageItem::~vtkTooltipImageItem()
{

}

//-----------------------------------------------------------------------------
bool vtkTooltipImageItem::Paint(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called in vtkTooltipImageItem.");
  
  if (!this->Visible)
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
void vtkTooltipImageItem::SetImageIndex(int imageId)
{
	this->TipImage = this->GetImageAtIndex(imageId);
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
void vtkTooltipImageItem::SetImageStack(vtkImageData* stack)
{
  this->ImageStack = stack;
  this->ImageStack->UpdateInformation();
  this->reslice->SetInput(this->ImageStack);
  this->reslice->Modified();
  this->lut->SetRange(this->ImageStack->GetPointData()->GetScalars()->GetRange());
  this->lut->Modified();
  int extent[6];
  this->ImageStack->UpdateInformation();
  this->ImageStack->GetWholeExtent(extent);
  this->NumImages = (extent[5]-extent[4]+1);
  
  // Default to 0 index image so there's something in TipImage
  this->SetImageIndex(0);
}

//-----------------------------------------------------------------------------
vtkImageData* vtkTooltipImageItem::GetImageAtIndex(int imageId)
{
  if (this->ImageStack)
    {
    // Calculate the center of the volume
    this->ImageStack->UpdateInformation();
    int extent[6];
    double spacing[3];
    double origin[3];
    this->ImageStack->GetWholeExtent(extent);
    this->ImageStack->GetSpacing(spacing);
    this->ImageStack->GetOrigin(origin);

    double center[3];
    center[0] = origin[0] + spacing[0] * 0.5 * (extent[0] + extent[1]); 
    center[1] = origin[1] + spacing[1] * 0.5 * (extent[2] + extent[3]); 
    center[2] = origin[2] + spacing[2] * 0.5 * (extent[4] + extent[5]); 

    // Set the point through which to slice
    vtkMatrix4x4 *resliceAxes = reslice->GetResliceAxes();
	  double zpos = origin[2] + spacing[2]*(extent[4]+static_cast<float>(imageId));
    resliceAxes->SetElement(0, 3, center[0]);
    resliceAxes->SetElement(1, 3, center[1]);
    resliceAxes->SetElement(2, 3, zpos);
    this->reslice->Modified();
    
    this->color->Update();
    
    return this->color->GetOutput();
    }
  else
    {
    return NULL;
    }
}

//-----------------------------------------------------------------------------
int vtkTooltipImageItem::GetNumberOfImages()
{
  if (this->ImageStack)
    {
    return this->NumImages;
    }
  else
    {
    return 0;
    }
}

//-----------------------------------------------------------------------------
void vtkTooltipImageItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
