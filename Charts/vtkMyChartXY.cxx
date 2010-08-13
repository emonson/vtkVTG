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
#include "vtkContextMouseEvent.h"
#include "vtkTransform2D.h"
#include "vtkContextScene.h"
#include "vtkPoints2D.h"
#include "vtkVector.h"

#include "vtkPlot.h"
#include "vtkPlotBar.h"
#include "vtkPlotStacked.h"
#include "vtkPlotLine.h"
#include "vtkMyPlotPoints.h"
#include "vtkContextMapper2D.h"

#include "vtkAxis.h"
#include "vtkPlotGrid.h"
#include "vtkChartLegend.h"
#include "vtkTooltipImageItem.h"

#include "vtkTable.h"
#include "vtkAbstractArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkIdTypeArray.h"

#include "vtkImageData.h"
#include "vtkMatrix4x4.h"
#include "vtkImageReslice.h"
#include "vtkLookupTable.h"
#include "vtkColorTransferFunction.h"
#include "vtkImageMapToColors.h"
#include "vtkPointData.h"
#include "vtkVector.h"

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

// Testing
#include <time.h>
#include <string.h>
//#include <stdlib.h>

// My STL containers
#include <vtkstd/vector>
#include <vtkstd/string>

//-----------------------------------------------------------------------------
class vtkMyChartXYPrivate
{
public:
  vtkMyChartXYPrivate()
    {
    this->Colors = vtkSmartPointer<vtkColorSeries>::New();
    this->PlotCorners.resize(4);
    this->PlotTransforms.resize(4);
    this->PlotTransforms[0] = vtkSmartPointer<vtkTransform2D>::New();
    this->StackedPlotAccumulator = vtkSmartPointer<vtkDataArray>();
    this->StackParticipantsChanged.Modified();
    }

  vtkstd::vector<vtkPlot *> plots; // Charts can contain multiple plots of data
  vtkstd::vector< vtkstd::vector<vtkPlot *> > PlotCorners; // Stored by corner...
  vtkstd::vector< vtkSmartPointer<vtkTransform2D> > PlotTransforms; // Transforms
  vtkstd::vector<vtkAxis *> axes; // Charts can contain multiple axes
  vtkSmartPointer<vtkColorSeries> Colors; // Colors in the chart
  vtkSmartPointer<vtkDataArray> StackedPlotAccumulator;
  vtkTimeStamp StackParticipantsChanged;   // Plot added or plot visibility changed
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

//-----------------------------------------------------------------------------
bool vtkMyChartXY::LocatePointInPlots(const vtkContextMouseEvent &mouse)
{
  size_t n = this->ChartPrivate->plots.size();
  if (mouse.ScreenPos[0] > this->Point1[0] &&
      mouse.ScreenPos[0] < this->Point2[0] &&
      mouse.ScreenPos[1] > this->Point1[1] &&
      mouse.ScreenPos[1] < this->Point2[1] && n)
    {
    // Iterate through each corner, and check for a nearby point
    for (int i = 0; i < 4; ++i)
      {
      if (this->ChartPrivate->PlotCorners[i].size())
        {
        vtkVector2f position;
        vtkTransform2D* transform = this->ChartPrivate->PlotTransforms[i];
        transform->InverseTransformPoints(mouse.Pos.GetData(),
                                          position.GetData(), 1);
        // Use a tolerance of +/- 5 pixels
        vtkVector2f tolerance(5*(1.0/transform->GetMatrix()->GetElement(0, 0)),
                              5*(1.0/transform->GetMatrix()->GetElement(1, 1)));
        // Iterate through the visible plots and return on the first hit
        for (int j = static_cast<int>(this->ChartPrivate->PlotCorners[i].size()-1);
             j >= 0; --j)
          {
          vtkPlot* plot = this->ChartPrivate->PlotCorners[i][j];
          if (plot->GetVisible())
            {
            if (plot->IsA("vtkMyPlotPoints"))
              {
							vtkMyPlotPoints* myPlot = vtkMyPlotPoints::SafeDownCast(plot);
              vtkVector2f plotPos;
							int found_ind = myPlot->GetNearestPoint(position, tolerance, &plotPos);
							if (found_ind >= 0)
								{
								// We found a point, set up the tooltip and return
								vtksys_ios::ostringstream ostr;
								ostr << myPlot->GetLabel() << ": " << found_ind;
								this->Tooltip->SetText(ostr.str().c_str());
								this->Tooltip->SetPosition(mouse.ScreenPos[0]+8, mouse.ScreenPos[1]+6);
								
								// Testing random image from stack
// 								if (this->TooltipShowImage)
// 									{
// 									int num_images = myPlot->GetNumberOfImages();
// 									if (num_images > 0)
// 										{
// 										// int random_index = rand() % num_images;
// 										// this->Tooltip->SetTipImage(myPlot->GetImageAtIndex(random_index));
// 										this->Tooltip->SetTipImage(myPlot->GetImageAtIndex(found_ind));
// 										}
// 									}
								return true;
								}
							}
					  else
					    {
              vtkVector2f plotPos;
							int found = plot->GetNearestPoint(position, tolerance, &plotPos);
							if (found >= 0)
								{
								// We found a point, set up the tooltip and return
								vtksys_ios::ostringstream ostr;
								ostr << plot->GetLabel() << ": " << plotPos.X() << ", " << plotPos.Y();
								this->Tooltip->SetText(ostr.str().c_str());
								this->Tooltip->SetPosition(mouse.ScreenPos[0]+2, mouse.ScreenPos[1]+2);
								return true;
								}
              }
            }
          }
        }
      }
    }
  return false;
}


//-----------------------------------------------------------------------------
void vtkMyChartXY::SetTooltipShowImage(bool ShowImage)
{
  this->TooltipShowImage = ShowImage;
  this->Tooltip->SetShowImage(this->TooltipShowImage);
  
  // Assuming for now that an image stack has been set in one of the vtkMyPlotPoints
  // We know the plot will only ever be in one of the corners
  for (int i = 0; i < 4; ++i)
    {
    vtkstd::vector<vtkPlot*>::iterator it =
        this->ChartPrivate->PlotCorners[i].begin();
    for ( ; it !=this->ChartPrivate->PlotCorners[i].end(); ++it)
      {
//       if ((*it)->IsA("vtkMyPlotPoints"))
//         {
//         vtkMyPlotPoints* myPlot = vtkMyPlotPoints::SafeDownCast((*it));
//         if ( myPlot->GetNumberOfImages() > 0)
//           {
//           this->Tooltip->SetTipImage(myPlot->GetImageAtIndex(0));
//           }
//         }
      }
    }
  
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
