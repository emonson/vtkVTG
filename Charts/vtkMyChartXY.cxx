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
class vtkMyChartXYPrivate
{
public:
  vtkMyChartXYPrivate()
    {
    this->Colors = vtkSmartPointer<vtkColorSeries>::New();
    this->Clip = vtkSmartPointer<vtkContextClip>::New();
    this->Borders[0] = 60;
    this->Borders[1] = 50;
    this->Borders[2] = 20;
    this->Borders[3] = 20;
    }

  vtkstd::vector<vtkPlot *> plots; // Charts can contain multiple plots of data
  vtkstd::vector<vtkContextTransform *> PlotCorners; // Stored by corner...
  vtkstd::vector<vtkAxis *> axes; // Charts can contain multiple axes
  vtkSmartPointer<vtkColorSeries> Colors; // Colors in the chart
  vtkSmartPointer<vtkContextClip> Clip; // Colors in the chart
  int Borders[4];
};


//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMyChartXY);

//-----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkMyChartXY, HighlightLink, vtkAnnotationLink);

//-----------------------------------------------------------------------------
vtkMyChartXY::vtkMyChartXY()
{
  this->TooltipShowImage = false;
  this->Tooltip = vtkTooltipImageItem::New();
  this->Tooltip->SetShowImage(this->TooltipShowImage);
  this->Tooltip->SetVisible(false);
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
      vtkstd::vector<vtkPlot*>::iterator it =
          this->ChartPrivate->plots.begin();
      for ( ; it != this->ChartPrivate->plots.end(); ++it)
        {
				if ((*it)->IsA("vtkMyPlotPoints"))
					{
					vtkMyPlotPoints* myPlot = vtkMyPlotPoints::SafeDownCast(*it);
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
  printf("Reaching proper SetTooltipInfo\n");
	this->Tooltip->SetImageIndex(seriesIndex);
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::SetTooltipShowImage(bool ShowImage)
{
  this->TooltipShowImage = ShowImage;
  this->Tooltip->SetShowImage(this->TooltipShowImage);  
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::SetTooltipImageScalingFactor(float ScalingFactor)
{
  this->Tooltip->SetScalingFactor(ScalingFactor);
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::SetTooltipImageTargetSize(int pixels)
{
  this->Tooltip->SetTargetSize(pixels);
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::SetTooltipImageStack(vtkImageData* stack)
{
  this->Tooltip->SetImageStack(stack);
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Axes: " << endl;
  for (int i = 0; i < 4; ++i)
    {
    this->ChartPrivate->axes[i]->PrintSelf(os, indent.GetNextIndent());
    }
  if (this->ChartPrivate)
    {
    os << indent << "Number of plots: " << this->ChartPrivate->plots.size()
       << endl;
    for (unsigned int i = 0; i < this->ChartPrivate->plots.size(); ++i)
      {
      os << indent << "Plot " << i << ":" << endl;
      this->ChartPrivate->plots[i]->PrintSelf(os, indent.GetNextIndent());
      }
    }

}
