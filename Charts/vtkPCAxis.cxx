/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPCAxis.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPCAxis.h"

#include "vtkContext2D.h"
#include "vtkPen.h"
#include "vtkTextProperty.h"
#include "vtkVector.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkStdString.h"
#include "vtksys/ios/sstream"

#include "vtkObjectFactory.h"

#include "math.h"

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPCAxis);

//-----------------------------------------------------------------------------
vtkPCAxis::vtkPCAxis()
{
}

//-----------------------------------------------------------------------------
vtkPCAxis::~vtkPCAxis()
{
}

//-----------------------------------------------------------------------------
bool vtkPCAxis::Paint(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called in vtkAxis.");

  if (!this->Visible)
    {
    return false;
    }

  painter->ApplyPen(this->Pen);

  // Draw this axis
  painter->GetPen()->SetColor(200,200,200);
  painter->DrawLine(this->Point1[0], this->Point1[1],
                    this->Point2[0], this->Point2[1]);
  painter->GetPen()->SetColor(0,0,0);

  vtkTextProperty *prop = painter->GetTextProp();

  // Draw the axis title if there is one
  if (this->Title && this->Title[0])
    {
    int x = 0;
    int y = 0;
    painter->ApplyTextProp(this->TitleProperties);

		// PARALLEL
		x = vtkContext2D::FloatToInt(this->Point1[0]);
		y = vtkContext2D::FloatToInt(this->Point1[1] - this->MaxLabel[1] - 15);
		prop->SetOrientation(0.0);
		prop->SetVerticalJustificationToTop();

    painter->DrawString(x, y, this->Title);
    }

  // Now draw the tick marks
  painter->ApplyTextProp(this->LabelProperties);

  float *tickPos = this->TickScenePositions->GetPointer(0);
  vtkStdString *tickLabel = this->TickLabels->GetPointer(0);
  vtkIdType numMarks = this->TickScenePositions->GetNumberOfTuples();

  // There are four possible tick label positions, which should be set by the
  // class laying out the axes.

	// PARALLEL
	prop->SetJustificationToRight();
	prop->SetVerticalJustificationToCentered();

	// Draw the tick marks and labels
	for (vtkIdType i = 0; i < numMarks; ++i)
		{
		painter->DrawLine(this->Point1[0] - 5, tickPos[i],
											this->Point1[0],     tickPos[i]);
		if (this->LabelsVisible)
			{
			painter->DrawString(this->Point1[0] - 7, tickPos[i], tickLabel[i]);
			}
		}

  return true;
}

//-----------------------------------------------------------------------------
bool vtkPCAxis::PaintNoLines(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called in vtkPCAxis.");

  if (!this->Visible)
    {
    return false;
    }

  painter->ApplyPen(this->Pen);

  vtkTextProperty *prop = painter->GetTextProp();

  // Draw the axis title if there is one
  if (this->Title && this->Title[0])
    {
    int x = 0;
    int y = 0;
    painter->ApplyTextProp(this->TitleProperties);

		// PARALLEL
		x = vtkContext2D::FloatToInt(this->Point1[0]);
		y = vtkContext2D::FloatToInt(this->Point1[1] - this->MaxLabel[1] - 15);
		prop->SetOrientation(0.0);
		prop->SetVerticalJustificationToTop();

    painter->DrawString(x, y, this->Title);
    }

  // Now draw the tick marks
  painter->ApplyTextProp(this->LabelProperties);

  float *tickPos = this->TickScenePositions->GetPointer(0);
  vtkStdString *tickLabel = this->TickLabels->GetPointer(0);
  vtkIdType numMarks = this->TickScenePositions->GetNumberOfTuples();

  // There are four possible tick label positions, which should be set by the
  // class laying out the axes.

	// PARALLEL
	prop->SetJustificationToRight();
	prop->SetVerticalJustificationToCentered();

	// Draw the tick marks and labels
	for (vtkIdType i = 0; i < numMarks; ++i)
		{
		painter->DrawLine(this->Point1[0] - 5, tickPos[i],
											this->Point1[0],     tickPos[i]);
		if (this->LabelsVisible)
			{
			painter->DrawString(this->Point1[0] - 7, tickPos[i], tickLabel[i]);
			}
		}

  return true;
}

//-----------------------------------------------------------------------------
void vtkPCAxis::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Title)
    {
    os << indent << "Axis title: \"" << *this->Title << "\"" << endl;
    }
  os << indent << "Minimum point: " << this->Point1[0] << ", "
     << this->Point1[1] << endl;
  os << indent << "Maximum point: " << this->Point2[0] << ", "
     << this->Point2[1] << endl;
  os << indent << "Range: " << this->Minimum << " - " << this->Maximum << endl;
  os << indent << "Number of tick marks: " << this->NumberOfTicks << endl;

}
