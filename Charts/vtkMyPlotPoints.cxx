/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMyPlotPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMyPlotPoints.h"

#include "vtkContext2D.h"
#include "vtkPen.h"
#include "vtkAxis.h"
#include "vtkContextMapper2D.h"
#include "vtkPoints2D.h"
#include "vtkTable.h"
#include "vtkDataArray.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include "vtkMatrix4x4.h"
#include "vtkImageReslice.h"
#include "vtkLookupTable.h"
#include "vtkImageMapToColors.h"
#include "vtkPointData.h"
#include "vtkVector.h"

#include "vtkstd/vector"
#include "vtkstd/algorithm"

// PIMPL for STL vector...
// This is an older form which should probably be updated to newer version, but
// since this really should be a different class/structure anyway, not worrying for now...
// (Cheating by storing integer index in 3rd float Z point)
class vtkMyPlotPoints::VectorPIMPL3 : public vtkstd::vector<vtkVector3f>
{
public:
  VectorPIMPL3()
    : vtkstd::vector<vtkVector3f>::vector()
  {
  }
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMyPlotPoints);

//-----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkMyPlotPoints, HighlightSelection, vtkIdTypeArray);

//-----------------------------------------------------------------------------
vtkMyPlotPoints::vtkMyPlotPoints()
{
  // Since Sorted is a new type in this class, won't be set to null
  // in the superclass constructor
  this->Sorted3 = NULL;
  
  this->HighlightSelection = NULL;
}

//-----------------------------------------------------------------------------
vtkMyPlotPoints::~vtkMyPlotPoints()
{
  if (this->HighlightSelection)
    {
    this->HighlightSelection->Delete();
    this->HighlightSelection = NULL;
    }
}

//-----------------------------------------------------------------------------
void vtkMyPlotPoints::Update()
{
  // Had to reimplement this so the correct UpdateTableCache would be called...
  if (!this->Visible)
    {
    return;
    }
  // Check if we have an input
  vtkTable *table = this->Data->GetInput();
  if (!table)
    {
    vtkDebugMacro(<< "Update event called with no input table set.");
    return;
    }
  else if(this->Data->GetMTime() > this->BuildTime ||
          table->GetMTime() > this->BuildTime ||
          this->MTime > this->BuildTime)
    {
    vtkDebugMacro(<< "Updating cached values.");
    this->UpdateTableCache(table);
    }
  else if ((this->XAxis && this->XAxis->GetMTime() > this->BuildTime) ||
           (this->YAxis && this->YAxis->GetMaximum() > this->BuildTime))
    {
    if (this->LogX != this->XAxis->GetLogScale() ||
        this->LogY != this->YAxis->GetLogScale())
      {
      this->UpdateTableCache(table);
      }
    }
}

//-----------------------------------------------------------------------------
bool vtkMyPlotPoints::Paint(vtkContext2D *painter)
{
  // Paint superclass methods first
  this->Superclass::Paint(painter);

  float width = this->Pen->GetWidth() * 2.3;
  if (width < 8.0)
    {
    width = 8.0;
    }

// Now add some decorations for our highlighted points...
  if (this->HighlightSelection)
    {
    vtkDebugMacro(<<"HighlightSelection set " << this->HighlightSelection->GetNumberOfTuples());
    for (int i = 0; i < this->HighlightSelection->GetNumberOfTuples(); ++i)
      {
      this->GeneraterMarker(static_cast<int>(width+2.7), true);

      painter->GetPen()->SetColor(0, 128, 255, 154);
      painter->GetPen()->SetWidth(width+2.7);

      vtkIdType id = 0;
      this->HighlightSelection->GetTupleValue(i, &id);
      if (id < this->Points->GetNumberOfPoints())
        {
        double *point = this->Points->GetPoint(id);
        float p[] = { point[0], point[1] };
        painter->DrawPointSprites(this->HighlightMarker, p, 1);
        }
      }
    }
  else
    {
    vtkDebugMacro("No highlight selection set.");
    }

  return true;
}

//-----------------------------------------------------------------------------
namespace
{

// See if the point is within tolerance.
bool inRange23(const vtkVector2f& point, const vtkVector2f& tol,
             const vtkVector3f& current)
{
  if (current.X() > point.X() - tol.X() && current.X() < point.X() + tol.X() &&
      current.Y() > point.Y() - tol.Y() && current.Y() < point.Y() + tol.Y())
    {
    return true;
    }
  else
    {
    return false;
    }
}

// Compare the two vectors, in X component only
bool compVector3fX(const vtkVector3f& v1, const vtkVector3f& v2)
{
  if (v1.X() < v2.X())
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
int vtkMyPlotPoints::GetNearestPoint(const vtkVector2f& point,
                                    const vtkVector2f& tol,
                                    vtkVector2f* location)
{
  // Right now doing a simple bisector search of the array. This should be
  // revisited. Assumes the x axis is sorted, which should always be true for
  // line plots.
  if (!this->Points)
    {
    return -1;
    }
  vtkIdType n = this->Points->GetNumberOfPoints();
  if (n < 2)
    {
    return -1;
    }

  // Sort the data if it has not been done already.  We need to sort it
  // and collect the base and extent into the same vector since both will
  // get involved in range checking.
  // This initial Sorted creation is probably slow compared to newer PlotPoints
  // version which just uses pointers to data, so need to update this sometime
  // when I switch to real non-float/int cheating method...
  if (!this->Sorted3)
    {
    // printf("* * * Rebuilding Sorted3 * * * \n");
    vtkVector2f* data =
        static_cast<vtkVector2f*>(this->Points->GetVoidPointer(0));
    this->Sorted3 = new VectorPIMPL3();
    for (int i = 0; i < n; i++)
      {
      vtkVector3f combined(data[i].X(), data[i].Y(), static_cast<float>(i));
      this->Sorted3->push_back(combined);
      }
    vtkstd::sort(this->Sorted3->begin(), this->Sorted3->end(), compVector3fX);
    }

  // Set up our search array, use the STL lower_bound algorithm
  VectorPIMPL3::iterator low;
  VectorPIMPL3 &v = *this->Sorted3;

  // Get the lowest point we might hit within the supplied tolerance
  vtkVector3f lowPoint(point.X()-tol.X(), 0.0f, 0.0f);
  low = vtkstd::lower_bound(v.begin(), v.end(), lowPoint, compVector3fX);

  // Now consider the y axis
  float highX = point.X() + tol.X();
  while (low != v.end())
    {
    if (inRange23(point, tol, *low))
      {
      // If we're in range, the value that's interesting is the absolute value of 
      // the "wedge" at the closest point, not the base or extent by themselves
      // *location = *low;
      location->SetX((*low).X());
      location->SetY((*low).Y());
      return static_cast<int>((*low).Z());
      }
    else if (low->X() > highX)
      {
      break;
      }
    ++low;
    }
  return -1;

}


//-----------------------------------------------------------------------------
bool vtkMyPlotPoints::UpdateTableCache(vtkTable *table)
{
	this->Superclass::UpdateTableCache(table);
	
  if (this->Sorted3)
    {
    delete this->Sorted3;
    this->Sorted3 = NULL;
    }

  this->BuildTime.Modified();
  return true;
}

//-----------------------------------------------------------------------------
void vtkMyPlotPoints::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
