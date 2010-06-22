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
class vtkAxisImagePrivate
{
public:
  vtkAxisImagePrivate()
    {
    this->Point1[0] = 0;
    this->Point1[1] = 0;
    this->Point2[0] = 0;
    this->Point2[1] = 0;
    this->Image = vtkSmartPointer<vtkImageData>::New();
    this->ColumnIndex = -1;
    }

  int Point1[2];
  int Point2[2];
  vtkSmartPointer<vtkImageData> Image;
  vtkIdType ColumnIndex;
};

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
    this->aiScalingFactor = 1.0;
    this->aiWidth = 0;
    this->aiHeight = 0;
    this->aiOrientation = vtkMyChartXY::VERTICAL;	// set default here
    this->aiGap = 10;			// Set default here
    this->aiXSpace = 40;	// Set default here : used if axis images vertical
    this->aiYSpace = 60;	// Set default here : used if axis images horizontal
    this->currentXai = 0;
    this->currentYai = 1;
    }

  vtkstd::vector<vtkPlot *> plots; // Charts can contain multiple plots of data
  vtkstd::vector< vtkstd::vector<vtkPlot *> > PlotCorners; // Stored by corner...
  vtkstd::vector< vtkSmartPointer<vtkTransform2D> > PlotTransforms; // Transforms
  vtkstd::vector<vtkAxis *> axes; // Charts can contain multiple axes
  vtkSmartPointer<vtkColorSeries> Colors; // Colors in the chart
  vtkSmartPointer<vtkDataArray> StackedPlotAccumulator;
  vtkTimeStamp StackParticipantsChanged;   // Plot added or plot visibility changed
  
  vtkstd::vector<vtkAxisImagePrivate *> axisImages;
  float aiScalingFactor;
  int aiWidth, aiHeight, aiGap, aiXSpace, aiYSpace;
  int currentXai, currentYai;
  int aiOrientation;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMyChartXY);

//-----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkMyChartXY, HighlightLink, vtkAnnotationLink);

//-----------------------------------------------------------------------------
vtkMyChartXY::vtkMyChartXY()
{
  this->Legend = vtkChartLegend::New();
  this->Legend->SetChart(this);
  this->ChartPrivate = new vtkMyChartXYPrivate;
  for (int i = 0; i < 4; ++i)
    {
    this->ChartPrivate->axes.push_back(vtkAxis::New());
    // By default just show the left and bottom axes
    this->ChartPrivate->axes.back()->SetVisible(i < 2 ? true : false);
    }
  this->ChartPrivate->axes[vtkAxis::LEFT]->SetPosition(vtkAxis::LEFT);
  this->ChartPrivate->axes[vtkAxis::BOTTOM]->SetPosition(vtkAxis::BOTTOM);
  this->ChartPrivate->axes[vtkAxis::RIGHT]->SetPosition(vtkAxis::RIGHT);
  this->ChartPrivate->axes[vtkAxis::TOP]->SetPosition(vtkAxis::TOP);

  // Set up the x and y axes - should be congigured based on data
  this->ChartPrivate->axes[vtkAxis::LEFT]->SetTitle("Y Axis");
  this->ChartPrivate->axes[vtkAxis::BOTTOM]->SetTitle("X Axis");

  this->Grid = vtkPlotGrid::New();
  this->Grid->SetXAxis(this->ChartPrivate->axes[1]);
  this->Grid->SetYAxis(this->ChartPrivate->axes[0]);

  this->PlotTransformValid = false;

  this->BoxOrigin[0] = this->BoxOrigin[1] = 0.0f;
  this->BoxGeometry[0] = this->BoxGeometry[1] = 0.0f;
  this->DrawBox = false;
  this->DrawNearestPoint = false;
  this->DrawAxesAtOrigin = false;
  this->BarWidthFraction = 0.8f;
  
  this->TooltipShowImage = false;
  this->Tooltip = vtkTooltipImageItem::New();
  this->Tooltip->SetShowImage(this->TooltipShowImage);
  this->Tooltip->SetVisible(false);

  // Link back into chart to highlight selections made in other plots
  this->HighlightLink = NULL;

  this->AxisImageStack = NULL;
  this->NumImages = 0;
  this->CenterImage = NULL;

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
  // Need to set the Input when AxisImageStack is assigned

  // Create a blue-white-red diverging lookup table
  this->lut = vtkSmartPointer<vtkLookupTable>::New();
  int lutNum = 256;
  this->lut->SetNumberOfTableValues(lutNum);
  this->lut->Build();
  
  vtkSmartPointer<vtkColorTransferFunction> ctf = 
  		vtkSmartPointer<vtkColorTransferFunction>::New();
	ctf->SetColorSpaceToDiverging();
// 	float c_lo[3] = {59.0/255.0, 76.0/255.0, 192.0/255.0}; // Blue-red
// 	float c_hi[3] = {180.0/255.0, 4.0/255.0, 38.0/255.0};
// 	float c_lo[3] = {1.0/255.0, 102.0/255.0, 94.0/255.0};	// Colorbrewer BrBG 7
// 	float c_hi[3] = {140.0/255.0, 81.0/255.0, 10.0/255.0};
// 	float c_lo[3] = {27.0/255.0, 120.0/255.0, 55.0/255.0}; 	// Colorbrewer PRGn 7
// 	float c_hi[3] = {118.0/255.0, 42.0/255.0, 131.0/255.0};
// 	float c_lo[3] = {84.0/255.0, 39.0/255.0, 136.0/255.0}; 	// Colorbrewer PuOr 7
// 	float c_hi[3] = {230.0/255.0, 97.0/255.0, 1.0/255.0}; 
// 	ctf->AddRGBPoint(0.0, c_lo[0], c_lo[1], c_lo[2]);	// blue
// 	ctf->AddRGBPoint(1.0, c_hi[0], c_hi[1], c_hi[2]);		// red
	
	vtkstd::vector< vtkstd::vector<float> > cl;
	float cc6[] = {140, 81, 10}; 		vtkstd::vector<float> cc6v(cc6,cc6+7); cl.push_back(cc6v);
	float cc5[] = {216, 179, 101}; 	vtkstd::vector<float> cc5v(cc5,cc5+7); cl.push_back(cc5v);
	float cc4[] = {246, 232, 195}; 	vtkstd::vector<float> cc4v(cc4,cc4+7); cl.push_back(cc4v);
	float cc3[] = {245, 245, 245}; 	vtkstd::vector<float> cc3v(cc3,cc3+7); cl.push_back(cc3v);
	float cc2[] = {199, 234, 229}; 	vtkstd::vector<float> cc2v(cc2,cc2+7); cl.push_back(cc2v);
	float cc1[] = {90, 180, 172}; 	vtkstd::vector<float> cc1v(cc1,cc1+7); cl.push_back(cc1v);
	float cc0[] = {1, 102, 94}; 		vtkstd::vector<float> cc0v(cc0,cc0+7); cl.push_back(cc0v);
	
	float vv[] = {6,5,4,3,2,1,0};
	for (int ii = 0; ii < 7; ii++) 
		{
		float pt = vv[ii]/6.0;
		ctf->AddRGBPoint(pt, cl.at(ii)[0]/255.0f, cl.at(ii)[1]/255.0f, cl.at(ii)[2]/255.0f);
		}

	double ramp_val; 
	double cc[3];
  for (int ii = 0; ii < lutNum; ii++ )
    {
    ramp_val = static_cast<double>(ii)/static_cast<double>(lutNum); 
    ctf->GetColor(ramp_val, cc);
    this->lut->SetTableValue(ii,cc[0],cc[1],cc[2],1.0);
    }
  this->lut->SetRange(-1024,1024);

  // Map the image through the lookup table
  this->color = vtkSmartPointer<vtkImageMapToColors>::New();
  this->color->SetLookupTable(this->lut);
  this->color->SetInputConnection(this->reslice->GetOutputPort());

	// Create a greyscale lookup table for center image
  this->lutBW = vtkSmartPointer<vtkLookupTable>::New();
	this->lutBW->SetValueRange(0.0, 1.0); 			// from black to white
	this->lutBW->SetSaturationRange(0.0, 0.0); 	// no color saturation
	this->lutBW->SetRampToLinear();							// set range when colorBW intput set
	this->lutBW->Build();
  // Map the center image through the lookup table
  this->colorBW = vtkSmartPointer<vtkImageMapToColors>::New();
  this->colorBW->SetLookupTable(this->lutBW);
	// Set input when CenterImage is assigned
}

//-----------------------------------------------------------------------------
vtkMyChartXY::~vtkMyChartXY()
{
  for (unsigned int i = 0; i < this->ChartPrivate->plots.size(); ++i)
    {
    this->ChartPrivate->plots[i]->Delete();
    }
  for (size_t i = 0; i < 4; ++i)
    {
    this->ChartPrivate->axes[i]->Delete();
    }
  delete this->ChartPrivate;
  this->ChartPrivate = 0;

  this->Grid->Delete();
  this->Grid = 0;
  this->Legend->Delete();
  this->Legend = 0;

  this->Tooltip->Delete();
  this->Tooltip = 0;
  if (this->HighlightLink)
    {
    this->HighlightLink->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::Update()
{
  // The Stack accumulator should be re-initialized at the start of every 
  // update cycle.
  this->ChartPrivate->StackedPlotAccumulator = NULL;

  // Perform any necessary updates that are not graphical
  // Update the plots if necessary
  for (size_t i = 0; i < this->ChartPrivate->plots.size(); ++i)
    {
    this->ChartPrivate->plots[i]->Update();
    
    // Couldn't figure out another good place to set the axis titles...
    if (this->ChartPrivate->plots[i]-IsA("vtkMyPlotPoints") &&
        strcmp(this->ChartPrivate->axes[0]->GetTitle(),"Y Axis") == 0)
    	{
			vtkMyPlotPoints* myPlot = vtkMyPlotPoints::SafeDownCast(this->ChartPrivate->plots[i]); 
			vtkTable* table = myPlot->GetData()->GetInput();
			const char* xName = myPlot->GetData()->GetInputArrayToProcess(0, table)->GetName();
			const char* yName = myPlot->GetData()->GetInputArrayToProcess(1, table)->GetName();
			this->ChartPrivate->axes[0]->SetTitle(yName);
			this->ChartPrivate->axes[1]->SetTitle(xName);
			}
    }
  if (this->ShowLegend)
    {
    this->Legend->Update();
    }
}

//-----------------------------------------------------------------------------
bool vtkMyChartXY::Paint(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called.");

  int geometry[] = { this->GetScene()->GetSceneWidth(),
                     this->GetScene()->GetSceneHeight() };
  if (geometry[0] == 0 || geometry[1] == 0 || !this->Visible)
    {
    // The geometry of the chart must be valid before anything can be drawn
    return false;
    }

  int visiblePlots = 0;
  for (size_t i = 0; i < this->ChartPrivate->plots.size(); ++i)
    {
    if (this->ChartPrivate->plots[i]->GetVisible())
      {
      ++visiblePlots;
      }
    }

  if (visiblePlots == 0)
    {
    // Nothing to plot, so don't draw anything.
    return false;
    }

  this->Update();

  bool recalculateTransform = false;
  this->CalculateBarPlots();
  
  // NOTE: using this in DrawImage for center image for now -- should set for real...
  //  partly because borders now have to be set here and in SetBorders() call later
  // NOTE: There's also something strange here -- if data is changed, then if I don't have
  //  the preliminary positions set for the axis images, then they'll be placed
  //  at the origin until the first render...
	int origin[2] = {0,0};
	// This first call takes care of center image right after data change...
	if (this->ChartPrivate->aiOrientation == vtkMyChartXY::VERTICAL)
		{
		origin[0] = 20;
		origin[1] = 50;
		}
	else
		{
		origin[0] = 60;
		origin[1] = this->Geometry[1] - 160 + 50;
		}
	
  if (geometry[0] != this->Geometry[0] || geometry[1] != this->Geometry[1] ||
      this->MTime > this->ChartPrivate->axes[0]->GetMTime())
    {
    // Take up the entire window right now, this could be made configurable
    this->SetGeometry(geometry);
    // Borders (Left, Right, Top, Bottom)
		if (this->ChartPrivate->aiOrientation == vtkMyChartXY::VERTICAL)
			{
    	this->SetBorders(120, 20, 20, 50);
			origin[0] = 20;
			origin[1] = 50;
    	}
    else
    	{
    	this->SetBorders(60, 20, 160, 80);
			origin[0] = 60;
			origin[1] = this->Geometry[1] - 160 + 50;
    	}
  
    // This is where we set the axes up too
    // Y axis (left)
    this->ChartPrivate->axes[0]->SetPoint1(this->Point1[0], this->Point1[1]);
    this->ChartPrivate->axes[0]->SetPoint2(this->Point1[0], this->Point2[1]);
    // X axis (bottom)
    this->ChartPrivate->axes[1]->SetPoint1(this->Point1[0], this->Point1[1]);
    this->ChartPrivate->axes[1]->SetPoint2(this->Point2[0], this->Point1[1]);
    // Y axis (right)
    this->ChartPrivate->axes[2]->SetPoint1(this->Point2[0], this->Point1[1]);
    this->ChartPrivate->axes[2]->SetPoint2(this->Point2[0], this->Point2[1]);
    // X axis (top)
    this->ChartPrivate->axes[3]->SetPoint1(this->Point1[0], this->Point2[1]);
    this->ChartPrivate->axes[3]->SetPoint2(this->Point2[0], this->Point2[1]);

    // Put the legend in the top corner of the chart
    this->Legend->SetPoint(this->Point2[0], this->Point2[1]);
    // Cause the plot transform to be recalculated if necessary
    recalculateTransform = true;
    
    if (this->AxisImageStack)
      {
			if (this->ChartPrivate->aiOrientation == vtkMyChartXY::VERTICAL)
				{
				// NOTE: Leaving space here for center image (placed below axis images for now)
				//   which should always be the same size as axis images
				
				// Set initial scaling factor even before Paint for initial positions
				float pixelHeight = this->Point2[1] - this->Point1[1];
				float sumOfYExts = (this->NumImages+1)*this->ChartPrivate->aiHeight;
				float sumOfGaps = (this->NumImages-1+1)*this->ChartPrivate->aiGap;
				float YScale = (pixelHeight-sumOfGaps)/sumOfYExts;
				float XScale = (float)this->ChartPrivate->aiXSpace/(float)this->ChartPrivate->aiWidth;
				
				this->ChartPrivate->aiScalingFactor = (XScale < YScale) ? XScale : YScale;
				
				// Create the vector of axisImage objects
				float scWidth = this->ChartPrivate->aiWidth*this->ChartPrivate->aiScalingFactor;
				float scHeight = this->ChartPrivate->aiHeight*this->ChartPrivate->aiScalingFactor;
				for (int ii = 0; ii < this->NumImages; ii++) 
					{
					vtkAxisImagePrivate *ai = this->ChartPrivate->axisImages[ii];
					// NOTE: Adding one imHeight and gap to origin[1] to leave room for center image
					ai->Point1[0] = origin[0];
					ai->Point1[1] = origin[1] + scHeight + this->ChartPrivate->aiGap +
							ii*(scHeight + this->ChartPrivate->aiGap);
					ai->Point2[0] = ai->Point1[0] + scWidth;
					ai->Point2[1] = ai->Point1[1] + scHeight;
					}
				}
			else
				{
				// NOTE: Leaving space here for center image (placed left of axis images for now)
				//   which should always be the same size as axis images
				
				// Set initial scaling factor even before Paint for initial positions
				float pixelWidth = this->Point2[0] - this->Point1[0];
				float sumOfXExts = (this->NumImages+1)*this->ChartPrivate->aiWidth;
				float sumOfGaps = (this->NumImages-1+1)*this->ChartPrivate->aiGap;
				float XScale = (pixelWidth-sumOfGaps)/sumOfXExts;
				float YScale = (float)this->ChartPrivate->aiYSpace/(float)this->ChartPrivate->aiHeight;
				
				this->ChartPrivate->aiScalingFactor = (XScale < YScale) ? XScale : YScale;
				
				// Create the vector of axisImage objects
				float scWidth = this->ChartPrivate->aiWidth*this->ChartPrivate->aiScalingFactor;
				float scHeight = this->ChartPrivate->aiHeight*this->ChartPrivate->aiScalingFactor;
				for (int ii = 0; ii < this->NumImages; ii++) 
					{
					vtkAxisImagePrivate *ai = this->ChartPrivate->axisImages[ii];
					// Adding one imHeight and gap to origin[1] to leave room for center image
					ai->Point1[0] = origin[0] + scHeight + this->ChartPrivate->aiGap +
							ii*(scHeight + this->ChartPrivate->aiGap);
					ai->Point1[1] = origin[1];
					ai->Point2[0] = ai->Point1[0] + scWidth;
					ai->Point2[1] = ai->Point1[1] + scHeight;
					}
				}
			}
    }

  if (this->ChartPrivate->plots[0]->GetData()->GetInput()->GetMTime() > this->MTime || 
      this->ChartPrivate->StackParticipantsChanged > this->MTime)
    {
    this->RecalculateBounds();
    }

  // Recalculate the plot transform, min and max values if necessary
  if (!this->PlotTransformValid)
    {
    this->RecalculatePlotBounds();
    this->RecalculatePlotTransforms();
    }
  else if (recalculateTransform)
    {
    this->RecalculatePlotTransforms();
    }

  // Update the axes in the chart
  for (int i = 0; i < 4; ++i)
    {
    this->ChartPrivate->axes[i]->Update();
    }

  // Draw the grid - the axes take care of its color and visibility
  this->Grid->Paint(painter);

  // Plot the series of the chart
  this->RenderPlots(painter);

  // Set the color and width, draw the axes, color and width push to axis props
  painter->GetPen()->SetColorF(0.0, 0.0, 0.0, 1.0);
  painter->GetPen()->SetWidth(1.0);

  // Paint the axes in the chart
  for (int i = 0; i < 4; ++i)
    {
    this->ChartPrivate->axes[i]->Paint(painter);
    }

  if (this->ShowLegend)
    {
    this->Legend->Paint(painter);
    }

  // Draw the selection box if necessary
  if (this->DrawBox)
    {
    painter->GetBrush()->SetColor(255, 255, 255, 0);
    painter->GetPen()->SetColor(0, 0, 0, 255);
    painter->GetPen()->SetWidth(1.0);
    painter->DrawRect(this->BoxOrigin[0], this->BoxOrigin[1],
                      this->BoxGeometry[0], this->BoxGeometry[1]);
    }

  if (this->Title)
    {
    vtkPoints2D *rect = vtkPoints2D::New();
    rect->InsertNextPoint(this->Point1[0], this->Point2[1]);
    rect->InsertNextPoint(this->Point2[0]-this->Point1[0], 10);
    painter->ApplyTextProp(this->TitleProperties);
    painter->DrawStringRect(rect, this->Title);
    rect->Delete();
    }
    
  // Draw the XY association graphics behind axis images
  if (this->AxisImageStack)
    {
		if (this->ChartPrivate->aiOrientation == vtkMyChartXY::VERTICAL)
			{
			int xI = this->ChartPrivate->currentXai;
			int yI = this->ChartPrivate->currentYai;
			int x0 = this->ChartPrivate->axisImages[0]->Point1[0] - 5;
			int y0 = this->ChartPrivate->axisImages[xI]->Point1[1] + static_cast<int>((float)this->ChartPrivate->aiHeight*this->ChartPrivate->aiScalingFactor/2.0);
			int x1 = this->ChartPrivate->axisImages[0]->Point2[0] + 5;
			int y1 = this->ChartPrivate->axisImages[yI]->Point1[1] + static_cast<int>((float)this->ChartPrivate->aiHeight*this->ChartPrivate->aiScalingFactor/2.0);
			painter->GetBrush()->SetColor(255, 255, 255, 0);
			painter->GetPen()->SetColor(0, 0, 0, 100);
			painter->GetPen()->SetWidth(2.0);
			painter->DrawLine(x0,y0,x0+10,y0);
			painter->DrawLine(x0,y1,x0+10,y1);
			painter->DrawLine(x0,y0,x0,y1);
			}
		else
			{
			int xI = this->ChartPrivate->currentXai;
			int x0 = this->ChartPrivate->axisImages[xI]->Point1[0];
			int y0 = this->ChartPrivate->axisImages[xI]->Point1[1]-2;
			int x1 = this->ChartPrivate->axisImages[xI]->Point2[0];
			int y1 = this->ChartPrivate->axisImages[xI]->Point1[1]-2;
			painter->GetBrush()->SetColor(255, 255, 255, 0);
			painter->GetPen()->SetColor(0, 0, 0, 100);
			painter->GetPen()->SetWidth(2.0);
			painter->DrawLine(x0,y0,x1,y1);

			int yI = this->ChartPrivate->currentYai;
			x0 = this->ChartPrivate->axisImages[yI]->Point1[0]-2;
			y0 = this->ChartPrivate->axisImages[yI]->Point1[1];
			x1 = this->ChartPrivate->axisImages[yI]->Point1[0]-2;
			y1 = this->ChartPrivate->axisImages[yI]->Point2[1];
			painter->GetBrush()->SetColor(255, 255, 255, 0);
			painter->GetPen()->SetColor(0, 0, 0, 100);
			painter->GetPen()->SetWidth(2.0);
			painter->DrawLine(x0,y0,x1,y1);

			}
    }
  
  // Draw the axis images and center image
  if (this->AxisImageStack)
    {
    for (int ii = 0; ii < this->NumImages; ii++)
      {
      painter->DrawImage(this->ChartPrivate->axisImages[ii]->Point1[0],
      		this->ChartPrivate->axisImages[ii]->Point1[1],
      		this->ChartPrivate->aiScalingFactor,
      		this->ChartPrivate->axisImages[ii]->Image);
      }
    
    if (this->CenterImage)
      {
      painter->DrawImage(origin[0], origin[1],
      		this->ChartPrivate->aiScalingFactor,
      		this->colorBW->GetOutput());      
      }
    }

  // Draw in the current mouse location...
  this->Tooltip->Paint(painter);

  return true;
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::RenderPlots(vtkContext2D *painter)
{
  vtkIdTypeArray *idArray = 0;
  if (this->AnnotationLink)
    {
    this->AnnotationLink->Update();
    vtkSelection *selection =
        vtkSelection::SafeDownCast(this->AnnotationLink->GetOutputDataObject(2));
    if (selection->GetNumberOfNodes())
      {
      vtkSelectionNode *node = selection->GetNode(0);
      idArray = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
      }
    }
  else
    {
    vtkDebugMacro("No annotation link set.");
    }

  // Handle highlight selections back into this chart
  vtkIdTypeArray *hidArray = 0;
  if (this->HighlightLink)
    {
    this->HighlightLink->Update();
    vtkSelection *selection =
        vtkSelection::SafeDownCast(this->HighlightLink->GetOutputDataObject(2));
    if (selection->GetNumberOfNodes())
      {
      vtkSelectionNode *node = selection->GetNode(0);
      hidArray = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
      }
    }
  else
    {
    vtkDebugMacro("No highlight annotation link set.");
    }


  // Clip drawing while plotting
  float clip[] = { this->Point1[0], this->Point1[1],
                 this->Point2[0]-this->Point1[0],
                 this->Point2[1]-this->Point1[1] };
  // Check whether the scene has a transform - use it if so
  if (this->Scene->HasTransform())
    {
    this->Scene->GetTransform()->InverseTransformPoints(clip, clip, 2);
    }
  int clipi[] = { static_cast<int>(clip[0]),
                  static_cast<int>(clip[1]),
                  static_cast<int>(clip[2]),
                  static_cast<int>(clip[3]) };
  painter->GetDevice()->SetClipping(clipi);

  // Push the matrix and use the transform we just calculated
  for (int i = 0; i < 4; ++i)
    {
    if (this->ChartPrivate->PlotCorners[i].size())
      {
      painter->PushMatrix();
      painter->AppendTransform(this->ChartPrivate->PlotTransforms[i]);

      // Now iterate through the plots
      vtkstd::vector<vtkPlot*>::iterator it =
          this->ChartPrivate->PlotCorners[i].begin();
      for ( ; it != this->ChartPrivate->PlotCorners[i].end(); ++it)
        {
        (*it)->SetSelection(idArray);
				if ((*it)->IsA("vtkMyPlotPoints"))
					{
					vtkMyPlotPoints* myPlot = vtkMyPlotPoints::SafeDownCast(*it);
        	myPlot->SetHighlightSelection(hidArray);
        	}
        (*it)->Paint(painter);
        }
      painter->PopMatrix();
      }
    }

  // Stop clipping of the plot area and reset back to screen coordinates
  painter->GetDevice()->DisableClipping();
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::CalculateBarPlots()
{
  // Calculate the width, spacing and offsets for the bar plot - they are grouped
  size_t n = this->ChartPrivate->plots.size();
  vtkstd::vector<vtkPlotBar *> bars;
  for (size_t i = 0; i < n; ++i)
    {
    vtkPlotBar* bar = vtkPlotBar::SafeDownCast(this->ChartPrivate->plots[i]);
    if (bar && bar->GetVisible())
      {
      bars.push_back(bar);
      }
    }
  if (bars.size())
    {
    // We have some bar plots - work out offsets etc.
    float barWidth = 0.0;
    vtkPlotBar* bar = bars[0];
    if (!bar->GetUseIndexForXSeries())
      {
      vtkTable *table = bar->GetData()->GetInput();
      vtkDataArray* x = bar->GetData()->GetInputArrayToProcess(0, table);
      if (x->GetSize() > 1)
        {
        double x0 = x->GetTuple1(0);
        double x1 = x->GetTuple1(1);
        float width = static_cast<float>((x1 - x0) * this->BarWidthFraction);
        barWidth = width / bars.size();
        }
      }
    else
      {
      barWidth = 1.0f / bars.size() * this->BarWidthFraction;
      }

    // Now set the offsets and widths on each bar
    // The offsetIndex deals with the fact that half the bars
    // must shift to the left of the point and half to the right
    int offsetIndex = static_cast<int>(bars.size() - 1);
    for (size_t i = 0; i < bars.size(); ++i)
      {
      bars[i]->SetWidth(barWidth);
      bars[i]->SetOffset(offsetIndex * (barWidth / 2));
      // Increment by two since we need to shift by half widths
      // but make room for entire bars. Increment backwards because
      // offsets are always subtracted and Positive offsets move
      // the bar leftwards.  Negative offsets will shift the bar
      // to the right.
      offsetIndex -= 2;
      //bars[i]->SetOffset(float(bars.size()-i-1)*(barWidth/2));
      }
    }
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::RecalculatePlotTransforms()
{
  if (this->ChartPrivate->PlotCorners[0].size())
    {
    this->RecalculatePlotTransform(this->ChartPrivate->axes[vtkAxis::BOTTOM],
                                   this->ChartPrivate->axes[vtkAxis::LEFT],
                                   this->ChartPrivate->PlotTransforms[0]);
    }
  if (this->ChartPrivate->PlotCorners[1].size())
    {
    if (!this->ChartPrivate->PlotTransforms[1])
      {
      this->ChartPrivate->PlotTransforms[1] = vtkSmartPointer<vtkTransform2D>::New();
      }
    this->RecalculatePlotTransform(this->ChartPrivate->axes[vtkAxis::BOTTOM],
                                   this->ChartPrivate->axes[vtkAxis::RIGHT],
                                   this->ChartPrivate->PlotTransforms[1]);
    }
  if (this->ChartPrivate->PlotCorners[2].size())
    {
    if (!this->ChartPrivate->PlotTransforms[2])
      {
      this->ChartPrivate->PlotTransforms[2] = vtkSmartPointer<vtkTransform2D>::New();
      }
    this->RecalculatePlotTransform(this->ChartPrivate->axes[vtkAxis::TOP],
                                   this->ChartPrivate->axes[vtkAxis::RIGHT],
                                   this->ChartPrivate->PlotTransforms[2]);
    }
  if (this->ChartPrivate->PlotCorners[3].size())
    {
    if (!this->ChartPrivate->PlotTransforms[3])
      {
      this->ChartPrivate->PlotTransforms[3] = vtkSmartPointer<vtkTransform2D>::New();
      }
    this->RecalculatePlotTransform(this->ChartPrivate->axes[vtkAxis::TOP],
                                   this->ChartPrivate->axes[vtkAxis::LEFT],
                                   this->ChartPrivate->PlotTransforms[3]);
    }
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::RecalculatePlotTransform(vtkAxis *x, vtkAxis *y,
                                          vtkTransform2D *transform)
{
  // Get the scale for the plot area from the x and y axes
  float *min = x->GetPoint1();
  float *max = x->GetPoint2();
  if (fabs(max[0] - min[0]) == 0.0f)
    {
    return;
    }
  float xScale = (x->GetMaximum() - x->GetMinimum()) / (max[0] - min[0]);

  // Now the y axis
  min = y->GetPoint1();
  max = y->GetPoint2();
  if (fabs(max[1] - min[1]) == 0.0f)
    {
    return;
    }
  float yScale = (y->GetMaximum() - y->GetMinimum()) / (max[1] - min[1]);

  transform->Identity();
  transform->Translate(this->Point1[0], this->Point1[1]);
  // Get the scale for the plot area from the x and y axes
  transform->Scale(1.0 / xScale, 1.0 / yScale);
  transform->Translate(
      -this->ChartPrivate->axes[vtkAxis::BOTTOM]->GetMinimum(),
      -this->ChartPrivate->axes[vtkAxis::LEFT]->GetMinimum());

  // Move the axes if necessary and if the draw axes at origin ivar is true.
  if (this->DrawAxesAtOrigin && x == this->ChartPrivate->axes[vtkAxis::BOTTOM] &&
      y == this->ChartPrivate->axes[vtkAxis::LEFT])
    {
    // Get the screen coordinates for the origin, and move the axes there.
    float origin[2] = { 0.0, 0.0 };
    transform->TransformPoints(origin, origin, 1);
    // Need to clamp the axes in the plot area.
    if (int(origin[0]) < this->Point1[0])
      {
      origin[0] = this->Point1[0];
      }
    if (int(origin[0]) > this->Point2[0])
      {
      origin[0] = this->Point2[0];
      }
    if (int(origin[1]) < this->Point1[1])
      {
      origin[1] = this->Point1[1];
      }
    if (int(origin[1]) > this->Point2[1])
      {
      origin[1] = this->Point2[1];
      }

    this->ChartPrivate->axes[vtkAxis::BOTTOM]->SetPoint1(this->Point1[0], origin[1]);
    this->ChartPrivate->axes[vtkAxis::BOTTOM]->SetPoint2(this->Point2[0], origin[1]);
    this->ChartPrivate->axes[vtkAxis::LEFT]->SetPoint1(origin[0], this->Point1[1]);
    this->ChartPrivate->axes[vtkAxis::LEFT]->SetPoint2(origin[0], this->Point2[1]);
    }

  this->PlotTransformValid = true;
}

//-----------------------------------------------------------------------------
int vtkMyChartXY::GetPlotCorner(vtkPlot *plot)
{
  vtkAxis *x = plot->GetXAxis();
  vtkAxis *y = plot->GetYAxis();
  if (x == this->ChartPrivate->axes[vtkAxis::BOTTOM] &&
      y == this->ChartPrivate->axes[vtkAxis::LEFT])
    {
    return 0;
    }
  else if (x == this->ChartPrivate->axes[vtkAxis::BOTTOM] &&
           y == this->ChartPrivate->axes[vtkAxis::RIGHT])
    {
    return 1;
    }
  else if (x == this->ChartPrivate->axes[vtkAxis::TOP] &&
           y == this->ChartPrivate->axes[vtkAxis::RIGHT])
    {
    return 2;
    }
  else if (x == this->ChartPrivate->axes[vtkAxis::TOP] &&
           y == this->ChartPrivate->axes[vtkAxis::LEFT])
    {
    return 3;
    }
  else
    {
    // Should never happen.
    return 4;
    }
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::SetPlotCorner(vtkPlot *plot, int corner)
{
  if (corner < 0 || corner > 3)
    {
    vtkWarningMacro("Invalid corner specified, should be between 0 and 3: "
                    << corner);
    return;
    }
  if (!this->RemovePlotFromConers(plot))
    {
    vtkWarningMacro("Error removing plot from corners.");
    }
  this->ChartPrivate->PlotCorners[corner].push_back(plot);
  if (corner == 0)
    {
    plot->SetXAxis(this->ChartPrivate->axes[vtkAxis::BOTTOM]);
    plot->SetYAxis(this->ChartPrivate->axes[vtkAxis::LEFT]);
    }
  else if (corner == 1)
    {
    plot->SetXAxis(this->ChartPrivate->axes[vtkAxis::BOTTOM]);
    plot->SetYAxis(this->ChartPrivate->axes[vtkAxis::RIGHT]);
    }
  else if (corner == 2)
    {
    plot->SetXAxis(this->ChartPrivate->axes[vtkAxis::TOP]);
    plot->SetYAxis(this->ChartPrivate->axes[vtkAxis::RIGHT]);
    }
  else if (corner == 3)
    {
    plot->SetXAxis(this->ChartPrivate->axes[vtkAxis::TOP]);
    plot->SetYAxis(this->ChartPrivate->axes[vtkAxis::LEFT]);
    }
  this->PlotTransformValid = false;
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::RecalculatePlotBounds()
{
  // Get the bounds of each plot, and each axis  - ordering as laid out below
  double y1[] = { 0.0, 0.0 }; // left -> 0
  double x1[] = { 0.0, 0.0 }; // bottom -> 1
  double y2[] = { 0.0, 0.0 }; // right -> 2
  double x2[] = { 0.0, 0.0 }; // top -> 3
  // Store whether the ranges have been initialized - follows same order
  bool initialized[] = { false, false, false, false };

  vtkstd::vector<vtkPlot*>::iterator it;
  double bounds[4] = { 0.0, 0.0, 0.0, 0.0 };
  for (it = this->ChartPrivate->plots.begin();
       it != this->ChartPrivate->plots.end(); ++it)
    {
    if ((*it)->GetVisible() == false)
      {
      continue;
      }
    (*it)->GetBounds(bounds);
    int corner = this->GetPlotCorner(*it);
    
    // Initialize the appropriate ranges, or push out the ranges
    if ((corner == 0 || corner == 3)) // left
      {
      if (!initialized[0])
        {
        y1[0] = bounds[2];
        y1[1] = bounds[3];
        initialized[0] = true;
        }
      else
        {
        if (y1[0] > bounds[2]) // min
          {
          y1[0] = bounds[2];
          }
        if (y1[1] < bounds[3]) // max
          {
          y1[1] = bounds[3];
          }
        }
      }
    if ((corner == 0 || corner == 1)) // bottom
      {
      if (!initialized[1])
        {
        x1[0] = bounds[0];
        x1[1] = bounds[1];
        initialized[1] = true;
        }
      else
        {
        if (x1[0] > bounds[0]) // min
          {
          x1[0] = bounds[0];
          }
        if (x1[1] < bounds[1]) // max
          {
          x1[1] = bounds[1];
          }
        }
      }
    if ((corner == 1 || corner == 2)) // right
      {
      if (!initialized[2])
        {
        y2[0] = bounds[2];
        y2[1] = bounds[3];
        initialized[2] = true;
        }
      else
        {
        if (y2[0] > bounds[2]) // min
          {
          y2[0] = bounds[2];
          }
        if (y2[1] < bounds[3]) // max
          {
          y2[1] = bounds[3];
          }
        }
      }
    if ((corner == 2 || corner == 3)) // top
      {
      if (!initialized[1])
        {
        x2[0] = bounds[0];
        x2[1] = bounds[1];
        initialized[3] = true;
        }
      else
        {
        if (x2[0] > bounds[0]) // min
          {
          x2[0] = bounds[0];
          }
        if (x2[1] < bounds[1]) // max
          {
          x2[1] = bounds[1];
          }
        }
      }
    }

  // Now set the newly calculated bounds on the axes
  for (int i = 0; i < 4; ++i)
    {
    vtkAxis *axis = this->ChartPrivate->axes[i];
    double *range = 0;
    switch (i)
      {
      case 0:
        range = y1;
        break;
      case 1:
        range = x1;
        break;
      case 2:
        range = y2;
        break;
      case 3:
        range = x2;
        break;
      default:
        return;
      }

    if (axis->GetBehavior() == 0 && initialized[i])
      {
      axis->SetRange(range[0], range[1]);
      axis->AutoScale();
      }
    }

  this->Modified();
}

//-----------------------------------------------------------------------------
vtkPlot * vtkMyChartXY::AddPlot(int type)
{
  // Use a variable to return the object created (or NULL), this is necessary
  // as the HP compiler is broken (thinks this function does not return) and
  // the MS compiler generates a warning about unreachable code if a redundant
  // return is added at the end.
  vtkPlot *plot = NULL;
  vtkColor3ub color = this->ChartPrivate->Colors->GetColorRepeating(
      static_cast<int>(this->ChartPrivate->plots.size()));
  switch (type)
    {
    case LINE:
      {
      vtkPlotLine *line = vtkPlotLine::New();
      line->GetPen()->SetColor(color.GetData());
      plot = line;
      break;
      }
    case POINTS:
      {
      vtkMyPlotPoints *points = vtkMyPlotPoints::New();
      points->GetPen()->SetColor(color.GetData());
      plot = points;
      break;
      }
    case BAR:
      {
      vtkPlotBar *bar = vtkPlotBar::New();
      bar->GetBrush()->SetColor(color.GetData());
      plot = bar;
      break;
      }
    case STACKED:
      {
      vtkPlotStacked *stacked = vtkPlotStacked::New();
      stacked->SetParent(this);
      stacked->GetBrush()->SetColor(color.GetData());
      plot = stacked;
      break;
      }
    default:
      plot = NULL;
    }
  // Add the plot to the default corner
  plot->SetXAxis(this->ChartPrivate->axes[vtkAxis::BOTTOM]);
  plot->SetYAxis(this->ChartPrivate->axes[vtkAxis::LEFT]);
	if (plot->IsA("vtkMyPlotPoints"))
		{
		vtkMyPlotPoints* myPlot = vtkMyPlotPoints::SafeDownCast(plot);
		vtkIdTypeArray* highlightSelection = vtkIdTypeArray::New();
		// NOTE: This may be dubious, setting the same highlight selection for all plots added...
		myPlot->SetHighlightSelection(highlightSelection);
		}
  this->ChartPrivate->plots.push_back(plot);
  this->ChartPrivate->PlotCorners[0].push_back(plot);
  // Ensure that the bounds are recalculated
  this->PlotTransformValid = false;
  // Mark the scene as dirty
  this->Scene->SetDirty(true);
  return plot;
}

//-----------------------------------------------------------------------------
bool vtkMyChartXY::RemovePlot(vtkIdType index)
{
  if (static_cast<vtkIdType>(this->ChartPrivate->plots.size()) > index)
    {
    this->RemovePlotFromConers(this->ChartPrivate->plots[index]);
    this->ChartPrivate->plots[index]->Delete();
    this->ChartPrivate->plots.erase(this->ChartPrivate->plots.begin()+index);

    // Ensure that the bounds are recalculated
    this->PlotTransformValid = false;
    // Mark the scene as dirty
    this->Scene->SetDirty(true);
    return true;
    }
  else
    {
    return false;
    }
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::ClearPlots()
{
  for (unsigned int i = 0; i < this->ChartPrivate->plots.size(); ++i)
    {
    this->RemovePlotFromConers(this->ChartPrivate->plots[i]);
    this->ChartPrivate->plots[i]->Delete();
    }
  this->ChartPrivate->plots.clear();
  // Ensure that the bounds are recalculated
  this->PlotTransformValid = false;
  // Clear out all axis image stuff
  for (int ii = 0; ii < this->ChartPrivate->axisImages.size(); ++ii)
  	{
  	delete this->ChartPrivate->axisImages[ii];
  	}
  this->ChartPrivate->axisImages.clear();
  // Set the x and y axes titles back to default
  this->ChartPrivate->axes[vtkAxis::LEFT]->SetTitle("Y Axis");
  this->ChartPrivate->axes[vtkAxis::BOTTOM]->SetTitle("X Axis");
  
  // Mark the scene as dirty
  this->Scene->SetDirty(true);
}

//-----------------------------------------------------------------------------
vtkPlot* vtkMyChartXY::GetPlot(vtkIdType index)
{
  if (static_cast<vtkIdType>(this->ChartPrivate->plots.size()) > index)
    {
    return this->ChartPrivate->plots[index];
    }
  else
    {
    return NULL;
    }
}

//-----------------------------------------------------------------------------
vtkIdType vtkMyChartXY::GetNumberOfPlots()
{
  return this->ChartPrivate->plots.size();
}

//-----------------------------------------------------------------------------
vtkAxis* vtkMyChartXY::GetAxis(int axisIndex)
{
  if (axisIndex < 4)
    {
    return this->ChartPrivate->axes[axisIndex];
    }
  else
    {
    return NULL;
    }
}

//-----------------------------------------------------------------------------
vtkIdType vtkMyChartXY::GetNumberOfAxes()
{
  return 4;
}


//-----------------------------------------------------------------------------
void vtkMyChartXY::RecalculateBounds()
{
  // Ensure that the bounds are recalculated
  this->PlotTransformValid = false;
  // Mark the scene as dirty
  this->Scene->SetDirty(true);
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::SetScene(vtkContextScene *scene)
{
  this->vtkContextItem::SetScene(scene);
  this->Tooltip->SetScene(scene);
}
//-----------------------------------------------------------------------------
namespace {
template <class A>
void InitializeAccumulator(A *a, int n)
{
  for (int i = 0; i < n; ++i)
    {
    a[i] = 0;
    }
}

}

//-----------------------------------------------------------------------------
vtkDataArray *vtkMyChartXY::GetStackedPlotAccumulator(int dataType, int n)
{
  if (!this->ChartPrivate->StackedPlotAccumulator) 
    {
    this->ChartPrivate->StackedPlotAccumulator.TakeReference(vtkDataArray::SafeDownCast(vtkDataArray::CreateArray(dataType)));
    if (!this->ChartPrivate->StackedPlotAccumulator) 
      {
      return NULL;
      }
    this->ChartPrivate->StackedPlotAccumulator->SetNumberOfTuples(n);
    switch (dataType) 
      {
          vtkTemplateMacro(
            InitializeAccumulator(static_cast<VTK_TT*>(this->ChartPrivate->StackedPlotAccumulator->GetVoidPointer(0)),n));
      }
    return this->ChartPrivate->StackedPlotAccumulator;
    }
  else 
    {
    if (this->ChartPrivate->StackedPlotAccumulator->GetDataType() != dataType)
      {
      vtkErrorMacro("DataType of Accumulator " << this->ChartPrivate->StackedPlotAccumulator->GetDataType() << 
                    "does not match request " << dataType);
      return NULL;
      }
    if (this->ChartPrivate->StackedPlotAccumulator->GetNumberOfTuples() != n) 
      {
      vtkErrorMacro("Number of tuples in Accumulator " << this->ChartPrivate->StackedPlotAccumulator->GetNumberOfTuples() << 
                    "does not match request " << n);
      return NULL;
      }
    return this->ChartPrivate->StackedPlotAccumulator;
    }
}

//-----------------------------------------------------------------------------
vtkTimeStamp vtkMyChartXY::GetStackParticipantsChanged()
{
  return this->ChartPrivate->StackParticipantsChanged;
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::SetStackPartipantsChanged()
{
  this->ChartPrivate->StackParticipantsChanged.Modified();
}

//-----------------------------------------------------------------------------
bool vtkMyChartXY::Hit(const vtkContextMouseEvent &mouse)
{
  int maxI = this->NumImages-1;
  // In plot area
  if (mouse.ScreenPos[0] > this->Point1[0] &&
      mouse.ScreenPos[0] < this->Point2[0] &&
      mouse.ScreenPos[1] > this->Point1[1] &&
      mouse.ScreenPos[1] < this->Point2[1])
    {
    return true;
    }
  // In axis images area
	if (this->ChartPrivate->aiOrientation == vtkMyChartXY::VERTICAL)
		{
		if (mouse.ScreenPos[0] > this->ChartPrivate->axisImages[0]->Point1[0] &&
						 mouse.ScreenPos[0] < this->ChartPrivate->axisImages[0]->Point2[0] &&
						 mouse.ScreenPos[1] > this->ChartPrivate->axisImages[0]->Point1[1] &&
						 mouse.ScreenPos[1] < this->ChartPrivate->axisImages[maxI]->Point2[1])
			{
			return true;
			}
		else
			{
			return false;
			}
		}
	else
		{
		if (mouse.ScreenPos[0] > this->ChartPrivate->axisImages[0]->Point1[0] &&
						 mouse.ScreenPos[0] < this->ChartPrivate->axisImages[maxI]->Point2[0] &&
						 mouse.ScreenPos[1] > this->ChartPrivate->axisImages[0]->Point1[1] &&
						 mouse.ScreenPos[1] < this->ChartPrivate->axisImages[0]->Point2[1])
			{
			return true;
			}
		else
			{
			return false;
			}
		}
}

//-----------------------------------------------------------------------------
bool vtkMyChartXY::MouseEnterEvent(const vtkContextMouseEvent &)
{
  // Find the nearest point on the curves and snap to it
  this->DrawNearestPoint = true;

  return true;
}

//-----------------------------------------------------------------------------
bool vtkMyChartXY::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  if (mouse.Button == vtkContextMouseEvent::RIGHT_BUTTON)
    {
    // Figure out how much the mouse has moved by in plot coordinates - pan
    double screenPos[2] = { mouse.ScreenPos[0], mouse.ScreenPos[1] };
    double lastScreenPos[2] = { mouse.LastScreenPos[0], mouse.LastScreenPos[1] };
    double pos[2] = { 0.0, 0.0 };
    double last[2] = { 0.0, 0.0 };

    // Go from screen to scene coordinates to work out the delta
    this->ChartPrivate->PlotTransforms[0]
        ->InverseTransformPoints(screenPos, pos, 1);
    this->ChartPrivate->PlotTransforms[0]
        ->InverseTransformPoints(lastScreenPos, last, 1);
    double delta[] = { last[0] - pos[0], last[1] - pos[1] };

    // Now move the axes and recalculate the transform
    vtkAxis* xAxis = this->ChartPrivate->axes[vtkAxis::BOTTOM];
    vtkAxis* yAxis = this->ChartPrivate->axes[vtkAxis::LEFT];
    xAxis->SetMinimum(xAxis->GetMinimum() + delta[0]);
    xAxis->SetMaximum(xAxis->GetMaximum() + delta[0]);
    yAxis->SetMinimum(yAxis->GetMinimum() + delta[1]);
    yAxis->SetMaximum(yAxis->GetMaximum() + delta[1]);

    // Same again for the axes in the top right
    if (this->ChartPrivate->PlotTransforms[2])
    {
      // Go from screen to scene coordinates to work out the delta
      this->ChartPrivate->PlotTransforms[2]
          ->InverseTransformPoints(screenPos, pos, 1);
      this->ChartPrivate->PlotTransforms[2]
          ->InverseTransformPoints(lastScreenPos, last, 1);
      delta[0] = last[0] - pos[0];
      delta[1] = last[1] - pos[1];

      // Now move the axes and recalculate the transform
      xAxis = this->ChartPrivate->axes[vtkAxis::TOP];
      yAxis = this->ChartPrivate->axes[vtkAxis::RIGHT];
      xAxis->SetMinimum(xAxis->GetMinimum() + delta[0]);
      xAxis->SetMaximum(xAxis->GetMaximum() + delta[0]);
      yAxis->SetMinimum(yAxis->GetMinimum() + delta[1]);
      yAxis->SetMaximum(yAxis->GetMaximum() + delta[1]);
    }

    this->RecalculatePlotTransforms();
    // Mark the scene as dirty
    this->Scene->SetDirty(true);
    }
  else if (mouse.Button == vtkContextMouseEvent::LEFT_BUTTON)
    {
    this->BoxGeometry[0] = mouse.Pos[0] - this->BoxOrigin[0];
    this->BoxGeometry[1] = mouse.Pos[1] - this->BoxOrigin[1];
    // Mark the scene as dirty
    this->Scene->SetDirty(true);
    }
  else if (mouse.Button == vtkContextMouseEvent::MIDDLE_BUTTON)
    {
    this->BoxGeometry[0] = mouse.Pos[0] - this->BoxOrigin[0];
    this->BoxGeometry[1] = mouse.Pos[1] - this->BoxOrigin[1];
    // Mark the scene as dirty
    this->Scene->SetDirty(true);
    }
  else if (mouse.Button < 0)
    {
    this->Scene->SetDirty(true);
    this->Tooltip->SetVisible(this->LocatePointInPlots(mouse));
    }

  return true;
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
              vtkVector3f plotPosAndInd;
							bool found = myPlot->GetNearestPoint(position, tolerance, &plotPosAndInd);
							if (found)
								{
								// We found a point, set up the tooltip and return
								vtksys_ios::ostringstream ostr;
								ostr << myPlot->GetLabel() << ": " << (int)plotPosAndInd.Z();
								this->Tooltip->SetText(ostr.str().c_str());
								this->Tooltip->SetPosition(mouse.ScreenPos[0]+8, mouse.ScreenPos[1]+6);
								
								// Testing random image from stack
								if (this->TooltipShowImage)
									{
									int num_images = myPlot->GetNumberOfImages();
									if (num_images > 0)
										{
										// int random_index = rand() % num_images;
										// this->Tooltip->SetTipImage(myPlot->GetImageAtIndex(random_index));
										this->Tooltip->SetTipImage(myPlot->GetImageAtIndex(static_cast<int>(plotPosAndInd.Z())));
										}
									}
								return true;
								}
							}
					  else
					    {
              vtkVector2f plotPos;
							bool found = plot->GetNearestPoint(position, tolerance, &plotPos);
							if (found)
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
bool vtkMyChartXY::MouseLeaveEvent(const vtkContextMouseEvent &)
{
  this->DrawNearestPoint = false;
  this->Tooltip->SetVisible(false);
  return true;
}

//-----------------------------------------------------------------------------
bool vtkMyChartXY::MouseButtonPressEvent(const vtkContextMouseEvent &mouse)
{
  this->Tooltip->SetVisible(false);
  if (mouse.Button == vtkContextMouseEvent::RIGHT_BUTTON)
    {
    // The mouse panning action.
    return true;
    }
  else if (mouse.Button == vtkContextMouseEvent::LEFT_BUTTON)
    {
		// Iterate over the axis images, see if we are inside of one
		for (size_t i = 0; i < this->ChartPrivate->axisImages.size(); ++i)
			{
			vtkAxisImagePrivate* image = this->ChartPrivate->axisImages[i];
			if (mouse.ScreenPos[0] > image->Point1[0] && mouse.ScreenPos[0] < image->Point2[0] &&
					mouse.ScreenPos[1] > image->Point1[1] && mouse.ScreenPos[1] < image->Point2[1])
				{
				bool modify_data;
				if (i > this->ChartPrivate->currentYai)
				  {
						this->ChartPrivate->currentYai = i;
						this->ChartPrivate->currentXai = i-1;
						modify_data = true;
				  }
				else if (i < this->ChartPrivate->currentXai)
				  {
						this->ChartPrivate->currentYai = i+1;
						this->ChartPrivate->currentXai = i;
						modify_data = true;
					}
				else
				  {
				  modify_data = false;
				  }
				if (modify_data)
				  {
					// NOTE: Looking for first visible vtkMyPlotPoints
					size_t n = this->ChartPrivate->plots.size();
					for (size_t p = 0; p < n; ++p)
						{
						vtkMyPlotPoints* plot = vtkMyPlotPoints::SafeDownCast(this->ChartPrivate->plots[p]);
						if (plot && plot->GetVisible())
							{
							vtkTable* table = plot->GetData()->GetInput();
							int yI = this->ChartPrivate->axisImages[this->ChartPrivate->currentYai]->ColumnIndex;
							int xI = this->ChartPrivate->axisImages[this->ChartPrivate->currentXai]->ColumnIndex;
							plot->SetInputArray(0,table->GetColumnName(xI));
							plot->SetInputArray(1,table->GetColumnName(yI));
							this->ChartPrivate->axes[0]->SetTitle(table->GetColumnName(yI));
							this->ChartPrivate->axes[1]->SetTitle(table->GetColumnName(xI));
							plot->Update();
    					this->Scene->SetDirty(true);
    					this->RecalculatePlotBounds();
							break;
							}
						}
				  }
				// Not sure why return true or false
				return false;
				}
			}
		
		// Selection, for now at least...
		this->BoxOrigin[0] = mouse.Pos[0];
		this->BoxOrigin[1] = mouse.Pos[1];
		this->BoxGeometry[0] = this->BoxGeometry[1] = 0.0f;
		this->DrawBox = true;
		return true;

    }
  else if (mouse.Button == vtkContextMouseEvent::MIDDLE_BUTTON)
    {
    // Right mouse button - zoom box
    this->BoxOrigin[0] = mouse.Pos[0];
    this->BoxOrigin[1] = mouse.Pos[1];
    this->BoxGeometry[0] = this->BoxGeometry[1] = 0.0f;
    this->DrawBox = true;
    return true;
    }
  else
    {
    return false;
    }
}

//-----------------------------------------------------------------------------
bool vtkMyChartXY::MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse)
{
  if (mouse.Button == vtkContextMouseEvent::LEFT_BUTTON)
    {
    // Check whether a valid selection box was drawn
    this->BoxGeometry[0] = mouse.Pos[0] - this->BoxOrigin[0];
    this->BoxGeometry[1] = mouse.Pos[1] - this->BoxOrigin[1];
    if (fabs(this->BoxGeometry[0]) < 0.5 || fabs(this->BoxGeometry[1]) < 0.5)
      {
      // Invalid box size - do nothing
      this->BoxGeometry[0] = this->BoxGeometry[1] = 0.0f;
      this->DrawBox = false;
      return true;
      }

    // Iterate through the plots and build a selection
    for (int i = 0; i < 4; ++i)
    {
      if (this->ChartPrivate->PlotCorners[i].size())
      {
        vtkTransform2D *transform = this->ChartPrivate->PlotTransforms[i];
        transform->InverseTransformPoints(this->BoxOrigin, this->BoxOrigin, 1);
        float point2[] = { mouse.Pos[0], mouse.Pos[1] };
        transform->InverseTransformPoints(point2, point2, 1);

        vtkVector2f min(this->BoxOrigin);
        vtkVector2f max(point2);
        if (min.X() > max.X())
          {
          float tmp = min.X();
          min.SetX(max.X());
          max.SetX(tmp);
          }
        if (min.Y() > max.Y())
          {
          float tmp = min.Y();
          min.SetY(max.Y());
          max.SetY(tmp);
          }

        vtkstd::vector<vtkPlot*>::iterator it =
            this->ChartPrivate->PlotCorners[i].begin();
        for ( ; it != this->ChartPrivate->PlotCorners[i].end(); ++it)
          {
          vtkPlot* plot = *it;
          if (plot->SelectPoints(min, max))
            {
            if (this->AnnotationLink)
              {
              // FIXME: Build up a selection from each plot?
              vtkSelection* selection = vtkSelection::New();
              vtkSelectionNode* node = vtkSelectionNode::New();
							node->SetContentType(vtkSelectionNode::INDICES);
							node->SetFieldType(vtkSelectionNode::POINT);
              selection->AddNode(node);
              node->SetSelectionList(plot->GetSelection());
              this->AnnotationLink->SetCurrentSelection(selection);
              node->Delete();
              selection->Delete();
              }
            }
          }
        }
      }

    this->InvokeEvent(vtkCommand::SelectionChangedEvent);
    this->BoxGeometry[0] = this->BoxGeometry[1] = 0.0f;
    this->DrawBox = false;
    // Mark the scene as dirty
    this->Scene->SetDirty(true);
    return true;
    }
  if (mouse.Button == vtkContextMouseEvent::MIDDLE_BUTTON)
    {
    // Check whether a valid zoom box was drawn
    this->BoxGeometry[0] = mouse.Pos[0] - this->BoxOrigin[0];
    this->BoxGeometry[1] = mouse.Pos[1] - this->BoxOrigin[1];
    if (fabs(this->BoxGeometry[0]) < 0.5 || fabs(this->BoxGeometry[1]) < 0.5)
      {
      // Invalid box size - do nothing
      this->BoxGeometry[0] = this->BoxGeometry[1] = 0.0f;
      this->DrawBox = false;
      return true;
      }

    // Zoom into the chart by the specified amount, and recalculate the bounds
    float point2[] = { mouse.Pos[0], mouse.Pos[1] };

    this->ZoomInAxes(this->ChartPrivate->axes[vtkAxis::BOTTOM],
                     this->ChartPrivate->axes[vtkAxis::LEFT],
                     this->BoxOrigin, point2);
    this->ZoomInAxes(this->ChartPrivate->axes[vtkAxis::TOP],
                     this->ChartPrivate->axes[vtkAxis::RIGHT],
                     this->BoxOrigin, point2);

    this->RecalculatePlotTransforms();
    this->BoxGeometry[0] = this->BoxGeometry[1] = 0.0f;
    this->DrawBox = false;
    // Mark the scene as dirty
    this->Scene->SetDirty(true);
    return true;
    }
  return false;
}

void vtkMyChartXY::ZoomInAxes(vtkAxis *x, vtkAxis *y, float *origin, float *max)
{
  vtkTransform2D *transform = vtkTransform2D::New();
  this->RecalculatePlotTransform(x, y, transform);
  float torigin[2];
  transform->InverseTransformPoints(origin, torigin, 1);
  float tmax[2];
  transform->InverseTransformPoints(max, tmax, 1);

  // Ensure we preserve the directionality of the axes
  if (x->GetMaximum() > x->GetMinimum())
    {
    x->SetMaximum(torigin[0] > tmax[0] ? torigin[0] : tmax[0]);
    x->SetMinimum(torigin[0] < tmax[0] ? torigin[0] : tmax[0]);
    }
  else
    {
    x->SetMaximum(torigin[0] < tmax[0] ? torigin[0] : tmax[0]);
    x->SetMinimum(torigin[0] > tmax[0] ? torigin[0] : tmax[0]);
    }
  if (y->GetMaximum() > y->GetMinimum())
    {
    y->SetMaximum(torigin[1] > tmax[1] ? torigin[1] : tmax[1]);
    y->SetMinimum(torigin[1] < tmax[1] ? torigin[1] : tmax[1]);
    }
  else
    {
    y->SetMaximum(torigin[1] < tmax[1] ? torigin[1] : tmax[1]);
    y->SetMinimum(torigin[1] > tmax[1] ? torigin[1] : tmax[1]);
    }
  x->RecalculateTickSpacing();
  y->RecalculateTickSpacing();
  transform->Delete();
}

//-----------------------------------------------------------------------------
bool vtkMyChartXY::MouseWheelEvent(const vtkContextMouseEvent &, int delta)
{
  this->Tooltip->SetVisible(false);
  // Get the bounds of each plot.
  for (int i = 0; i < 4; ++i)
    {
    vtkAxis *axis = this->ChartPrivate->axes[i];
    double min = axis->GetMinimum();
    double max = axis->GetMaximum();
    double frac = (max - min) * 0.1;
    if (frac > 0.0)
      {
      min += delta*frac;
      max -= delta*frac;
      }
    else
      {
      min -= delta*frac;
      max += delta*frac;
      }
    axis->SetMinimum(min);
    axis->SetMaximum(max);
    axis->RecalculateTickSpacing();
    }

  this->RecalculatePlotTransforms();

  // Mark the scene as dirty
  this->Scene->SetDirty(true);

  return true;
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::ProcessSelectionEvent(vtkObject* , void* )
{
}

//-----------------------------------------------------------------------------
bool vtkMyChartXY::RemovePlotFromConers(vtkPlot *plot)
{
  // We know the plot will only ever be in one of the corners
  for (int i = 0; i < 4; ++i)
    {
    vtkstd::vector<vtkPlot*>::iterator it =
        this->ChartPrivate->PlotCorners[i].begin();
    for ( ; it !=this->ChartPrivate->PlotCorners[i].end(); ++it)
      {
      if ((*it) == plot)
        {
        this->ChartPrivate->PlotCorners[i].erase(it);
        return true;
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
      if ((*it)->IsA("vtkMyPlotPoints"))
        {
        vtkMyPlotPoints* myPlot = vtkMyPlotPoints::SafeDownCast((*it));
        if ( myPlot->GetNumberOfImages() > 0)
          {
          this->Tooltip->SetTipImage(myPlot->GetImageAtIndex(0));
          }
        }
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
void vtkMyChartXY::SetAxisImageStack(vtkImageData* stack)
{
  this->AxisImageStack = stack;
  this->AxisImageStack->UpdateInformation();
  // Set input to reslice filter
  this->reslice->SetInput(this->AxisImageStack);
  this->reslice->Modified();
	// Set range of blue-white-red lut to range of whole stack
	double i_range[2];
	this->AxisImageStack->GetPointData()->GetArray("DiffIntensity")->GetRange(i_range);
	double i_abs[2] = {fabs(i_range[0]),fabs(i_range[1])};
	double i_ext = (i_abs[0] >= i_abs[1]) ? i_abs[0] : i_abs[1];
	if (i_ext < 1e-10) i_ext = 1024;
  this->lut->SetRange(-i_ext, i_ext);
  this->lut->Modified();
  // Gather number of images and image dimensions (extent)
  int extent[6];
  this->AxisImageStack->GetWholeExtent(extent);
  this->NumImages = (extent[5]-extent[4]+1);
  this->ChartPrivate->aiWidth = extent[1];
  this->ChartPrivate->aiHeight = extent[3];
  
  // Look through data table column names to gather valid data column indices
  // NOTE: Looking for first visible vtkMyPlotPoints and using that table
  size_t n = this->ChartPrivate->plots.size();
  vtkMyPlotPoints* plot;
  vtkTable* table;
  for (size_t i = 0; i < n; ++i)
    {
    plot = vtkMyPlotPoints::SafeDownCast(this->ChartPrivate->plots[i]);
    if (plot && plot->GetVisible())
      {
      table = plot->GetData()->GetInput();
      break;
      }
    }
  if (!plot || !(table->GetNumberOfColumns()>0))
    {
		printf("No viable plot found in SetAxisImageStack\n");
		return;
    }
  vtkstd::vector<vtkIdType> col_idxs;
  for (vtkIdType ii = 0; ii < table->GetNumberOfColumns(); ii++)
    {
    const char *col_name = table->GetColumnName(ii);
    if (strstr(col_name, "_ids"))
      {
      continue;
      }
    else
      {
      col_idxs.push_back(ii);
      }
    }
  if (col_idxs.size() != this->NumImages)
    {
		printf("Number of viable columns doesn't equal the number of images!\n");
		return;
    }
  const char* xName = plot->GetData()->GetInputArrayToProcess(0, table)->GetName();
  const char* yName = plot->GetData()->GetInputArrayToProcess(1, table)->GetName();
  
	if (this->ChartPrivate->aiOrientation == vtkMyChartXY::VERTICAL)
	  {
		// NOTE: Leaving space here for center image (placed below axis images for now)
		//   which should always be the same size as axis images
		
		// Set initial scaling factor even before Paint for initial positions
		float pixelHeight = this->Point2[1] - this->Point1[1];
		float sumOfYExts = (this->NumImages+1)*this->ChartPrivate->aiHeight;
		float sumOfGaps = (this->NumImages-1+1)*this->ChartPrivate->aiGap;
		float YScale = (pixelHeight-sumOfGaps)/sumOfYExts;
		
		float XScale = (float)this->ChartPrivate->aiXSpace/(float)this->ChartPrivate->aiWidth;
		
		this->ChartPrivate->aiScalingFactor = (XScale < YScale) ? XScale : YScale;
		
		// Create the vector of axisImage objects
		int origin[2] = {20,50};
		float scWidth = this->ChartPrivate->aiWidth*this->ChartPrivate->aiScalingFactor;
		float scHeight = this->ChartPrivate->aiHeight*this->ChartPrivate->aiScalingFactor;
		for (int ii = 0; ii < this->NumImages; ii++) 
			{
			vtkAxisImagePrivate *ai = new vtkAxisImagePrivate;
			ai->Image->DeepCopy(this->GetImageAtIndex(ii));
			// Real positions set in Paint() routine to adjust for scene geometry
			// NOTE: Doesn't crash if you remove this, but for some reason images show
			//   up at origin until next render (mouse enter) if this is deleted...
			// Adding one imHeight and gap to origin[1] to leave room for center image
			ai->Point1[0] = origin[0];
			ai->Point1[1] = origin[1] + scHeight + this->ChartPrivate->aiGap +
					ii*(scHeight + this->ChartPrivate->aiGap);
			ai->Point2[0] = ai->Point1[0] + scWidth;
			ai->Point2[1] = ai->Point1[1] + scHeight;
			ai->ColumnIndex = col_idxs.at(ii);
			this->ChartPrivate->axisImages.push_back(ai);
			
			// Keep track of current X and Y data as axisImages indices
			const char *col_name = table->GetColumnName(col_idxs.at(ii));
			if (strcmp(col_name, xName) == 0) 
				{
				this->ChartPrivate->currentXai = ii;
				}
			if (strcmp(col_name, yName) == 0)
				{
				this->ChartPrivate->currentYai = ii;
				}
			}
		}
	else
	  {
		// NOTE: Leaving space here for center image (placed left of axis images for now)
		//   which should always be the same size as axis images
		
		// Set initial scaling factor even before Paint for initial positions
		float pixelWidth = this->Point2[0] - this->Point1[0];
		float sumOfXExts = (this->NumImages+1)*this->ChartPrivate->aiWidth;
		float sumOfGaps = (this->NumImages-1+1)*this->ChartPrivate->aiGap;
		float XScale = (pixelWidth-sumOfGaps)/sumOfXExts;
		
		float YScale = (float)this->ChartPrivate->aiYSpace/(float)this->ChartPrivate->aiHeight;
		
		this->ChartPrivate->aiScalingFactor = (XScale < YScale) ? XScale : YScale;
		
		// Create the vector of axisImage objects
		int origin[2] = {this->Point1[0],this->Point2[1]+50};
		float scWidth = this->ChartPrivate->aiWidth*this->ChartPrivate->aiScalingFactor;
		float scHeight = this->ChartPrivate->aiHeight*this->ChartPrivate->aiScalingFactor;
		for (int ii = 0; ii < this->NumImages; ii++) 
			{
			vtkAxisImagePrivate *ai = new vtkAxisImagePrivate;
			ai->Image->DeepCopy(this->GetImageAtIndex(ii));
			// Real positions set in Paint() routine to adjust for scene geometry
			// NOTE: Doesn't crash if you remove this, but for some reason images show
			//   up at origin until next render (mouse enter) if this is deleted...
			// Adding one imHeight and gap to origin[1] to leave room for center image
			ai->Point1[0] = origin[0] + scHeight + this->ChartPrivate->aiGap +
					ii*(scHeight + this->ChartPrivate->aiGap);
			ai->Point1[1] = origin[1];
			ai->Point2[0] = ai->Point1[0] + scWidth;
			ai->Point2[1] = ai->Point1[1] + scHeight;
			ai->ColumnIndex = col_idxs.at(ii);
			this->ChartPrivate->axisImages.push_back(ai);
			
			// Keep track of current X and Y data as axisImages indices
			const char *col_name = table->GetColumnName(col_idxs.at(ii));
			if (strcmp(col_name, xName) == 0) 
				{
				this->ChartPrivate->currentXai = ii;
				}
			if (strcmp(col_name, yName) == 0)
				{
				this->ChartPrivate->currentYai = ii;
				}
			}
	  }
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::SetCenterImage(vtkImageData* image)
{
  this->CenterImage = image;
  this->CenterImage->UpdateInformation();
  // Set input to colorBW filter
  this->colorBW->SetInput(this->CenterImage);
  this->colorBW->Modified();
	// Set range of lutBW
	double i_range[2];
	this->CenterImage->GetPointData()->GetArray("Intensity")->GetRange(i_range);
  this->lutBW->SetRange(i_range);
  this->lutBW->Modified();
  // Not calling update before draw, so do it here.
  this->colorBW->UpdateWholeExtent();
  this->colorBW->Update();
}

//-----------------------------------------------------------------------------
vtkImageData* vtkMyChartXY::GetImageAtIndex(int imageId)
{
  if (this->AxisImageStack)
    {
    // Calculate the center of the volume
    this->AxisImageStack->UpdateInformation();
    int extent[6];
    double spacing[3];
    double origin[3];
    this->AxisImageStack->GetWholeExtent(extent);
    this->AxisImageStack->GetSpacing(spacing);
    this->AxisImageStack->GetOrigin(origin);

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
int vtkMyChartXY::GetNumberOfImages()
{
  if (this->AxisImageStack)
    {
    return this->NumImages;
    }
  else
    {
    return 0;
    }
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::SetAxisImagesVertical()
{
  this->ChartPrivate->aiOrientation = vtkMyChartXY::VERTICAL;
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::SetAxisImagesHorizontal()
{
  this->ChartPrivate->aiOrientation = vtkMyChartXY::HORIZONTAL;
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
