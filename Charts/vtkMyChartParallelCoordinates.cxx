/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMyChartParallelCoordinates.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMyChartParallelCoordinates.h"
#include "vtkMyPlotParallelCoordinates.h"

#include "vtkContext2D.h"
#include "vtkBrush.h"
#include "vtkPen.h"
#include "vtkContextScene.h"
#include "vtkContextMouseEvent.h"
#include "vtkTextProperty.h"
#include "vtkAxis.h"
#include "vtkAxis.h"
#include "vtkContextMapper2D.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkDataArray.h"
#include "vtkIdTypeArray.h"
#include "vtkTransform2D.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"
#include "vtkAnnotationLink.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkStringArray.h"

#include "vtkstd/vector"
#include "vtkstd/algorithm"

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMyChartParallelCoordinates);

//-----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkMyChartParallelCoordinates, HighlightLink, vtkAnnotationLink);

//-----------------------------------------------------------------------------
vtkMyChartParallelCoordinates::vtkMyChartParallelCoordinates()
{
  // Replace original 
  vtkSmartPointer<vtkMyPlotParallelCoordinates> newPlot = vtkSmartPointer<vtkMyPlotParallelCoordinates>::New();
  this->SetPlot(newPlot);

  this->DrawSets = false;
  this->NumPerSet = 1;
  this->CurrentScale = 0;
  this->XYcurrentX = 0;
  this->XYcurrentY = 0;
  // Link back into chart to highlight selections made in other plots
  this->HighlightLink = NULL;
  this->HighlightSelection = vtkIdTypeArray::New();
  vtkMyPlotParallelCoordinates::SafeDownCast(this->Storage->Plot)->SetHighlightSelection(this->HighlightSelection);
  this->ScaleDims.clear();
}

//-----------------------------------------------------------------------------
vtkMyChartParallelCoordinates::~vtkMyChartParallelCoordinates()
{
  if (this->HighlightLink)
    {
    this->HighlightLink->Delete();
    }
}


//-----------------------------------------------------------------------------
bool vtkMyChartParallelCoordinates::Paint(vtkContext2D *painter)
{
  if (this->GetScene()->GetViewWidth() == 0 ||
      this->GetScene()->GetViewHeight() == 0 ||
      !this->Visible || !this->GetPlot()->GetVisible() ||
      this->VisibleColumns->GetNumberOfTuples() < 2)
    {
    // The geometry of the chart must be valid before anything can be drawn
    return false;
    }

  this->Update();
  this->UpdateGeometry();

  // Handle selections
  vtkIdTypeArray *idArray = 0;
  unsigned long plotMTime = this->GetPlot()->GetMTime();
  if (this->AnnotationLink)
    {
    vtkSelection *selection = this->AnnotationLink->GetCurrentSelection();
    if (selection->GetNumberOfNodes() &&
        this->AnnotationLink->GetMTime() > plotMTime)
      {
      vtkSelectionNode *node = selection->GetNode(0);
      idArray = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
      this->GetPlot()->SetSelection(idArray);
      }
    }
  else
    {
    vtkDebugMacro("No annotation link set.");
    }

  // Handle highlight selections back into this chart
  idArray = 0;
  if (this->HighlightLink)
    {
    vtkSelection *selection = this->HighlightLink->GetCurrentSelection();
    if (selection->GetNumberOfNodes() &&
        this->HighlightLink->GetMTime() > plotMTime)
      {
      vtkSelectionNode *node = selection->GetNode(0);
      idArray = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
      this->GetPlot()->SetHighlightSelection(idArray);
      }
    }
  else
    {
    vtkDebugMacro("No highlight annotation link set.");
    }

  // Prepare information on axis indices for drawing boxes (bounds)
  int num_groups = this->ScaleDims.size();
  vtkstd::vector<int> group_starts(num_groups);
  vtkstd::vector<int> group_ends(num_groups);

  if (num_groups > 0)
    {
    group_starts[0] = 0;
    group_ends[0] = this->ScaleDims[0] - 1;
    for (int ii=1; ii < num_groups; ++ii)
      {
      group_starts[ii] = group_starts[ii-1] + this->ScaleDims[ii-1];
      group_ends[ii] = group_ends[ii-1] + this->ScaleDims[ii];
      }
    }

  // Draw set rectangles if desired
  if (this->DrawSets)
    {
    int oldLineType = painter->GetPen()->GetLineType();
    
    // Main sets boxes
    painter->GetPen()->SetLineType(1);
    painter->GetPen()->SetColor(0,0,0);
    painter->GetPen()->SetWidth(1.0);
    for (int i = 0; i < this->ScaleDims.size(); ++i)
      {
      int idx0 = group_starts.at(i);
      int idx1 = group_ends.at(i);
      vtkAxis* axis0 = this->GetAxis(idx0);
      vtkAxis* axis1 = this->GetAxis(idx1);
      if (this->GetPlot()->GetScalarVisibility())
        {
        // Use gray box background for colored lines
        painter->GetBrush()->SetColor(150, 150, 150, 20);
        }
      else
        {
        // yellow-gold otherwise
        painter->GetBrush()->SetColor(254, 209, 0, 20);
        }
      painter->DrawRect(axis0->GetPoint1()[0],
                        this->Point1[1],
                        axis1->GetPoint1()[0]-axis0->GetPoint1()[0],
                        this->Point2[1]-this->Point1[1]);
      // Extra set box for current scale
      if (i == this->CurrentScale)
        {
        if (this->GetPlot()->GetScalarVisibility())
          {
          // Use gray box background for colored lines
          painter->GetBrush()->SetColor(150, 150, 150, 60);
          }
        else
          {
          // yellow-gold otherwise
          painter->GetBrush()->SetColor(254, 209, 0, 60);
          }
        painter->DrawRect(axis0->GetPoint1()[0], 
                          this->Point1[1],
                          axis1->GetPoint1()[0]-axis0->GetPoint1()[0], 
                          this->Point2[1]-this->Point1[1]);
        // And annotation for externally plotted X and Y axes
        if ((this->XYcurrentX >= 0) && (this->XYcurrentY >= 0))
					{
					vtkAxis* axisX = this->GetAxis(idx0+this->XYcurrentX);
					vtkAxis* axisY = this->GetAxis(idx0+this->XYcurrentY);
					painter->GetBrush()->SetColor(200, 200, 200, 100);
					
					painter->DrawLine(axisX->GetPoint1()[0]-3, 
														this->Point2[1]+6,
														axisX->GetPoint1()[0]+3,
														this->Point2[1]+6);
					painter->DrawEllipse(axisX->GetPoint1()[0], 
														this->Point2[1]+6,
														3,3);
					
					painter->DrawLine(axisY->GetPoint1()[0], 
														this->Point2[1]+3,
														axisY->GetPoint1()[0],
														this->Point2[1]+9);
					painter->DrawEllipse(axisY->GetPoint1()[0], 
														this->Point2[1]+6,
														3,3);
          }
        }
      }
    painter->GetPen()->SetLineType(oldLineType);
    }
    
	// Paint axes, but only if there are not too many of them
	if (this->GetVisibleColumns()->GetNumberOfValues() < 60)
	  {
	  for (int i = 0; i < this->GetNumberOfAxes(); i++)
			{
			vtkAxis* axis = this->GetAxis(i);
			axis->GetPen()->SetColor(200,200,200);
			axis->Paint(painter);
			axis->GetPen()->SetColor(0,0,0);
			}
    }
	else
	  {
	  for (int i = 0; i < this->GetNumberOfAxes(); i++)
			{
			vtkAxis* axis = this->GetAxis(i);
			unsigned char oldOpacity = axis->GetPen()->GetOpacity();
			axis->GetPen()->SetOpacity(0);
			axis->Paint(painter);
			axis->GetPen()->SetOpacity(oldOpacity);
			}
    }

  // Paint the actual lines of the plot
  painter->PushMatrix();
  painter->SetTransform(this->Storage->Transform);
  this->GetPlot()->Paint(painter);
  painter->PopMatrix();

  // If there is a selected axis, draw the highlight
  if (this->Storage->CurrentAxis >= 0)
    {
    painter->GetBrush()->SetColor(200, 200, 200, 150);
    painter->GetPen()->SetLineType(0);
    vtkAxis* axis = this->Storage->Axes[this->Storage->CurrentAxis];
    painter->DrawRect(axis->GetPoint1()[0]-3, this->Point1[1],
                      6, this->Point2[1]-this->Point1[1]);
    }

  // Now draw our active selections
  for (size_t i = 0; i < this->Storage->AxesSelections.size(); ++i)
    {
    vtkVector<float, 2> &range = this->Storage->AxesSelections[i];
    if (range[0] != range[1])
      {
      painter->GetBrush()->SetColor(200, 20, 20, 220);
      painter->GetPen()->SetLineType(0);
      float x = this->Storage->Axes[i]->GetPoint1()[0] - 3;
      float y = range[0];
      y *= this->Storage->Transform->GetMatrix()->GetElement(1, 1);
      y += this->Storage->Transform->GetMatrix()->GetElement(1, 2);
      float height = range[1] - range[0];
      height *= this->Storage->Transform->GetMatrix()->GetElement(1, 1);

      painter->DrawRect(x, y, 6, height);
      }
    }

  // Semi-transparent box over non-valid scales
  if (this->DrawSets && this->CurrentScale < (num_groups-1))
    {
    int oldLineType = painter->GetPen()->GetLineType();

    painter->GetBrush()->SetColor(254, 254, 254, 150);
    painter->GetPen()->SetLineType(0);
    // painter->GetPen()->SetOpacity(0.0);
    int opaquePadding = 4;  // extra padding so axes and points themselves are covered
    int idx0 = group_starts.at(this->CurrentScale+1);
    int idx1 = group_ends.back();
    vtkAxis* axis0 = this->Storage->Axes.at(idx0);
    vtkAxis* axis1 = this->Storage->Axes.at(idx1);
    painter->DrawRect(axis0->GetPoint1()[0]-opaquePadding,
                      this->Point1[1]-opaquePadding,
                      axis1->GetPoint1()[0]-axis0->GetPoint1()[0]+(2*opaquePadding),
                      this->Point2[1]-this->Point1[1]+(2*opaquePadding));
    
    painter->GetPen()->SetLineType(oldLineType);
    }
    
  return true;
}

//-----------------------------------------------------------------------------
void vtkMyChartParallelCoordinates::SetNumberOfScales(int num_scales)
{
  this->ScaleDims.clear();
  this->ScaleDims.resize(num_scales);
}

//-----------------------------------------------------------------------------
void vtkMyChartParallelCoordinates::SetScaleDim(vtkIdType index, int dim_size)
{
  this->ScaleDims.at(index) = dim_size;
}

//-----------------------------------------------------------------------------
void vtkMyChartParallelCoordinates::SetAllColumnsInvisible()
{
  // Setting all columns invisible so paint won't spit up if it gets new
  // data that has less columns than chart visible columns
  this->VisibleColumns->SetNumberOfTuples(0);
  // Also need to reset CurrentAxis so it won't be greater than
  // number of axes
  this->Storage->CurrentAxis = -1;
  this->Modified();
  this->Update();
  return;
}

//-----------------------------------------------------------------------------
void vtkMyChartParallelCoordinates::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
