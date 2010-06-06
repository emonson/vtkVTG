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
vtkMyPlotPoints::vtkMyPlotPoints()
{
  this->Points = NULL;
  this->Sorted = NULL;
  this->BadPoints = NULL;
  this->MarkerStyle = vtkMyPlotPoints::NONE;
  this->LogX = false;
  this->LogY = false;
  this->Marker = NULL;
  this->HighlightMarker = NULL;
  this->ImageStack = NULL;
  this->NumImages = 0;

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
  this->reslice->SetInterpolationModeToLinear();
  // Need to set the Input when ImageStack is assigned

  // Create a greyscale lookup table
  this->lut = vtkSmartPointer<vtkLookupTable>::New();
  this->lut->SetRange(0, 1); 			// image intensity range
  this->lut->SetValueRange(0.0, 1.0); 		// from black to white
  this->lut->SetSaturationRange(0.0, 0.0); 	// no color saturation
  this->lut->SetRampToLinear();
  this->lut->Build();

  // Map the image through the lookup table
  this->color = vtkSmartPointer<vtkImageMapToColors>::New();
  this->color->SetLookupTable(this->lut);
  this->color->SetInputConnection(this->reslice->GetOutputPort());

}

//-----------------------------------------------------------------------------
vtkMyPlotPoints::~vtkMyPlotPoints()
{
  if (this->Points)
    {
    this->Points->Delete();
    this->Points = NULL;
    }
  delete this->Sorted;
  if (this->BadPoints)
    {
    this->BadPoints->Delete();
    this->BadPoints = NULL;
    }
  if (this->Marker)
    {
    this->Marker->Delete();
    }
  if (this->HighlightMarker)
    {
    this->HighlightMarker->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkMyPlotPoints::Update()
{
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
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called in vtkMyPlotPoints.");

  if (!this->Visible || !this->Points)
    {
    return false;
    }

  float width = this->Pen->GetWidth() * 2.3;
  if (width < 8.0)
    {
    width = 8.0;
    }

  // Now add some decorations for our selected points...
  if (this->Selection)
    {
    vtkDebugMacro(<<"Selection set " << this->Selection->GetNumberOfTuples());
    for (int i = 0; i < this->Selection->GetNumberOfTuples(); ++i)
      {
      this->GeneraterMarker(static_cast<int>(width+2.7), true);

      painter->GetPen()->SetColor(255, 50, 0, 255);
      painter->GetPen()->SetWidth(width+2.7);

      vtkIdType id = 0;
      this->Selection->GetTupleValue(i, &id);
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
    vtkDebugMacro("No selection set.");
    }

  // If there is a marker style, then draw the marker for each point too
  if (this->MarkerStyle)
    {
    this->GeneraterMarker(static_cast<int>(width));
    painter->ApplyPen(this->Pen);
    painter->ApplyBrush(this->Brush);
    painter->GetPen()->SetWidth(width);
    painter->DrawPointSprites(this->Marker, this->Points);
    }

  return true;
}

//-----------------------------------------------------------------------------
void vtkMyPlotPoints::GeneraterMarker(int width, bool highlight)
{
  // Set up the image data, if highlight then the mark shape is different
  vtkImageData *data = 0;

  if (!highlight)
    {
    if (!this->Marker)
      {
      this->Marker = vtkImageData::New();
      this->Marker->SetScalarTypeToUnsignedChar();
      this->Marker->SetNumberOfScalarComponents(4);
      }
    else
      {
      if (this->Marker->GetMTime() >= this->GetMTime() &&
          this->Marker->GetMTime() >= this->Pen->GetMTime())
        {
        // Marker already generated, no need to do this again.
        return;
        }
      }
    data = this->Marker;
    }
  else
    {
    if (!this->HighlightMarker)
      {
      this->HighlightMarker = vtkImageData::New();
      this->HighlightMarker->SetScalarTypeToUnsignedChar();
      this->HighlightMarker->SetNumberOfScalarComponents(4);
      data = this->HighlightMarker;
      }
    else
      {
      if (this->HighlightMarker->GetMTime() >= this->GetMTime() &&
          this->HighlightMarker->GetMTime() >= this->Pen->GetMTime())
        {
        // Marker already generated, no need to do this again.
        return;
        }
      }
    data = this->HighlightMarker;
    }

  data->SetExtent(0, width-1, 0, width-1, 0, 0);
  data->AllocateScalars();
  unsigned char* image =
      static_cast<unsigned char*>(data->GetScalarPointer());

  // Generate the marker image at the required size
  switch (this->MarkerStyle)
    {
    case vtkMyPlotPoints::CROSS:
      {
      for (int i = 0; i < width; ++i)
        {
        for (int j = 0; j < width; ++j)
          {
          unsigned char color = 0;

          if (highlight)
            {
            if ((i >= j-1 && i <= j+1) || (i >= width-j-1 && i <= width-j+1))
              {
              color = 255;
              }
            }
          else
            {
            if (i == j || i == width-j)
              {
              color = 255;
              }
            }
          image[4*width*i + 4*j] = image[4*width*i + 4*j + 1] =
                                   image[4*width*i + 4*j + 2] = color;
          image[4*width*i + 4*j + 3] = color;
          }
        }
      break;
      }
    case vtkMyPlotPoints::PLUS:
      {
      int x = width / 2;
      int y = width / 2;
      for (int i = 0; i < width; ++i)
        {
        for (int j = 0; j < width; ++j)
          {
          unsigned char color = 0;
          if (i == x || j == y)
            {
            color = 255;
            }
          if (highlight)
            {
            if (i == x-1 || i == x+1 || j == y-1 || j == y+1)
              {
              color = 255;
              }
            }
          image[4*width*i + 4*j] = image[4*width*i + 4*j + 1] =
                                   image[4*width*i + 4*j + 2] = color;
          image[4*width*i + 4*j + 3] = color;
          }
        }
      break;
      }
    case vtkMyPlotPoints::SQUARE:
      {
      for (int i = 0; i < width; ++i)
        {
        for (int j = 0; j < width; ++j)
          {
          unsigned char color = 255;
          image[4*width*i + 4*j] = image[4*width*i + 4*j + 1] =
                                   image[4*width*i + 4*j + 2] = color;
          image[4*width*i + 4*j + 3] = color;
          }
        }
      break;
      }
    case vtkMyPlotPoints::CIRCLE:
      {
      double c = width/2.0;
      for (int i = 0; i < width; ++i)
        {
        double dx2 = (i - c)*(i-c);
        for (int j = 0; j < width; ++j)
          {
          double dy2 = (j - c)*(j - c);
          unsigned char color = 0;
          if ((c-sqrt(dx2 + dy2)) < 1.0)
            {
            color = 255;
            }
          image[4*width*i + 4*j] = image[4*width*i + 4*j + 1] =
                                   image[4*width*i + 4*j + 2] = color;
          image[4*width*i + 4*j + 3] = color;
          }
        }
      break;
      }
    case vtkMyPlotPoints::DIAMOND:
      {
      int c = width/2;
      for (int i = 0; i < width; ++i)
        {
        int dx = i-c > 0 ? i-c : c-i;
        for (int j = 0; j < width; ++j)
          {
          int dy = j-c > 0 ? j-c : c-j;
          unsigned char color = 0;
          if (c-dx >= dy)
            {
            color = 255;
            }
          image[4*width*i + 4*j] = image[4*width*i + 4*j + 1] =
                                   image[4*width*i + 4*j + 2] = color;
          image[4*width*i + 4*j + 3] = color;
          }
        }
      break;
      }
    default:
      {
      int x = width / 2;
      int y = width / 2;
      for (int i = 0; i < width; ++i)
        {
        for (int j = 0; j < width; ++j)
          {
          unsigned char color = 0;
          if (i == x || j == y)
            {
            color = 255;
            }
          image[4*width*i + 4*j] = image[4*width*i + 4*j + 1] =
                                   image[4*width*i + 4*j + 2] = color;
          image[4*width*i + 4*j + 3] = color;
          }
        }
      }
    }
}

//-----------------------------------------------------------------------------
bool vtkMyPlotPoints::PaintLegend(vtkContext2D *painter, float rect[4])
{
  painter->ApplyPen(this->Pen);
  painter->DrawLine(rect[0], rect[1]+0.5*rect[3],
                    rect[0]+rect[2], rect[1]+0.5*rect[3]);
  return true;
}

//-----------------------------------------------------------------------------
void vtkMyPlotPoints::GetBounds(double bounds[4])
{
  if (this->Points)
    {
    if (!this->BadPoints)
      {
      this->Points->GetBounds(bounds);
      }
    else
      {
      // There are bad points in the series - need to do this ourselves.
      this->CalculateBounds(bounds);
      }
    }
  vtkDebugMacro(<< "Bounds: " << bounds[0] << "\t" << bounds[1] << "\t"
                << bounds[2] << "\t" << bounds[3]);
}

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
bool vtkMyPlotPoints::GetNearestPoint(const vtkVector2f& point,
                                    const vtkVector2f& tol,
                                    vtkVector3f* location)
{
  // Right now doing a simple bisector search of the array. This should be
  // revisited. Assumes the x axis is sorted, which should always be true for
  // line plots.
  if (!this->Points)
    {
    return false;
    }
  vtkIdType n = this->Points->GetNumberOfPoints();
  if (n < 2)
    {
    return false;
    }

  // Sort the data if it has not been done already.  We need to sort it
  // and collect the base and extent into the same vector since both will
  // get involved in range checking.
  if (!this->Sorted)
    {
    vtkVector2f* data =
        static_cast<vtkVector2f*>(this->Points->GetVoidPointer(0));
    this->Sorted = new VectorPIMPL3();
    for (int i = 0; i < n; i++)
      {
      vtkVector3f combined(data[i].X(), data[i].Y(), (float)i);
      this->Sorted->push_back(combined);
      }
    vtkstd::sort(this->Sorted->begin(), this->Sorted->end(), compVector3fX);
    }

  // Set up our search array, use the STL lower_bound algorithm
  VectorPIMPL3::iterator low;
  VectorPIMPL3 &v = *this->Sorted;

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
      *location = *low;
      return true;
      }
    else if (low->X() > highX)
      {
      break;
      }
    ++low;
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkMyPlotPoints::SelectPoints(const vtkVector2f& min, const vtkVector2f& max)
{
  if (!this->Points)
    {
    return false;
    }

  if (!this->Selection)
    {
    this->Selection = vtkIdTypeArray::New();
    }
  this->Selection->SetNumberOfTuples(0);

  // Iterate through all points and check whether any are in range
  vtkVector2f* data = static_cast<vtkVector2f*>(this->Points->GetVoidPointer(0));
  vtkIdType n = this->Points->GetNumberOfPoints();

  for (vtkIdType i = 0; i < n; ++i)
    {
    if (data[i].X() >= min.X() && data[i].X() <= max.X() &&
        data[i].Y() >= min.Y() && data[i].Y() <= max.Y())
      {
      this->Selection->InsertNextValue(i);
      }
    }
  return this->Selection->GetNumberOfTuples() > 0;
}

//-----------------------------------------------------------------------------
namespace {

// Copy the two arrays into the points array
template<class A>
void CopyToPointsSwitch(vtkPoints2D *points, A *a, vtkDataArray *b, int n)
{
  switch(b->GetDataType())
    {
    vtkTemplateMacro(
        CopyToPoints(points, a, static_cast<VTK_TT*>(b->GetVoidPointer(0)), n));
    }
}

// Copy the two arrays into the points array
template<class A, class B>
void CopyToPoints(vtkPoints2D *points, A *a, B *b, int n)
{
  points->SetNumberOfPoints(n);
  float* data = static_cast<float*>(points->GetVoidPointer(0));
  for (int i = 0; i < n; ++i)
    {
    data[2*i] = a[i];
    data[2*i+1] = b[i];
    }
}

// Copy one array into the points array, use the index of that array as x
template<class A>
void CopyToPoints(vtkPoints2D *points, A *a, int n)
{
  points->SetNumberOfPoints(n);
  float* data = static_cast<float*>(points->GetVoidPointer(0));
  for (int i = 0; i < n; ++i)
    {
    data[2*i] = static_cast<float>(i);
    data[2*i+1] = a[i];
    }
}

}

//-----------------------------------------------------------------------------
bool vtkMyPlotPoints::UpdateTableCache(vtkTable *table)
{
  // Get the x and y arrays (index 0 and 1 respectively)
  vtkDataArray* x = this->UseIndexForXSeries ?
                    0 : this->Data->GetInputArrayToProcess(0, table);
  vtkDataArray* y = this->Data->GetInputArrayToProcess(1, table);
  if (!x && !this->UseIndexForXSeries)
    {
    vtkErrorMacro(<< "No X column is set (index 0).");
    this->BuildTime.Modified();
    return false;
    }
  else if (!y)
    {
    vtkErrorMacro(<< "No Y column is set (index 1).");
    this->BuildTime.Modified();
    return false;
    }
  else if (!this->UseIndexForXSeries &&
           x->GetNumberOfTuples() != y->GetNumberOfTuples())
    {
    vtkErrorMacro("The x and y columns must have the same number of elements. "
                  << x->GetNumberOfTuples() << ", " << y->GetNumberOfTuples());
    this->BuildTime.Modified();
    return false;
    }

  if (!this->Points)
    {
    this->Points = vtkPoints2D::New();
    }

  // Now copy the components into their new columns
  if (this->UseIndexForXSeries)
    {
    switch(y->GetDataType())
      {
        vtkTemplateMacro(
            CopyToPoints(this->Points,
                         static_cast<VTK_TT*>(y->GetVoidPointer(0)),
                         y->GetNumberOfTuples()));
      }
    }
  else
    {
    switch(x->GetDataType())
      {
      vtkTemplateMacro(
          CopyToPointsSwitch(this->Points,
                             static_cast<VTK_TT*>(x->GetVoidPointer(0)),
                             y, x->GetNumberOfTuples()));
      }
    }
  this->CalculateLogSeries();
  this->FindBadPoints();
  this->Points->Modified();
  if (this->Sorted)
    {
    delete this->Sorted;
    this->Sorted = 0;
    }
  this->BuildTime.Modified();
  return true;
}

//-----------------------------------------------------------------------------
inline void vtkMyPlotPoints::CalculateLogSeries()
{
  if (!this->XAxis || !this->YAxis)
    {
    return;
    }
  this->LogX = this->XAxis->GetLogScale();
  this->LogY = this->YAxis->GetLogScale();
  float* data = static_cast<float*>(this->Points->GetVoidPointer(0));
  vtkIdType n = this->Points->GetNumberOfPoints();
  if (this->LogX)
    {
    for (vtkIdType i = 0; i < n; ++i)
      {
      data[2*i] = log10(data[2*i]);
      }
    }
  if (this->LogY)
    {
    for (vtkIdType i = 0; i < n; ++i)
    {
    data[2*i+1] = log10(data[2*i+1]);
    }
  }
}

//-----------------------------------------------------------------------------
inline void vtkMyPlotPoints::FindBadPoints()
{
  // This should be run after CalculateLogSeries as a final step.
  float* data = static_cast<float*>(this->Points->GetVoidPointer(0));
  vtkIdType n = this->Points->GetNumberOfPoints();
  if (!this->BadPoints)
    {
    this->BadPoints = vtkIdTypeArray::New();
    }
  else
    {
    this->BadPoints->SetNumberOfTuples(0);
    }

  // Scan through and find any bad points.
  for (vtkIdType i = 0; i < n; ++i)
    {
    vtkIdType p = 2*i;
    if (vtkMath::IsInf(data[p]) || vtkMath::IsInf(data[p+1]) ||
        vtkMath::IsNan(data[p]) || vtkMath::IsNan(data[p+1]))
      {
      this->BadPoints->InsertNextValue(i);
      }
    }

  if (this->BadPoints->GetNumberOfTuples() == 0)
    {
    this->BadPoints->Delete();
    this->BadPoints = NULL;
    }
}

//-----------------------------------------------------------------------------
inline void vtkMyPlotPoints::CalculateBounds(double bounds[4])
{
  // We can use the BadPoints array to skip the bad points
  if (!this->Points || !this->BadPoints)
    {
    return;
    }
  vtkIdType start = 0;
  vtkIdType end = 0;
  vtkIdType i = 0;
  vtkIdType nBad = this->BadPoints->GetNumberOfTuples();
  vtkIdType nPoints = this->Points->GetNumberOfPoints();
  if (this->BadPoints->GetValue(i) == 0)
    {
    while (i < nBad && i == this->BadPoints->GetValue(i))
      {
      start = this->BadPoints->GetValue(i++) + 1;
      }
    if (start < nPoints)
      {
      end = nPoints;
      }
    else
      {
      // They are all bad points
      return;
      }
    }
  if (i < nBad)
    {
    end = this->BadPoints->GetValue(i++);
    }
  else
    {
    end = nPoints;
    }
  vtkVector2f* pts = static_cast<vtkVector2f*>(this->Points->GetVoidPointer(0));

  // Initialize our min/max
  bounds[0] = bounds[1] = pts[start].X();
  bounds[2] = bounds[3] = pts[start++].Y();

  while (start < nPoints)
    {
    // Calculate the min/max in this range
    while (start < end)
      {
      float x = pts[start].X();
      float y = pts[start++].Y();
      if (x < bounds[0])
        {
        bounds[0] = x;
        }
      else if (x > bounds[1])
        {
        bounds[1] = x;
        }
      if (y < bounds[2])
        {
        bounds[2] = y;
        }
      else if (y > bounds[3])
        {
        bounds[3] = y;
        }
      }
    // Now figure out the next range
    start = end + 1;
    if (++i < nBad)
      {
      end = this->BadPoints->GetValue(i);
      }
    else
      {
      end = nPoints;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkMyPlotPoints::SetImageStack(vtkImageData* stack)
{
  this->ImageStack = stack;
  this->ImageStack->UpdateInformation();
  this->reslice->SetInput(this->ImageStack);
  this->reslice->Modified();
  this->lut->SetRange(this->ImageStack->GetPointData()->GetScalars()->GetRange());
  this->lut->Modified();
  int extent[6];
  this->ImageStack->UpdateInformation();
  this->ImageStack->GetWholeExtent(extent);
  this->NumImages = (extent[5]-extent[4]+1);
}

//-----------------------------------------------------------------------------
vtkImageData* vtkMyPlotPoints::GetImageAtIndex(int imageId)
{
  if (this->ImageStack)
    {
    // Calculate the center of the volume
    this->ImageStack->UpdateInformation();
    int extent[6];
    double spacing[3];
    double origin[3];
    this->ImageStack->GetWholeExtent(extent);
    this->ImageStack->GetSpacing(spacing);
    this->ImageStack->GetOrigin(origin);

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
int vtkMyPlotPoints::GetNumberOfImages()
{
  if (this->ImageStack)
    {
    return this->NumImages;
    }
  else
    {
    return 0;
    }
}

//-----------------------------------------------------------------------------
void vtkMyPlotPoints::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
