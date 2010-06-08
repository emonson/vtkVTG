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
//#include <stdlib.h>

// My STL containers
#include <vtkstd/vector>

//-----------------------------------------------------------------------------
class vtkMyChartXYPrivate
{
public:
  vtkMyChartXYPrivate()
    {
    this->Colors = vtkSmartPointer<vtkColorSeries>::New();
    this->PlotTransform = vtkSmartPointer<vtkTransform2D>::New();
    }

  vtkstd::vector<vtkPlot *> plots; // Charts can contain multiple plots of data
  vtkSmartPointer<vtkTransform2D> PlotTransform; // Transforms
  vtkstd::vector<vtkAxis *> axes; // Charts can contain multiple axes
  vtkSmartPointer<vtkColorSeries> Colors; // Colors in the chart
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMyChartXY);

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
  
  this->TooltipShowImage = false;
  this->Tooltip = vtkTooltipImageItem::New();
  this->Tooltip->SetShowImage(this->TooltipShowImage);
  this->Tooltip->SetVisible(false);
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
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::Update()
{
  // Perform any necessary updates that are not graphical
  // Update the plots if necessary
  for (size_t i = 0; i < this->ChartPrivate->plots.size(); ++i)
    {
    this->ChartPrivate->plots[i]->Update();
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

  if (geometry[0] != this->Geometry[0] || geometry[1] != this->Geometry[1] ||
      this->MTime > this->ChartPrivate->axes[0]->GetMTime())
    {
    // Take up the entire window right now, this could be made configurable
    this->SetGeometry(geometry);
    // Borders (Left, Right, Top, Bottom)
    this->SetBorders(60, 20, 20, 50);
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
    }

  if (this->ChartPrivate->plots[0]->GetData()->GetInput()->GetMTime() > this->MTime)
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
	painter->PushMatrix();
	painter->AppendTransform(this->ChartPrivate->PlotTransform);

	// Now iterate through the plots
	vtkstd::vector<vtkPlot*>::iterator it =
			this->ChartPrivate->plots.begin();
	for ( ; it != this->ChartPrivate->plots.end(); ++it)
		{
		(*it)->SetSelection(idArray);
		(*it)->Paint(painter);
		}
	painter->PopMatrix();

  // Stop clipping of the plot area and reset back to screen coordinates
  painter->GetDevice()->DisableClipping();
}

//-----------------------------------------------------------------------------
void vtkMyChartXY::RecalculatePlotTransforms()
{
	this->RecalculatePlotTransform(this->ChartPrivate->axes[vtkAxis::BOTTOM],
																 this->ChartPrivate->axes[vtkAxis::LEFT],
																 this->ChartPrivate->PlotTransform);
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
void vtkMyChartXY::RecalculatePlotBounds()
{
  // Get the bounds of each plot, and each axis  - ordering as laid out below
  double y1[] = { 0.0, 0.0 }; // left -> 0
  double x1[] = { 0.0, 0.0 }; // bottom -> 1
  // Store whether the ranges have been initialized - follows same order
  bool initialized[] = { false, false };

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

    // Initialize the appropriate ranges, or push out the ranges

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

  // Now set the newly calculated bounds on the axes
  for (int i = 0; i < 2; ++i)
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
vtkPlot * vtkMyChartXY::AddPlot()
{
  // Use a variable to return the object created (or NULL), this is necessary
  // as the HP compiler is broken (thinks this function does not return) and
  // the MS compiler generates a warning about unreachable code if a redundant
  // return is added at the end.
  vtkPlot *plot = NULL;
  vtkColor3ub color = this->ChartPrivate->Colors->GetColorRepeating(
      static_cast<int>(this->ChartPrivate->plots.size()));

	vtkMyPlotPoints *points = vtkMyPlotPoints::New();
	points->GetPen()->SetColor(color.GetData());
	plot = points;

  // Add the plot to the default corner
  plot->SetXAxis(this->ChartPrivate->axes[vtkAxis::BOTTOM]);
  plot->SetYAxis(this->ChartPrivate->axes[vtkAxis::LEFT]);
  this->ChartPrivate->plots.push_back(plot);
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
    this->ChartPrivate->plots[i]->Delete();
    }
  this->ChartPrivate->plots.clear();
  // Ensure that the bounds are recalculated
  this->PlotTransformValid = false;
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
bool vtkMyChartXY::Hit(const vtkContextMouseEvent &mouse)
{
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
bool vtkMyChartXY::MouseEnterEvent(const vtkContextMouseEvent &)
{
  // Find the nearest point on the curves and snap to it
  this->DrawNearestPoint = true;

  return true;
}

//-----------------------------------------------------------------------------
bool vtkMyChartXY::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  if (mouse.Button == vtkContextMouseEvent::LEFT_BUTTON)
    {
    // Figure out how much the mouse has moved by in plot coordinates - pan
    double screenPos[2] = { mouse.ScreenPos[0], mouse.ScreenPos[1] };
    double lastScreenPos[2] = { mouse.LastScreenPos[0], mouse.LastScreenPos[1] };
    double pos[2] = { 0.0, 0.0 };
    double last[2] = { 0.0, 0.0 };

    // Go from screen to scene coordinates to work out the delta
    this->ChartPrivate->PlotTransform
        ->InverseTransformPoints(screenPos, pos, 1);
    this->ChartPrivate->PlotTransform
        ->InverseTransformPoints(lastScreenPos, last, 1);
    double delta[] = { last[0] - pos[0], last[1] - pos[1] };

    // Now move the axes and recalculate the transform
    vtkAxis* xAxis = this->ChartPrivate->axes[vtkAxis::BOTTOM];
    vtkAxis* yAxis = this->ChartPrivate->axes[vtkAxis::LEFT];
    xAxis->SetMinimum(xAxis->GetMinimum() + delta[0]);
    xAxis->SetMaximum(xAxis->GetMaximum() + delta[0]);
    yAxis->SetMinimum(yAxis->GetMinimum() + delta[1]);
    yAxis->SetMaximum(yAxis->GetMaximum() + delta[1]);

    this->RecalculatePlotTransforms();
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
  else if (mouse.Button == vtkContextMouseEvent::RIGHT_BUTTON)
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
		vtkVector2f position;
		vtkTransform2D* transform = this->ChartPrivate->PlotTransform;
		transform->InverseTransformPoints(mouse.Pos.GetData(),
																			position.GetData(), 1);
		// Use a tolerance of +/- 5 pixels
		vtkVector2f tolerance(5*(1.0/transform->GetMatrix()->GetElement(0, 0)),
													5*(1.0/transform->GetMatrix()->GetElement(1, 1)));
		// Iterate through the visible plots and return on the first hit
		for (int j = static_cast<int>(this->ChartPrivate->plots.size()-1);
				 j >= 0; --j)
			{
			vtkPlot* plot = this->ChartPrivate->plots[j];
			if (plot->GetVisible())
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
					
					if (this->TooltipShowImage)
						{
						int num_images = myPlot->GetNumberOfImages();
						if (num_images > 0)
							{
							this->Tooltip->SetTipImage(myPlot->GetImageAtIndex(static_cast<int>(plotPosAndInd.Z())));
							}
						}
					return true;
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
  if (mouse.Button == vtkContextMouseEvent::LEFT_BUTTON)
    {
    // The mouse panning action.
    return true;
    }
  else if (mouse.Button == vtkContextMouseEvent::MIDDLE_BUTTON)
    {
    // Selection, for now at least...
    this->BoxOrigin[0] = mouse.Pos[0];
    this->BoxOrigin[1] = mouse.Pos[1];
    this->BoxGeometry[0] = this->BoxGeometry[1] = 0.0f;
    this->DrawBox = true;
    return true;
    }
  else if (mouse.Button == vtkContextMouseEvent::RIGHT_BUTTON)
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
  if (mouse.Button == vtkContextMouseEvent::MIDDLE_BUTTON)
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
		vtkTransform2D *transform = this->ChartPrivate->PlotTransform;
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
				this->ChartPrivate->plots.begin();
		for ( ; it != this->ChartPrivate->plots.end(); ++it)
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

    this->InvokeEvent(vtkCommand::SelectionChangedEvent);
    this->BoxGeometry[0] = this->BoxGeometry[1] = 0.0f;
    this->DrawBox = false;
    // Mark the scene as dirty
    this->Scene->SetDirty(true);
    return true;
    }
  if (mouse.Button == vtkContextMouseEvent::RIGHT_BUTTON)
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
