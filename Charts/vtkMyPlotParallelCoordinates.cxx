/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMyPlotParallelCoordinates.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMyPlotParallelCoordinates.h"
#include "vtkMyChartParallelCoordinates.h"

#include "vtkContext2D.h"
#include "vtkColor.h"
#include "vtkAxis.h"
#include "vtkPen.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkVector.h"
#include "vtkTransform2D.h"
#include "vtkContextDevice2D.h"
#include "vtkContextMapper2D.h"
#include "vtkPoints2D.h"
#include "vtkTable.h"
#include "vtkDataArray.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkStringArray.h"
#include "vtkTimeStamp.h"
#include "vtkInformation.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkLookupTable.h"

// Need to turn some arrays of strings into categories
#include "vtkStringToCategory.h"

#include "vtkObjectFactory.h"


//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMyPlotParallelCoordinates);

//-----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkMyPlotParallelCoordinates, HighlightSelection, vtkIdTypeArray);

//-----------------------------------------------------------------------------
vtkMyPlotParallelCoordinates::vtkMyPlotParallelCoordinates()
{
  this->HighlightSelection = NULL;
}

//-----------------------------------------------------------------------------
vtkMyPlotParallelCoordinates::~vtkMyPlotParallelCoordinates()
{
  if (this->HighlightSelection)
    {
    this->HighlightSelection->Delete();
    this->HighlightSelection = NULL;
    }
}

//-----------------------------------------------------------------------------
// bool vtkMyPlotParallelCoordinates::Paint(vtkContext2D *painter)
// {
//   // This is where everything should be drawn, or dispatched to other methods.
//   vtkDebugMacro(<< "Paint event called in vtkMyPlotParallelCoordinates.");
// 
//   if (!this->Visible)
//     {
//     return false;
//     }
// 
//   float width = this->Pen->GetWidth() * 2.0;
//   if (width < 1.5)
//     {
//     width = 1.5;
//     }
// 
//   // Now to plot the points
//   if (this->Points)
//     {
//     painter->ApplyPen(this->Pen);
//     painter->DrawPoly(this->Points);
//     painter->GetPen()->SetLineType(vtkPen::SOLID_LINE);
//     }
// 
//   // If there is a marker style, then draw the marker for each point too
//   if (this->MarkerStyle && this->Points)
//     {
//     this->GeneraterMarker(vtkContext2D::FloatToInt(width));
//     painter->ApplyPen(this->GetPen());
//     painter->ApplyBrush(this->GetBrush());
//     painter->GetPen()->SetWidth(width);
//     painter->DrawPointSprites(this->Marker, this->Points);
//     }
// 
//   painter->ApplyPen(this->Pen);
// 
//   if (this->Storage->size() == 0)
//     {
//     return false;
//     }
// 
//   size_t cols = this->Storage->size();
//   size_t rows = this->Storage->at(0).size();
//   vtkVector2f* line = new vtkVector2f[cols];
//   vtkColor4ub* colors = new vtkColor4ub[cols];
// 
//   // Update the axis positions
//   for (size_t i = 0; i < cols; ++i)
//     {
//     this->Storage->AxisPos[i] = this->Parent->GetAxis(int(i)) ?
//                                 this->Parent->GetAxis(int(i))->GetPoint1()[0] :
//                                 0;
//     }
// 
//   vtkIdType selection = 0;
//   vtkIdType id = 0;
//   vtkIdType selectionSize = 0;
//   if (this->Selection)
//     {
//     selectionSize = this->Selection->GetNumberOfTuples();
//     if (selectionSize)
//       {
//       this->Selection->GetTupleValue(selection, &id);
//       }
//     }
// 
//   // Draw all of the points
//   // TODO: This needs to be updated for colored markers
//   this->GeneraterMarker(vtkContext2D::FloatToInt(width));
//   painter->ApplyPen(this->GetPen());
//   painter->ApplyBrush(this->GetBrush());
//   painter->GetPen()->SetWidth(width);
//   painter->GetPen()->SetOpacity(180);
//   for (size_t i = 0; i < rows; ++i)
//     {
//     for (size_t j = 0; j < cols; ++j)
//       {
//       line[j].Set(this->Storage->AxisPos[j], (*this->Storage)[j][i]);
//       }
//     painter->DrawPointSprites(this->Marker, line[0].GetData(), static_cast<int>(cols));
//     }
// 
//   // Draw all of the lines
//   painter->ApplyPen(this->Pen);
//   int nc_comps;
//   if (this->ScalarVisibility && this->Colors)
//     {
//     nc_comps = static_cast<int>(this->Colors->GetNumberOfComponents());
//     }
//   if (this->ScalarVisibility && this->Colors && (nc_comps == 4))
//     {
//     for (size_t i = 0, nc = 0; i < rows; ++i, nc += nc_comps)
//       {
//       for (size_t j = 0; j < cols; ++j)
//         {
//         line[j].Set(this->Storage->AxisPos[j], (*this->Storage)[j][i]);
//         colors[j].Set(*this->Colors->GetPointer(nc), *this->Colors->GetPointer(nc+1),
//                       *this->Colors->GetPointer(nc+2), *this->Colors->GetPointer(nc+3));
//         }
//       painter->DrawPoly(line[0].GetData(), static_cast<int>(cols),
//                         colors[0].GetData(), nc_comps);
//       }
//     }
//   else
//     {
//     for (size_t i = 0; i < rows; ++i)
//       {
//       for (size_t j = 0; j < cols; ++j)
//         {
//         line[j].Set(this->Storage->AxisPos[j], (*this->Storage)[j][i]);
//         }
//       painter->DrawPoly(line[0].GetData(), static_cast<int>(cols));
//       }
//     }
// 
//   // Now draw the selected lines
//   if (this->Selection)
//     {
//     painter->GetPen()->SetColor(255, 0, 0, 154);
//     for (vtkIdType i = 0; i < this->Selection->GetNumberOfTuples(); ++i)
//       {
//       for (size_t j = 0; j < cols; ++j)
//         {
//         this->Selection->GetTupleValue(i, &id);
//         line[j].Set(this->Storage->AxisPos[j], (*this->Storage)[j][id]);
//         }
//       painter->DrawPoly(line[0].GetData(), static_cast<int>(cols));
//       }
//     }
// 
//   // Now draw any highlight selected lines (coming from outside the corresponding chart)
//   if (this->HighlightSelection)
//     {
//     painter->GetPen()->SetColor(0, 128, 255, 154);
//     painter->GetPen()->SetWidth(width*1.15);
//     for (vtkIdType i = 0; i < this->HighlightSelection->GetNumberOfTuples(); ++i)
//       {
//       for (size_t j = 0; j < cols; ++j)
//         {
//         this->HighlightSelection->GetTupleValue(i, &id);
//         line[j].Set(this->Storage->AxisPos[j], (*this->Storage)[j][id]);
//         }
//       painter->DrawPoly(line[0].GetData(), static_cast<int>(cols));
//       }
//     }
//   // ### END CUSTOM ###
// 
//   delete[] line;
//   delete[] colors;
// 
//   return true;
// }

//-----------------------------------------------------------------------------
bool vtkMyPlotParallelCoordinates::Paint(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called in vtkPlotParallelCoordinates.");

  this->Superclass::Paint(painter);
  
  return true;
}

//-----------------------------------------------------------------------------
void vtkMyPlotParallelCoordinates::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
