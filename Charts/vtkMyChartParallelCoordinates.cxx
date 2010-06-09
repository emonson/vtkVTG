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
#include "vtkTextProperty.h"
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

// Minimal storage class for STL containers etc.
class vtkMyChartParallelCoordinates::Private
{
public:
  Private()
    {
    this->Plot = vtkSmartPointer<vtkMyPlotParallelCoordinates>::New();
    this->Transform = vtkSmartPointer<vtkTransform2D>::New();
    this->CurrentAxis = -1;
    this->AxisResize = -1;
    }
  ~Private()
    {
    for (vtkstd::vector<vtkAxis *>::iterator it = this->Axes.begin();
         it != this->Axes.end(); ++it)
      {
      (*it)->Delete();
      }
    }
  vtkSmartPointer<vtkMyPlotParallelCoordinates> Plot;
  vtkSmartPointer<vtkTransform2D> Transform;
  vtkstd::vector<vtkAxis *> Axes;
  vtkstd::vector<vtkVector<float, 2> > AxesSelections;
  int CurrentAxis;
  int AxisResize;
};

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMyChartParallelCoordinates);

//-----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkMyChartParallelCoordinates, HighlightLink, vtkAnnotationLink);

//-----------------------------------------------------------------------------
vtkMyChartParallelCoordinates::vtkMyChartParallelCoordinates()
{
  this->Storage = new vtkMyChartParallelCoordinates::Private;
  this->Storage->Plot->SetParent(this);
  this->GeometryValid = false;
  this->Selection = vtkIdTypeArray::New();
  this->Storage->Plot->SetSelection(this->Selection);
  this->VisibleColumns = vtkStringArray::New();
  this->DrawSets = false;
  this->NumPerSet = 1;
  this->CurrentScale = 0;
  // Link back into chart to highlight selections made in other plots
  this->HighlightLink = NULL;
  this->HighlightSelection = vtkIdTypeArray::New();
  this->Storage->Plot->SetHighlightSelection(this->HighlightSelection);
}

//-----------------------------------------------------------------------------
vtkMyChartParallelCoordinates::~vtkMyChartParallelCoordinates()
{
  this->Storage->Plot->SetSelection(NULL);
  this->Storage->Plot->SetHighlightSelection(NULL);
  delete this->Storage;
  this->Selection->Delete();
  this->HighlightSelection->Delete();
  this->VisibleColumns->Delete();
  if (this->HighlightLink)
    {
    this->HighlightLink->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkMyChartParallelCoordinates::Update()
{
  vtkTable* table = this->Storage->Plot->GetData()->GetInput();
  if (!table)
    {
    return;
    }

  if (table->GetMTime() < this->BuildTime && this->MTime < this->BuildTime)
  {
    return;
  }

  // Now we have a table, set up the axes accordingly, clear and build.
  if (static_cast<int>(this->Storage->Axes.size()) !=
      this->VisibleColumns->GetNumberOfTuples())
    {
    for (vtkstd::vector<vtkAxis *>::iterator it = this->Storage->Axes.begin();
         it != this->Storage->Axes.end(); ++it)
      {
      (*it)->Delete();
      }
    this->Storage->Axes.clear();

    for (int i = 0; i < this->VisibleColumns->GetNumberOfTuples(); ++i)
      {
      vtkAxis* axis = vtkAxis::New();
      axis->SetPosition(vtkAxis::PARALLEL);
      this->Storage->Axes.push_back(axis);
      }
    }

  // Now set up their ranges and locations
  for (int i = 0; i < this->VisibleColumns->GetNumberOfTuples(); ++i)
    {
    double range[2];
    vtkDataArray* array =
        vtkDataArray::SafeDownCast(table->GetColumnByName(this->VisibleColumns->GetValue(i)));
    if (array)
      {
      array->GetRange(range);
      }
    vtkAxis* axis = this->Storage->Axes[i];
    if (axis->GetBehavior() == 0)
      {
      axis->SetMinimum(range[0]);
      axis->SetMaximum(range[1]);
      axis->SetTitle(this->VisibleColumns->GetValue(i));
      }
    }
  this->Storage->AxesSelections.clear();

  this->Storage->AxesSelections.resize(this->Storage->Axes.size());
  this->GeometryValid = false;
  this->BuildTime.Modified();
}

//-----------------------------------------------------------------------------
bool vtkMyChartParallelCoordinates::Paint(vtkContext2D *painter)
{
  if (this->GetScene()->GetViewWidth() == 0 ||
      this->GetScene()->GetViewHeight() == 0 ||
      !this->Visible || !this->Storage->Plot->GetVisible() ||
      this->VisibleColumns->GetNumberOfTuples() < 2)
    {
    // The geometry of the chart must be valid before anything can be drawn
    return false;
    }

  this->Update();
  this->UpdateGeometry();

  // Handle selections
  vtkIdTypeArray *idArray = 0;
  if (this->AnnotationLink)
    {
    vtkSelection *selection = this->AnnotationLink->GetCurrentSelection();
    if (selection->GetNumberOfNodes() &&
        this->AnnotationLink->GetMTime() > this->Storage->Plot->GetMTime())
      {
      vtkSelectionNode *node = selection->GetNode(0);
      idArray = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
      this->Storage->Plot->SetSelection(idArray);
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
        this->HighlightLink->GetMTime() > this->Storage->Plot->GetMTime())
      {
      vtkSelectionNode *node = selection->GetNode(0);
      idArray = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
      this->Storage->Plot->SetHighlightSelection(idArray);
      }
    }
  else
    {
    vtkDebugMacro("No highlight annotation link set.");
    }

  // Draw set rectangles if desired
  if (this->DrawSets)
    {
    int oldLineType = painter->GetPen()->GetLineType();
    
    // Main sets boxes
    painter->GetBrush()->SetColor(254, 209, 0, 40);
    painter->GetPen()->SetLineType(0);
    for (int i = 0; i < this->VisibleColumns->GetNumberOfTuples(); i+=this->NumPerSet)
      {
      if ((i+this->NumPerSet-1) <= static_cast<int>(this->Storage->Axes.size()))
        {
        vtkAxis* axis0 = this->Storage->Axes[i];
        vtkAxis* axis1 = this->Storage->Axes[i+this->NumPerSet-1];
        painter->DrawRect(axis0->GetPoint1()[0], 
                          this->Point1[1],
                          axis1->GetPoint1()[0]-axis0->GetPoint1()[0], 
                          this->Point2[1]-this->Point1[1]);
        }
      }
    
    // Extra set box for current scale
    painter->GetBrush()->SetColor(254, 209, 0, 100);
    painter->GetPen()->SetLineType(0);
    for (int i = 0; i < this->VisibleColumns->GetNumberOfTuples(); i+=this->NumPerSet)
      {
      if ((i+this->NumPerSet-1) <= static_cast<int>(this->Storage->Axes.size()) &&
          (i/this->NumPerSet == this->CurrentScale))
        {
        vtkAxis* axis0 = this->Storage->Axes[i];
        vtkAxis* axis1 = this->Storage->Axes[i+this->NumPerSet-1];
        painter->DrawRect(axis0->GetPoint1()[0], 
                          this->Point1[1],
                          axis1->GetPoint1()[0]-axis0->GetPoint1()[0], 
                          this->Point2[1]-this->Point1[1]);
        }
      }
        
    painter->GetPen()->SetLineType(oldLineType);
    }
    
  // Paint the actual lines of the plot
  painter->PushMatrix();
  painter->SetTransform(this->Storage->Transform);
  this->Storage->Plot->Paint(painter);
  painter->PopMatrix();

  // Now we have a table, set up the axes accordingly, clear and build.
  for (vtkstd::vector<vtkAxis *>::iterator it = this->Storage->Axes.begin();
       it != this->Storage->Axes.end(); ++it)
    {
    (*it)->Paint(painter);
    }

  // If there is a selected axis, draw the highlight
  if (this->Storage->CurrentAxis >= 0)
    {
    painter->GetBrush()->SetColor(200, 200, 200, 150);
    vtkAxis* axis = this->Storage->Axes[this->Storage->CurrentAxis];
    painter->DrawRect(axis->GetPoint1()[0]-5, this->Point1[1],
                      10, this->Point2[1]-this->Point1[1]);
    }

  // Now draw our active selections
  for (size_t i = 0; i < this->Storage->AxesSelections.size(); ++i)
    {
    vtkVector<float, 2> &range = this->Storage->AxesSelections[i];
    if (range[0] != range[1])
      {
      painter->GetBrush()->SetColor(200, 20, 20, 220);
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
  if (this->DrawSets)
    {
    int oldLineType = painter->GetPen()->GetLineType();

    painter->GetBrush()->SetColor(254, 254, 254, 150);
    painter->GetPen()->SetLineType(0);
    int opaquePadding = 4; 	// extra padding so axes and points themselves are covered
    for (int i = 0; i < this->VisibleColumns->GetNumberOfTuples(); i+=this->NumPerSet)
      {
      if ((i+this->NumPerSet-1) <= static_cast<int>(this->Storage->Axes.size()) &&
          (i/this->NumPerSet == this->CurrentScale+2))	// keep next scale visible
        {
        vtkAxis* axis0 = this->Storage->Axes[i];
        vtkAxis* axis1 = this->Storage->Axes[this->Storage->Axes.size()-1];
        painter->DrawRect(axis0->GetPoint1()[0]-opaquePadding, 
                          this->Point1[1]-opaquePadding,
                          axis1->GetPoint1()[0]-axis0->GetPoint1()[0]+(2*opaquePadding),
                          this->Point2[1]-this->Point1[1]+(2*opaquePadding));
        }
      }
    
    painter->GetPen()->SetLineType(oldLineType);
    }
    
  return true;
}

//-----------------------------------------------------------------------------
void vtkMyChartParallelCoordinates::SetColumnVisibility(const char* name,
                                                      bool visible)
{
  if (visible)
    {
    for (vtkIdType i = 0; i < this->VisibleColumns->GetNumberOfTuples(); ++i)
      {
      if (strcmp(this->VisibleColumns->GetValue(i).c_str(), name) == 0)
        {
        // Already there, nothing more needs to be done
        return;
        }
      }
    // Add the column to the end of the list
    this->VisibleColumns->InsertNextValue(name);
    this->Modified();
    this->Update();
    }
  else
    {
    // Remove the value if present
    for (vtkIdType i = 0; i < this->VisibleColumns->GetNumberOfTuples(); ++i)
      {
      if (strcmp(this->VisibleColumns->GetValue(i).c_str(), name) == 0)
        {
        // Move all the later elements down by one, and reduce the size
        while (i < this->VisibleColumns->GetNumberOfTuples()-1)
          {
          this->VisibleColumns->SetValue(i, this->VisibleColumns->GetValue(i+1));
          ++i;
          }
        this->VisibleColumns->SetNumberOfTuples(
            this->VisibleColumns->GetNumberOfTuples()-1);
        this->Modified();
        this->Update();
        return;
        }
      }
    }
}

//-----------------------------------------------------------------------------
bool vtkMyChartParallelCoordinates::GetColumnVisibility(const char* name)
{
  for (vtkIdType i = 0; i < this->VisibleColumns->GetNumberOfTuples(); ++i)
    {
    if (strcmp(this->VisibleColumns->GetValue(i).c_str(), name) == 0)
      {
      return true;
      }
    }
  return false;
}

//-----------------------------------------------------------------------------
vtkPlot * vtkMyChartParallelCoordinates::AddPlot(int)
{
  return NULL;
}

//-----------------------------------------------------------------------------
bool vtkMyChartParallelCoordinates::RemovePlot(vtkIdType)
{
  return false;
}

//-----------------------------------------------------------------------------
void vtkMyChartParallelCoordinates::ClearPlots()
{
}

//-----------------------------------------------------------------------------
vtkPlot* vtkMyChartParallelCoordinates::GetPlot(vtkIdType)
{
  return this->Storage->Plot;
}

//-----------------------------------------------------------------------------
vtkIdType vtkMyChartParallelCoordinates::GetNumberOfPlots()
{
  return 1;
}

//-----------------------------------------------------------------------------
vtkAxis* vtkMyChartParallelCoordinates::GetAxis(int index)
{
  if (index < this->GetNumberOfAxes())
    {
    return this->Storage->Axes[index];
    }
  else
    {
    return NULL;
    }
}

//-----------------------------------------------------------------------------
vtkIdType vtkMyChartParallelCoordinates::GetNumberOfAxes()
{
  return this->Storage->Axes.size();
}

//-----------------------------------------------------------------------------
void vtkMyChartParallelCoordinates::UpdateGeometry()
{
  vtkVector2i geometry(this->GetScene()->GetViewWidth(),
                       this->GetScene()->GetViewHeight());

  if (geometry.X() != this->Geometry[0] || geometry.Y() != this->Geometry[1] ||
      !this->GeometryValid)
    {
    // Take up the entire window right now, this could be made configurable
    this->SetGeometry(geometry.GetData());
    this->SetBorders(60, 20, 20, 50);

    // Iterate through the axes and set them up to span the chart area.
    int xStep = (this->Point2[0] - this->Point1[0]) /
                (static_cast<int>(this->Storage->Axes.size())-1);
    int x =  this->Point1[0];

    for (size_t i = 0; i < this->Storage->Axes.size(); ++i)
      {
      vtkAxis* axis = this->Storage->Axes[i];
      axis->SetPoint1(x, this->Point1[1]);
      axis->SetPoint2(x, this->Point2[1]);
      if (axis->GetBehavior() == 0)
        {
        axis->AutoScale();
        }
      axis->Update();
      x += xStep;
      }

    this->GeometryValid = true;
    // Cause the plot transform to be recalculated if necessary
    this->CalculatePlotTransform();
    this->Storage->Plot->Update();
    }
}

//-----------------------------------------------------------------------------
void vtkMyChartParallelCoordinates::CalculatePlotTransform()
{
  // In the case of parallel coordinates everything is plotted in a normalized
  // system, where the range is from 0.0 to 1.0 in the y axis, and in screen
  // coordinates along the x axis.
  if (!this->Storage->Axes.size())
    {
    return;
    }

  vtkAxis* axis = this->Storage->Axes[0];
  float *min = axis->GetPoint1();
  float *max = axis->GetPoint2();
  float yScale = 1.0f / (max[1] - min[1]);

  this->Storage->Transform->Identity();
  this->Storage->Transform->Translate(0, axis->GetPoint1()[1]);
  // Get the scale for the plot area from the x and y axes
  this->Storage->Transform->Scale(1.0, 1.0 / yScale);
}

//-----------------------------------------------------------------------------
void vtkMyChartParallelCoordinates::RecalculateBounds()
{
  return;
}

//-----------------------------------------------------------------------------
bool vtkMyChartParallelCoordinates::Hit(const vtkContextMouseEvent &mouse)
{
  if (mouse.ScreenPos[0] > this->Point1[0]-10 &&
      mouse.ScreenPos[0] < this->Point2[0]+10 &&
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
bool vtkMyChartParallelCoordinates::MouseEnterEvent(const vtkContextMouseEvent &)
{
  return true;
}

//-----------------------------------------------------------------------------
bool vtkMyChartParallelCoordinates::MouseMoveEvent(const vtkContextMouseEvent &mouse)
{
  if (mouse.Button == vtkContextMouseEvent::LEFT_BUTTON)
    {
    // If an axis is selected, then lets try to narrow down a selection...
    if (this->Storage->CurrentAxis >= 0)
      {
      vtkVector<float, 2> &range =
          this->Storage->AxesSelections[this->Storage->CurrentAxis];

      // Normalize the coordinates
      float current = mouse.ScenePos.Y();
      current -= this->Storage->Transform->GetMatrix()->GetElement(1, 2);
      current /= this->Storage->Transform->GetMatrix()->GetElement(1, 1);

      if (current > 1.0f)
        {
        range[1] = 1.0f;
        }
      else if (current < 0.0f)
        {
        range[1] = 0.0f;
        }
      else
        {
        range[1] = current;
        }
      }
    this->Scene->SetDirty(true);
    }
  else if (mouse.Button == vtkContextMouseEvent::MIDDLE_BUTTON)
    {
    vtkAxis* axis = this->Storage->Axes[this->Storage->CurrentAxis];
    if (this->Storage->AxisResize == 0)
      {
      // Move the axis in x
      float deltaX = mouse.ScenePos.X() - mouse.LastScenePos.X();
      axis->SetPoint1(axis->GetPoint1()[0]+deltaX, axis->GetPoint1()[1]);
      axis->SetPoint2(axis->GetPoint2()[0]+deltaX, axis->GetPoint2()[1]);
      }
    else if (this->Storage->AxisResize == 1)
      {
      // Modify the bottom axis range...
      float deltaY = mouse.ScenePos.Y() - mouse.LastScenePos.Y();
      float scale = (axis->GetPoint2()[1]-axis->GetPoint1()[1]) /
                    (axis->GetMaximum() - axis->GetMinimum());
      axis->SetMinimum(axis->GetMinimum() - deltaY/scale);
      // If there is an active selection on the axis, remove it
      vtkVector<float, 2>& range =
          this->Storage->AxesSelections[this->Storage->CurrentAxis];
      if (range[0] != range[1])
        {
        range[0] = range[1] = 0.0f;
        this->ResetSelection();
        }

      // Now update everything that needs to be
      axis->Update();
      axis->RecalculateTickSpacing();
      this->Storage->Plot->Update();
      }
    else if (this->Storage->AxisResize == 2)
      {
      // Modify the bottom axis range...
      float deltaY = mouse.ScenePos.Y() - mouse.LastScenePos.Y();
      float scale = (axis->GetPoint2()[1]-axis->GetPoint1()[1]) /
                    (axis->GetMaximum() - axis->GetMinimum());
      axis->SetMaximum(axis->GetMaximum() - deltaY/scale);
      // If there is an active selection on the axis, remove it
      vtkVector<float, 2>& range =
          this->Storage->AxesSelections[this->Storage->CurrentAxis];
      if (range[0] != range[1])
        {
        range[0] = range[1] = 0.0f;
        this->ResetSelection();
        }

      axis->Update();
      axis->RecalculateTickSpacing();
      this->Storage->Plot->Update();
      }
    this->Scene->SetDirty(true);
    }

  return true;
}

//-----------------------------------------------------------------------------
bool vtkMyChartParallelCoordinates::MouseLeaveEvent(const vtkContextMouseEvent &)
{
  return true;
}

//-----------------------------------------------------------------------------
bool vtkMyChartParallelCoordinates::MouseButtonPressEvent(
    const vtkContextMouseEvent& mouse)
{
  if (mouse.Button == vtkContextMouseEvent::LEFT_BUTTON)
    {
    // Select an axis if we are within range
    if (mouse.ScenePos[1] > this->Point1[1] &&
        mouse.ScenePos[1] < this->Point2[1])
      {
      // Iterate over the axes, see if we are within 10 pixels of an axis
      for (size_t i = 0; i < this->Storage->Axes.size(); ++i)
        {
        vtkAxis* axis = this->Storage->Axes[i];
        if (axis->GetPoint1()[0]-5 < mouse.ScenePos[0] &&
            axis->GetPoint1()[0]+5 > mouse.ScenePos[0])
          {
          this->Storage->CurrentAxis = static_cast<int>(i);
          vtkVector<float, 2>& range = this->Storage->AxesSelections[i];
          if (range[0] != range[1])
            {
            range[0] = range[1] = 0.0f;
            this->ResetSelection();
            }

          // Transform into normalized coordinates
          float low = mouse.ScenePos[1];
          low -= this->Storage->Transform->GetMatrix()->GetElement(1, 2);
          low /= this->Storage->Transform->GetMatrix()->GetElement(1, 1);
          range[0] = range[1] = low;

          this->Scene->SetDirty(true);
          return true;
          }
        }
      }
    this->Storage->CurrentAxis = -1;
    this->Scene->SetDirty(true);
    return true;
    }
  else if (mouse.Button == vtkContextMouseEvent::MIDDLE_BUTTON)
    {
    // Middle mouse button - move and zoom the axes
    // Iterate over the axes, see if we are within 10 pixels of an axis
    for (size_t i = 0; i < this->Storage->Axes.size(); ++i)
      {
      vtkAxis* axis = this->Storage->Axes[i];
      if (axis->GetPoint1()[0]-10 < mouse.ScenePos[0] &&
          axis->GetPoint1()[0]+10 > mouse.ScenePos[0])
        {
        this->Storage->CurrentAxis = static_cast<int>(i);
        if (mouse.ScenePos.Y() > axis->GetPoint1()[1] &&
            mouse.ScenePos.Y() < axis->GetPoint1()[1] + 20)
          {
          // Resize the bottom of the axis
          this->Storage->AxisResize = 1;
          }
        else if (mouse.ScenePos.Y() < axis->GetPoint2()[1] &&
                 mouse.ScenePos.Y() > axis->GetPoint2()[1] - 20)
          {
          // Resize the top of the axis
          this->Storage->AxisResize = 2;
          }
        else
          {
          // Move the axis
          this->Storage->AxisResize = 0;
          }
        }
      }
    return true;
    }
  else
    {
    return false;
    }
}

//-----------------------------------------------------------------------------
bool vtkMyChartParallelCoordinates::MouseButtonReleaseEvent(
    const vtkContextMouseEvent& mouse)
{
  if (mouse.Button == vtkContextMouseEvent::LEFT_BUTTON)
    {
    if (this->Storage->CurrentAxis >= 0)
      {
      vtkVector<float, 2> &range =
          this->Storage->AxesSelections[this->Storage->CurrentAxis];

      float final = mouse.ScenePos[1];
      final -= this->Storage->Transform->GetMatrix()->GetElement(1, 2);
      final /= this->Storage->Transform->GetMatrix()->GetElement(1, 1);

      // Set the final mouse position
      if (final > 1.0)
        {
        range[1] = 1.0;
        }
      else if (final < 0.0)
        {
        range[1] = 0.0;
        }
      else
        {
        range[1] = final;
        }

      if (range[0] == range[1])
        {
        this->ResetSelection();
        }
      else
        {
        // Add a new selection
        if (range[0] < range[1])
          {
          this->Storage->Plot->SetSelectionRange(this->Storage->CurrentAxis,
                                                 range[0], range[1]);
          }
        else
          {
          this->Storage->Plot->SetSelectionRange(this->Storage->CurrentAxis,
                                                 range[1], range[0]);
          }
        }

      if (this->AnnotationLink)
        {
        vtkSelection* selection = vtkSelection::New();
        vtkSelectionNode* node = vtkSelectionNode::New();
        selection->AddNode(node);
        node->SetContentType(vtkSelectionNode::INDICES);
        node->SetFieldType(vtkSelectionNode::POINT);

        node->SetSelectionList(this->Storage->Plot->GetSelection());
        this->AnnotationLink->SetCurrentSelection(selection);
        selection->Delete();
        node->Delete();
        }
      this->InvokeEvent(vtkCommand::SelectionChangedEvent);
      this->Scene->SetDirty(true);
      }
    return true;
    }
  else if (mouse.Button == vtkContextMouseEvent::MIDDLE_BUTTON)
    {
    this->Storage->CurrentAxis = -1;
    this->Storage->AxisResize = -1;
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkMyChartParallelCoordinates::MouseWheelEvent(const vtkContextMouseEvent &,
                                                  int)
{
  return true;
}

//-----------------------------------------------------------------------------
void vtkMyChartParallelCoordinates::ResetSelection()
{
  // This function takes care of resetting the selection of the chart
  // Reset the axes.
  this->Storage->Plot->ResetSelectionRange();

  // Now set the remaining selections that were kept
  for (size_t i = 0; i < this->Storage->AxesSelections.size(); ++i)
    {
    vtkVector<float, 2> &range = this->Storage->AxesSelections[i];
    if (range[0] != range[1])
      {
      // Process the selected range and display this
      if (range[0] < range[1])
        {
        this->Storage->Plot->SetSelectionRange(static_cast<int>(i),
                                               range[0], range[1]);
        }
      else
        {
        this->Storage->Plot->SetSelectionRange(static_cast<int>(i),
                                               range[1], range[0]);
        }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkMyChartParallelCoordinates::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
