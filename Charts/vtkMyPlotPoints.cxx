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

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMyPlotPoints);

//-----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkMyPlotPoints, HighlightSelection, vtkIdTypeArray);

//-----------------------------------------------------------------------------
vtkMyPlotPoints::vtkMyPlotPoints()
{
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
void vtkMyPlotPoints::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
