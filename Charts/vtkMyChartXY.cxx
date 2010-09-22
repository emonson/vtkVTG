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
vtkCxxSetObjectMacro(vtkMyChartXY, DataColumnsLink, vtkAnnotationLink);

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
  
  // Link into chart to externally control which columns are plotted
  this->DataColumnsLink = NULL;

  this->LayoutChanged = true;
}

//-----------------------------------------------------------------------------
vtkMyChartXY::~vtkMyChartXY()
{
  if (this->HighlightLink)
    {
    this->HighlightLink->Delete();
    }
  if (this->DataColumnsLink)
    {
    this->DataColumnsLink->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::Update()
{
  // Do update on superclass
  this->Superclass::Update();
  
  // Update which columns should be plotted against each other
  // Do this without reference to this->ChartPrivate since that is private
  // data in the parent class. Using public API instead to get plots...
  if (this->DataColumnsLink)
    {
    this->DataColumnsLink->Update();
    vtkSelection *selection =
        vtkSelection::SafeDownCast(this->DataColumnsLink->GetOutputDataObject(2));
    if (selection->GetNumberOfNodes())
      {
      vtkSelectionNode *node = selection->GetNode(0);
      vtkIdTypeArray *cidArray =
          vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
      // If valid, should contain two indices for the two axes
      if (cidArray->GetNumberOfTuples() >= 2)
        {
        vtkIdType xI = cidArray->GetValue(0);
        vtkIdType yI = cidArray->GetValue(1);
				// Now iterate through the plots to update columns plotted
				for (int ii=0; ii < this->GetNumberOfPlots(); ++ii)
					{
					vtkPlotPoints* plot = vtkPlotPoints::SafeDownCast(this->GetPlot(ii));
					if (plot && plot->GetVisible())
						{
						vtkTable* table = plot->GetData()->GetInput();
						// See if data to be plotted has really changed
						if ((table->GetColumn(xI) != plot->GetData()->GetInputArrayToProcess(0,table) ||
						     table->GetColumn(yI) != plot->GetData()->GetInputArrayToProcess(1,table))
						     && (xI < table->GetNumberOfColumns() && yI < table->GetNumberOfColumns()))
						  {
							plot->SetInputArray(0,table->GetColumnName(xI));
							plot->SetInputArray(1,table->GetColumnName(yI));
							this->GetAxis(0)->SetTitle(table->GetColumnName(yI));
							this->GetAxis(1)->SetTitle(table->GetColumnName(xI));
							plot->Update();
							this->Scene->SetDirty(true);
							this->RecalculatePlotBounds();			
							}
						}
					}
				}
      }
    }
  else
    {
    vtkDebugMacro("No annotation link set.");
    }

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
	
	// Make sure the axis labels have been set to column names (for vtkMyPlotPoints)
	for (int ii=0; ii < this->GetNumberOfPlots(); ++ii)
		{
		vtkMyPlotPoints* myPlot = vtkMyPlotPoints::SafeDownCast(this->GetPlot(ii));
		if (myPlot && strcmp(this->GetAxis(0)->GetTitle(),"Y Axis") == 0)
			{
			vtkTable* table = myPlot->GetData()->GetInput();
			const char* xName = myPlot->GetData()->GetInputArrayToProcess(0, table)->GetName();
			const char* yName = myPlot->GetData()->GetInputArrayToProcess(1, table)->GetName();
			this->GetAxis(0)->SetTitle(yName);
			this->GetAxis(1)->SetTitle(xName);
			}
		}

	// Set the mapping for (axis image) index to data columns to avoid plotting _ids columns
	// Look through data table column names to gather valid data column indices
	// NOTE: Looking for first visible vtkMyPlotPoints and using that table
	// Find first MyPlotPoints to know where to grab the input table
	for (vtkIdType i = 0; i < this->GetNumberOfPlots(); ++i)
		{
		vtkMyPlotPoints* plot = vtkMyPlotPoints::SafeDownCast(this->GetPlot(i));
		if (plot && plot->GetVisible())
			{
			vtkTable* table = plot->GetData()->GetInput();
			this->col_idxs.clear();
			// Build up a vector of table column indices which do not contain _ids in their name
			for (vtkIdType ii = 0; ii < table->GetNumberOfColumns(); ii++)
				{
				const char *col_name = table->GetColumnName(ii);
				if (strstr(col_name, "_ids"))
					{
					continue;
					}
				else
					{
					this->col_idxs.push_back(ii);
					}
				}
			break;
			}
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
void vtkMyChartXY::SetPlotColumnIndices(int xI, int yI)
{
	// Direct chart control method of changing plotted data
	
	// Do mapping to "valid" non_ids columns
	int X = this->col_idxs.at(xI);
	int Y = this->col_idxs.at(yI);
	
	// Looking for first visible vtkMyPlotPoints
	for (vtkIdType p = 0; p < this->GetNumberOfPlots(); ++p)
		{
		vtkMyPlotPoints* plot = vtkMyPlotPoints::SafeDownCast(this->GetPlot(p));
		if (plot && plot->GetVisible())
			{
			vtkTable* table = plot->GetData()->GetInput();
			plot->SetInput(table,X,Y);
			this->GetAxis(0)->SetTitle(table->GetColumnName(Y));
			this->GetAxis(0)->Modified();
			this->GetAxis(1)->SetTitle(table->GetColumnName(X));
			this->GetAxis(1)->Modified();
			this->Modified();
			plot->Modified();
			plot->Update();
			this->RecalculateBounds();
			this->Update();
			// Not sure how to get this to Render()...?
			// May have to do that externally after calling this routine...
			break;
			}
		}
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}
