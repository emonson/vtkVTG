/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMyChartXY.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMyChartXY.h"

#include "vtkContext2D.h"
#include "vtkPen.h"
#include "vtkBrush.h"
#include "vtkColorSeries.h"
#include "vtkContextDevice2D.h"
#include "vtkTransform2D.h"
#include "vtkContextScene.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextTransform.h"
#include "vtkContextClip.h"
#include "vtkPoints2D.h"
#include "vtkVector.h"

#include "vtkPlot.h"
#include "vtkPlotBar.h"
#include "vtkPlotStacked.h"
#include "vtkPlotLine.h"
#include "vtkPlotPoints.h"
#include "vtkContextMapper2D.h"

#include "vtkAxis.h"
#include "vtkPlotGrid.h"
#include "vtkChartLegend.h"
#include "vtkTooltipItem.h"

#include "vtkTable.h"
#include "vtkAbstractArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkIdTypeArray.h"

#include "vtkAnnotationLink.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"

#include "vtkObjectFactory.h"
#include "vtkCommand.h"

#include "vtkStdString.h"
#include "vtkTextProperty.h"

#include "vtksys/ios/sstream"
#include "vtkDataArray.h"

// Custom
#include "vtkMyPlotPoints.h"
#include "vtkTooltipImageItem.h"

// My STL containers
#include <vtkstd/vector>


//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMyChartXY);

//-----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkMyChartXY, HighlightLink, vtkAnnotationLink);

//-----------------------------------------------------------------------------
vtkMyChartXY::vtkMyChartXY()
{
  this->TooltipShowImage = false;
  this->Tooltip = vtkTooltipImageItem::New();
  vtkTooltipImageItem::SafeDownCast(this->Tooltip)->SetShowImage(this->TooltipShowImage);
  vtkTooltipImageItem::SafeDownCast(this->Tooltip)->SetVisible(false);
  this->AddItem(this->Tooltip);
  this->Tooltip->Delete();

  // Link back into chart to highlight selections made in other plots
  this->HighlightLink = NULL;

  this->LayoutChanged = true;
}

//-----------------------------------------------------------------------------
vtkMyChartXY::~vtkMyChartXY()
{
  if (this->HighlightLink)
    {
    this->HighlightLink->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::Update()
{
  // Do update on superclass first
  vtkChartXY::Update();
  
  // Update the selections if necessary.
  // Do this without reference to this->ChartPrivate since that is private
  // data in the parent class. Using public API instead to get plots...
  if (this->HighlightLink)
    {
    this->HighlightLink->Update();
    vtkSelection *selection =
        vtkSelection::SafeDownCast(this->HighlightLink->GetOutputDataObject(2));
    if (selection->GetNumberOfNodes())
      {
      vtkSelectionNode *node = selection->GetNode(0);
      vtkIdTypeArray *hidArray =
          vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
      // Now iterate through the plots to update selection data
      for (int ii=0; ii < this->GetNumberOfPlots(); ++ii)
        {
        vtkPlot* pp = this->GetPlot(ii);
				if (pp->IsA("vtkMyPlotPoints"))
					{
					vtkMyPlotPoints* myPlot = vtkMyPlotPoints::SafeDownCast(pp);
        	myPlot->SetHighlightSelection(hidArray);
        	}
        }
      }
    }
  else
    {
    vtkDebugMacro("No annotation link set.");
    }

}

void vtkMyChartXY::SetTooltipInfo(const vtkContextMouseEvent& mouse, vtkVector2f plotPos, 
                                int seriesIndex, vtkPlot* plot)
{
	this->Tooltip->SetPosition(mouse.ScreenPos[0]+8, mouse.ScreenPos[1]+6);
	vtkTooltipImageItem::SafeDownCast(this->Tooltip)->SetImageIndex(seriesIndex);
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::SetTooltipShowImage(bool ShowImage)
{
  this->TooltipShowImage = ShowImage;
  vtkTooltipImageItem::SafeDownCast(this->Tooltip)->SetShowImage(this->TooltipShowImage);  
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::SetTooltipImageScalingFactor(float ScalingFactor)
{
  vtkTooltipImageItem::SafeDownCast(this->Tooltip)->SetScalingFactor(ScalingFactor);
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::SetTooltipImageTargetSize(int pixels)
{
  vtkTooltipImageItem::SafeDownCast(this->Tooltip)->SetTargetSize(pixels);
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::SetTooltipImageStack(vtkImageData* stack)
{
  vtkTooltipImageItem::SafeDownCast(this->Tooltip)->SetImageStack(stack);
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}
