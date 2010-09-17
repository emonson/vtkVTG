/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAxisImageItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAxisImageItem.h"

#include "vtkContext2D.h"
#include "vtkContextView.h"
#include "vtkRenderWindow.h"
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
#include "vtkMyChartXY.h"

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
class vtkAxisImageItemPrivate
{
public:
  vtkAxisImageItemPrivate()
    {
    this->Borders[0] = 20;
    this->Borders[1] = 20;
    this->Borders[2] = 20;
    this->Borders[3] = 20;

    this->aiScalingFactor = 1.0;
    this->aiWidth = 0;
    this->aiHeight = 0;
    this->aiOrientation = vtkAxisImageItem::VERTICAL;	// set default here
    this->aiGap = 10;			// Set default here
    this->aiXSpace = 40;	// Set default here : used if axis images vertical
    this->aiYSpace = 60;	// Set default here : used if axis images horizontal
    this->currentXai = 0;
    this->currentYai = 1;
    }

  int Borders[4];

  vtkstd::vector<vtkAxisImagePrivate *> axisImages;
  float aiScalingFactor;
  int aiWidth, aiHeight, aiGap, aiXSpace, aiYSpace;
  int currentXai, currentYai;
  int aiOrientation;
  vtkstd::vector<vtkIdType> col_idxs;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAxisImageItem);

//-----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkAxisImageItem, DataColumnsLink, vtkAnnotationLink);
vtkCxxSetObjectMacro(vtkAxisImageItem, ChartXYView, vtkContextView);

//-----------------------------------------------------------------------------
vtkAxisImageItem::vtkAxisImageItem()
{
  this->Geometry[0] = 0;
  this->Geometry[1] = 0;
  this->Point1[0] = 0;
  this->Point1[1] = 0;
  this->Point2[0] = 0;
  this->Point2[1] = 0;

  this->AIPrivate = new vtkAxisImageItemPrivate;
  this->AxisImageStack = NULL;
  this->NumImages = 0;
  this->CenterImage = NULL;

  // Link into chart to externally control which columns are plotted
  this->DataColumnsLink = NULL;

  // Link into chart to externally control which columns are plotted
  this->ChartXY = NULL;
  this->ChartXYView = NULL;

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
vtkAxisImageItem::~vtkAxisImageItem()
{
  for (unsigned int i = 0; i < this->AIPrivate->axisImages.size(); ++i)
    {
    delete this->AIPrivate->axisImages[i];
    }
  delete this->AIPrivate;
  this->AIPrivate = 0;

  if (this->DataColumnsLink)
    {
    this->DataColumnsLink->Delete();
    }
}

//-----------------------------------------------------------------------------
bool vtkAxisImageItem::Paint(vtkContext2D *painter)
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

  // NOTE: using this in DrawImage for center image for now -- should set for real...
  //  partly because borders now have to be set here and in SetBorders() call later
  // NOTE: There's also something strange here -- if data is changed, then if I don't have
  //  the preliminary positions set for the axis images, then they'll be placed
  //  at the origin until the first render...
    
	int origin[2] = {0,0};
	// This first call takes care of center image right after data change...
	if (this->AIPrivate->aiOrientation == vtkAxisImageItem::VERTICAL)
		{
		origin[0] = 20;
		origin[1] = 20;
		}
	else
		{
		origin[0] = 20;
		origin[1] = 20;
		}
	
  if (geometry[0] != this->Geometry[0] || geometry[1] != this->Geometry[1])
    {
    // Take up the entire window right now, this could be made configurable
    this->SetGeometry(geometry);
    // Borders (Left, Right, Top, Bottom)
    // I think the new API uses (Left, Bottom, Right, Top)
    this->SetBorders(this->AIPrivate->Borders[0],
                     this->AIPrivate->Borders[1],
                     this->AIPrivate->Borders[2],
                     this->AIPrivate->Borders[3]);
  
    if (this->AxisImageStack)
      {
			// NOTE: Leaving space here for center image (placed below axis images for now)
			//   which should always be the same size as axis images
			// Set initial scaling factor even before Paint for initial positions
			
			// Convenience assignments for shorter equations
			float pH = this->Point2[1] - this->Point1[1];	// pixel height of panel
			float pW = this->Point2[0] - this->Point1[0];	// pixel width of panel
			// Since there are borders usable space can go negative -- prevent detection of this
			if (pH < 1.0) pH = 1.0;
			if (pW < 1.0) pW = 1.0;
			float S = 0.0;	// scaling factor
			float G = this->AIPrivate->aiGap;
			float H = this->AIPrivate->aiHeight;
			float W = this->AIPrivate->aiWidth;
			// Want to peg the min scaling factor at 1 px for smallest dimension
			// since scaling factor calc can go negative with fixed (not proportional) gaps
			float minS = (1.0/G > 1.0/W) ? (1.0/G) : (1.0/W);
			int nY = 0;		// n-up in Y direction
			int nX = 0;		// n-up in X direction		
			
			if (this->AIPrivate->aiOrientation == vtkAxisImageItem::HORIZONTAL)
				{
				// For horizontal-flowing images, pixel height constrains scaling factor (S)
				// and how many can be placed on each row (nX)
				
				float prevS = -1.0;
				int prevNx = 0;
				// Increment nY and see when we hit the max in S
				while ((S-0.00001) > prevS)
				 {
				 nY++;
				 prevS = S;
				 prevNx = nX;
				 S = (pH - float((nY-1)*G))/((float)nY*H);
				 if (S < minS) S = minS;
				 nX = floor( (pW + G)/(W*S + G) );
				 // Now calculate whether all of the images will fit with that
				 // scale and nY
				 if (nX*nY < (this->NumImages+1))
				 	{
				 	// max number can fit in a row with this nY
				 	// nX = this->NumImages/nY + 1;
				 	nX = ceil(float(this->NumImages+1)/float(nY));
				 	S = (pW - float((nX-1)*G))/((float)nX*W);
				 	if (S < minS) S = minS;
				 	}
				 }
				// Went one past, so reset S to max values
				nY--;
				S = prevS;
				this->AIPrivate->aiScalingFactor = S;
				
				// Now figure out how many can fit in each row
				// nX = floor( (pW + G)/(W*S + G) );
				// nX = ceil(float(this->NumImages+1)/float(nY));
				nX = prevNx;
				}
			else
				{
				// TODO: Fill in VERTICAL routine...
				}
				
			int upper_left[2] = {this->Point1[0],this->Point2[1]};
			float scW = this->AIPrivate->aiWidth*this->AIPrivate->aiScalingFactor;
			float scH = this->AIPrivate->aiHeight*this->AIPrivate->aiScalingFactor;
			
			for (int ii = 0; ii < this->NumImages; ii++) 
				{
				vtkAxisImagePrivate *ai = this->AIPrivate->axisImages[ii];
				if (this->AIPrivate->aiOrientation == vtkAxisImageItem::HORIZONTAL)
					{
					int xi = (ii+1) % nX;
					int yi = (ii+1) / nX;
					
					ai->Point1[0] = upper_left[0] + xi*scW + xi*G;
					ai->Point1[1] = upper_left[1] - (yi+1)*scH - yi*G;
					}
				else
					{
					// TODO: Fill in VERTICAL routine...
					}
				
				ai->Point2[0] = ai->Point1[0] + scW;
				ai->Point2[1] = ai->Point1[1] + scH;
				}
			}
    }

  // Draw the XY association graphics behind axis images
  if (this->AxisImageStack)
    {
		if (this->AIPrivate->aiOrientation == vtkAxisImageItem::VERTICAL)
			{
			int xI = this->AIPrivate->currentXai;
			int yI = this->AIPrivate->currentYai;
			int x0 = this->AIPrivate->axisImages[0]->Point1[0] - 5;
			int y0 = this->AIPrivate->axisImages[xI]->Point1[1] + static_cast<int>((float)this->AIPrivate->aiHeight*this->AIPrivate->aiScalingFactor/2.0);
			int x1 = this->AIPrivate->axisImages[0]->Point2[0] + 5;
			int y1 = this->AIPrivate->axisImages[yI]->Point1[1] + static_cast<int>((float)this->AIPrivate->aiHeight*this->AIPrivate->aiScalingFactor/2.0);
			painter->GetBrush()->SetColor(255, 255, 255, 0);
			painter->GetPen()->SetColor(0, 0, 0, 100);
			painter->GetPen()->SetWidth(2.0);
			painter->DrawLine(x0,y0,x0+10,y0);
			painter->DrawLine(x0,y1,x0+10,y1);
			painter->DrawLine(x0,y0,x0,y1);
			}
		else
			{
			int xI = this->AIPrivate->currentXai;
			int x0 = this->AIPrivate->axisImages[xI]->Point1[0];
			int y0 = this->AIPrivate->axisImages[xI]->Point1[1]-2;
			int x1 = this->AIPrivate->axisImages[xI]->Point2[0];
			int y1 = this->AIPrivate->axisImages[xI]->Point1[1]-2;
			painter->GetBrush()->SetColor(255, 255, 255, 0);
			painter->GetPen()->SetColor(0, 0, 0, 100);
			painter->GetPen()->SetWidth(2.0);
			painter->DrawLine(x0,y0,x1,y1);

			int yI = this->AIPrivate->currentYai;
			x0 = this->AIPrivate->axisImages[yI]->Point1[0]-2;
			y0 = this->AIPrivate->axisImages[yI]->Point1[1];
			x1 = this->AIPrivate->axisImages[yI]->Point1[0]-2;
			y1 = this->AIPrivate->axisImages[yI]->Point2[1];
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
      painter->DrawImage(this->AIPrivate->axisImages[ii]->Point1[0],
      		this->AIPrivate->axisImages[ii]->Point1[1],
      		this->AIPrivate->aiScalingFactor,
      		this->AIPrivate->axisImages[ii]->Image);
      }
    
    if (this->CenterImage)
      {
      painter->DrawImage(this->Point1[0], 
          this->Point2[1] - this->AIPrivate->aiHeight*this->AIPrivate->aiScalingFactor,
      		this->AIPrivate->aiScalingFactor,
      		this->colorBW->GetOutput());      
      }
    }

//   if (this->ChartXY && this->ChartXYView && this->ChartXY->GetMTime() > this->GetMTime())
//   	{
//   	this->ChartXYView->Update();
//   	}
//   	
  return true;
}

//-----------------------------------------------------------------------------
void vtkAxisImageItem::ClearAxisImages()
{
  for (unsigned int i = 0; i < this->AIPrivate->axisImages.size(); ++i)
    {
    delete this->AIPrivate->axisImages[i];
    }
  this->AIPrivate->axisImages.clear();
	this->AIPrivate->col_idxs.clear();
	
	this->AxisImageStack = NULL;

  // Mark the scene as dirty
  this->Scene->SetDirty(true);
}

//-----------------------------------------------------------------------------
void vtkAxisImageItem::SetScene(vtkContextScene *scene)
{
  this->vtkContextItem::SetScene(scene);
}

//-----------------------------------------------------------------------------
bool vtkAxisImageItem::Hit(const vtkContextMouseEvent &mouse)
{
  int maxI = this->NumImages-1;
  // In axis images area
	if (mouse.ScreenPos[0] > this->Point1[0] &&
					 mouse.ScreenPos[0] < this->Point2[0] &&
					 mouse.ScreenPos[1] > this->Point1[1] &&
					 mouse.ScreenPos[1] < this->Point2[1])
		{
		return true;
		}
	else
		{
		return false;
		}
}

//-----------------------------------------------------------------------------
bool vtkAxisImageItem::MouseEnterEvent(const vtkContextMouseEvent &)
{
  return true;
}

//-----------------------------------------------------------------------------
bool vtkAxisImageItem::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  if (mouse.Button == vtkContextMouseEvent::RIGHT_BUTTON)
    {
    }
  else if (mouse.Button == vtkContextMouseEvent::LEFT_BUTTON)
    {
    }
  else if (mouse.Button == vtkContextMouseEvent::MIDDLE_BUTTON)
    {
    }
  else if (mouse.Button < 0)
    {
    }

  return true;
}

//-----------------------------------------------------------------------------
bool vtkAxisImageItem::MouseLeaveEvent(const vtkContextMouseEvent &)
{
  return true;
}

//-----------------------------------------------------------------------------
bool vtkAxisImageItem::MouseButtonPressEvent(const vtkContextMouseEvent &mouse)
{
  if (mouse.Button == vtkContextMouseEvent::RIGHT_BUTTON)
    {
    // The mouse panning action.
    return true;
    }
  else if (mouse.Button == vtkContextMouseEvent::LEFT_BUTTON)
    {
		// Iterate over the axis images, see if we are inside of one
		for (size_t i = 0; i < this->AIPrivate->axisImages.size(); ++i)
			{
			vtkAxisImagePrivate* image = this->AIPrivate->axisImages[i];
			if (mouse.ScreenPos[0] > image->Point1[0] && mouse.ScreenPos[0] < image->Point2[0] &&
					mouse.ScreenPos[1] > image->Point1[1] && mouse.ScreenPos[1] < image->Point2[1])
				{
				bool modify_data;
				if (i > this->AIPrivate->currentYai)
				  {
						this->AIPrivate->currentYai = i;
						this->AIPrivate->currentXai = i-1;
						modify_data = true;
				  }
				else if (i < this->AIPrivate->currentXai)
				  {
						this->AIPrivate->currentYai = i+1;
						this->AIPrivate->currentXai = i;
						modify_data = true;
					}
				else
				  {
				  modify_data = false;
				  }
				if (modify_data)
				  {
				  this->UpdateChartAxes();
				  }
				// Not sure why return true or false
				return false;
				}
			}
		return true;

    }
  else if (mouse.Button == vtkContextMouseEvent::MIDDLE_BUTTON)
    {
    return true;
    }
  else
    {
    return false;
    }
}

//-----------------------------------------------------------------------------
bool vtkAxisImageItem::MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse)
{
  if (mouse.Button == vtkContextMouseEvent::RIGHT_BUTTON)
    {
    return true;
    }
  if (mouse.Button == vtkContextMouseEvent::LEFT_BUTTON)
    {
    return true;
    }
  if (mouse.Button == vtkContextMouseEvent::MIDDLE_BUTTON)
    {
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkAxisImageItem::MouseWheelEvent(const vtkContextMouseEvent &, int delta)
{
  return true;
}

//-----------------------------------------------------------------------------
void vtkAxisImageItem::SetChartXY(vtkMyChartXY* chart)
{
	this->ChartXY = chart;
	this->SetColumnIndices();
}

//-----------------------------------------------------------------------------
void vtkAxisImageItem::UpdateChartAxes()
{
	// Direct chart control method of chaning plotted data
	
	// Looking for first visible vtkMyPlotPoints
	for (vtkIdType p = 0; p < this->ChartXY->GetNumberOfPlots(); ++p)
		{
		vtkMyPlotPoints* plot = vtkMyPlotPoints::SafeDownCast(this->ChartXY->GetPlot(p));
		if (plot && plot->GetVisible())
			{
			vtkTable* table = plot->GetData()->GetInput();
			int yI = this->AIPrivate->axisImages[this->AIPrivate->currentYai]->ColumnIndex;
			int xI = this->AIPrivate->axisImages[this->AIPrivate->currentXai]->ColumnIndex;
			// plot->SetInputArray(0,table->GetColumnName(xI));
			// plot->SetInputArray(1,table->GetColumnName(yI));
			// printf("Setting input %d, %d\n", xI, yI);
			// printf("Names %s, %s\n", table->GetColumnName(xI), table->GetColumnName(yI));
			plot->SetInput(table,xI,yI);
			this->ChartXY->GetAxis(0)->SetTitle(table->GetColumnName(yI));
			this->ChartXY->GetAxis(0)->Modified();
			this->ChartXY->GetAxis(1)->SetTitle(table->GetColumnName(xI));
			this->ChartXY->GetAxis(1)->Modified();
			this->ChartXY->Modified();
			plot->Modified();
			plot->Update();
			this->ChartXY->RecalculateBounds();
			this->ChartXY->Update();
			this->Scene->SetDirty(true);
			if (this->ChartXYView)
				{
				// this->ChartXYView->Update();
				this->ChartXYView->Render();
				}
			break;
			}
		}
}

//-----------------------------------------------------------------------------
void vtkAxisImageItem::SetColumnIndices()
{
	if (!this->ChartXY)
		{
		return;
		}
	else
		{
		// Look through data table column names to gather valid data column indices
		// NOTE: Looking for first visible vtkMyPlotPoints and using that table
		
		// TODO: Either part of this check needs to move into vtkMyChartXY so
		//   when it's sent indices from the axis images, it maps them properly
		//   into non-_ids column indices, or I need to register the chart with 
		//   this class so I can do this check here and send the proper indices
		//   to the chart...
		vtkIdType n = this->ChartXY->GetNumberOfPlots();
		vtkMyPlotPoints* plot;
		vtkTable* table;
		// Find first MyPlotPoints to know where to grab the input table
		for (vtkIdType i = 0; i < n; ++i)
			{
			plot = vtkMyPlotPoints::SafeDownCast(this->ChartXY->GetPlot(i));
			if (plot && plot->GetVisible())
				{
				table = plot->GetData()->GetInput();
				break;
				}
			}
		// Check whether the table has columns
		if (!plot || !(table->GetNumberOfColumns()>0))
			{
			printf("No viable plot found in SetChartXY\n");
			return;
			}
		// Grab current X and Y plotted column names
		const char* xName = plot->GetData()->GetInputArrayToProcess(0, table)->GetName();
		const char* yName = plot->GetData()->GetInputArrayToProcess(1, table)->GetName();
	
		// Build up a vector of table column indices which do not contain _ids in their name
		this->AIPrivate->col_idxs.clear();
		for (vtkIdType ii = 0; ii < table->GetNumberOfColumns(); ii++)
			{
			const char *col_name = table->GetColumnName(ii);
			if (strcmp(col_name, xName) == 0) 
				{
				this->AIPrivate->currentXai = ii;
				}
			if (strcmp(col_name, yName) == 0)
				{
				this->AIPrivate->currentYai = ii;
				}
			if (strstr(col_name, "_ids"))
				{
				continue;
				}
			else
				{
				this->AIPrivate->col_idxs.push_back(ii);
				}
			}
		// Check whether the vector of indices has the same length as the number of axis images
		if (this->AIPrivate->col_idxs.size() != this->NumImages &&
				this->AIPrivate->axisImages.size() > 0)
			{
			printf("Number of viable columns doesn't equal the number of images!\n");
			printf("Columns: %d, Images: %d\n", (int)this->AIPrivate->col_idxs.size(), this->NumImages);
			}
		
		// TODO: if (this->AIPrivate->axis_images.size() > 0), iterate through axis
		// images and set column index values
	}
}

//-----------------------------------------------------------------------------
void vtkAxisImageItem::SetAxisImageStack(vtkImageData* stack)
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
  // Storing actual number of pixels in width and height
  this->AIPrivate->aiWidth = extent[1]+1;		// extent is largest 0-based index
  this->AIPrivate->aiHeight = extent[3]+1;

	// If we've kept the same chart, but switched AxisImageStack, need to recompute	
	if (this->ChartXY)
		{
		SetColumnIndices();
		}

	// NOTE: Leaving space here for center image (placed below axis images for now)
	//   which should always be the same size as axis images
	// Set initial scaling factor even before Paint for initial positions
	
			// Convenience assignments for shorter equations
			float pH = this->Point2[1] - this->Point1[1];	// pixel height of panel
			float pW = this->Point2[0] - this->Point1[0];	// pixel width of panel
			// Since there are borders usable space can go negative -- prevent detection of this
			if (pH < 1.0) pH = 1.0;
			if (pW < 1.0) pW = 1.0;
			float S = 0.0;	// scaling factor
			float G = this->AIPrivate->aiGap;
			float H = this->AIPrivate->aiHeight;
			float W = this->AIPrivate->aiWidth;
			// Want to peg the min scaling factor at 1 px for smallest dimension
			// since scaling factor calc can go negative with fixed (not proportional) gaps
			float minS = (1.0/G > 1.0/W) ? (1.0/G) : (1.0/W);
			int nY = 0;		// n-up in Y direction
			int nX = 0;		// n-up in X direction		
			
			if (this->AIPrivate->aiOrientation == vtkAxisImageItem::HORIZONTAL)
				{
				// For horizontal-flowing images, pixel height constrains scaling factor (S)
				// and how many can be placed on each row (nX)
				
				float prevS = -1.0;
				int prevNx = 0;
				// Increment nY and see when we hit the max in S
				while ((S-0.00001) > prevS)
				 {
				 nY++;
				 prevS = S;
				 prevNx = nX;
				 S = (pH - float((nY-1)*G))/((float)nY*H);
				 if (S < minS) S = minS;
				 nX = floor( (pW + G)/(W*S + G) );
				 // Now calculate whether all of the images will fit with that
				 // scale and nY
				 if (nX*nY < (this->NumImages+1))
				 	{
				 	// max number can fit in a row with this nY
				 	// nX = this->NumImages/nY + 1;
				 	nX = ceil(float(this->NumImages+1)/float(nY));
				 	S = (pW - float((nX-1)*G))/((float)nX*W);
				 	if (S < minS) S = minS;
				 	}
				 }
				// Went one past, so reset S to max values
				nY--;
				S = prevS;
				this->AIPrivate->aiScalingFactor = S;
				
				// Now figure out how many can fit in each row
				// nX = floor( (pW + G)/(W*S + G) );
				// nX = ceil(float(this->NumImages+1)/float(nY));
				nX = prevNx;
				}
			else
				{
				// TODO: Fill in VERTICAL routine...
				}
		
	// Create the vector of axisImage objects
	this->AIPrivate->axisImages.clear();
	int upper_left[2] = {this->Point1[0],this->Point2[1]};
	float scW = this->AIPrivate->aiWidth*this->AIPrivate->aiScalingFactor;
	float scH = this->AIPrivate->aiHeight*this->AIPrivate->aiScalingFactor;
	
	for (int ii = 0; ii < this->NumImages; ii++) 
		{
		vtkAxisImagePrivate *ai = new vtkAxisImagePrivate;
		ai->Image->DeepCopy(this->GetImageAtIndex(ii));
		// Real positions set in Paint() routine to adjust for scene geometry
		// NOTE: Doesn't crash if you remove this, but for some reason images show
		//   up at origin until next render (mouse enter) if this is deleted...
		// Adding one imHeight and gap to origin[1] to leave room for center image
		
		if (this->AIPrivate->aiOrientation == vtkAxisImageItem::HORIZONTAL)
			{
			int xi = (ii+1) % nX;
			int yi = (ii+1) / nX;
			
			ai->Point1[0] = upper_left[0] + xi*scW + xi*G;
			ai->Point1[1] = upper_left[1] - (yi+1)*scH - yi*G;
			}
		else
			{
			// TODO: Fill in VERTICAL routine...
			}
		
		ai->Point2[0] = ai->Point1[0] + scW;
		ai->Point2[1] = ai->Point1[1] + scH;
		
		// If chart has been registered, record the actual column index
		if (this->AIPrivate->col_idxs.size() > 0)
			{
			ai->ColumnIndex = this->AIPrivate->col_idxs.at(ii);
			}
		else
			{
			ai->ColumnIndex = ii;
			}
		this->AIPrivate->axisImages.push_back(ai);
		}
}

//-----------------------------------------------------------------------------
void vtkAxisImageItem::SetCenterImage(vtkImageData* image)
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
vtkImageData* vtkAxisImageItem::GetImageAtIndex(int imageId)
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
int vtkAxisImageItem::GetNumberOfImages()
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
void vtkAxisImageItem::SetAxisImagesVertical()
{
  this->AIPrivate->aiOrientation = vtkAxisImageItem::VERTICAL;
}

//-----------------------------------------------------------------------------
void vtkAxisImageItem::SetAxisImagesHorizontal()
{
  this->AIPrivate->aiOrientation = vtkAxisImageItem::HORIZONTAL;
}

//-----------------------------------------------------------------------------
void vtkAxisImageItem::SetBottomBorder(int border)
{
  this->Point1[1] = border >= 0 ? border : 0;
}

//-----------------------------------------------------------------------------
void vtkAxisImageItem::SetTopBorder(int border)
{
 this->Point2[1] = border >=0 ?
                   this->Geometry[1] - border :
                   this->Geometry[1];
}

//-----------------------------------------------------------------------------
void vtkAxisImageItem::SetLeftBorder(int border)
{
  this->Point1[0] = border >= 0 ? border : 0;
}

//-----------------------------------------------------------------------------
void vtkAxisImageItem::SetRightBorder(int border)
{
  this->Point2[0] = border >=0 ?
                    this->Geometry[0] - border :
                    this->Geometry[0];
}

//-----------------------------------------------------------------------------
void vtkAxisImageItem::SetBorders(int left, int bottom, int right, int top)
{
  this->SetLeftBorder(left);
  this->SetRightBorder(right);
  this->SetTopBorder(top);
  this->SetBottomBorder(bottom);
}

//-----------------------------------------------------------------------------
void vtkAxisImageItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
