/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkTexturedTreeMapToPolyData.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "vtkTexturedTreeMapToPolyData.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTree.h"

vtkCxxRevisionMacro(vtkTexturedTreeMapToPolyData, "$Revision: 1.10 $");
vtkStandardNewMacro(vtkTexturedTreeMapToPolyData);

vtkTexturedTreeMapToPolyData::vtkTexturedTreeMapToPolyData()
{
  this->SetRectanglesArrayName("area");
  this->SetLevelArrayName("level");
  this->LevelDeltaZ = 0.001;
  this->AddNormals = true;
  
  // Texture map stuff
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;

  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;

  this->SRange[0] = 0.0;
  this->SRange[1] = 1.0;

  this->TRange[0] = 0.0;
  this->TRange[1] = 1.0;

}

vtkTexturedTreeMapToPolyData::~vtkTexturedTreeMapToPolyData()
{
}

int vtkTexturedTreeMapToPolyData::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTree");
  return 1;
}

int vtkTexturedTreeMapToPolyData::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkTree *inputTree = vtkTree::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *outputPoly = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // For each input vertex create 4 points and 1 cell (quad)
  vtkPoints* outputPoints = vtkPoints::New();
  outputPoints->SetNumberOfPoints(inputTree->GetNumberOfVertices()*4);
  vtkCellArray* outputCells = vtkCellArray::New();

  // Create an array for the point normals
  vtkFloatArray* normals = vtkFloatArray::New();
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(inputTree->GetNumberOfVertices()*4);
  normals->SetName("normals");

  vtkDataArray* coordArray = this->GetInputArrayToProcess(0, inputTree);
  if (!coordArray)
    {
    vtkErrorMacro("Area array not found.");
    return 0;
    }
  vtkDataArray* levelArray = this->GetInputArrayToProcess(1, inputTree);

  // Now set the point coordinates, normals, and insert the cell
  for (int i = 0; i < inputTree->GetNumberOfVertices(); i++)
    {
    // Grab coords from the input
    double coords[4];
    coordArray->GetTuple(i,coords);

    double z = 0;
    if (levelArray)
      {
      z = this->LevelDeltaZ * levelArray->GetTuple1(i);
      }
    else
      {
      z = this->LevelDeltaZ * inputTree->GetLevel(i);
      }

    int index = i*4;
    outputPoints->SetPoint(index,   coords[0], coords[2], z);
    outputPoints->SetPoint(index+1, coords[1], coords[2], z);
    outputPoints->SetPoint(index+2, coords[1], coords[3], z);
    outputPoints->SetPoint(index+3, coords[0], coords[3], z);
    
    // Create an asymetric gradient on the cells
    // this gradient helps differentiate same colored
    // cells from their neighbors. The asymetric
    // nature of the gradient is required.
    normals->SetComponent(index,   0, 0);
    normals->SetComponent(index,   1, .707);
    normals->SetComponent(index,   2, .707);
    
    normals->SetComponent(index+1, 0, 0);
    normals->SetComponent(index+1, 1, .866);
    normals->SetComponent(index+1, 2, .5);

    normals->SetComponent(index+2, 0, 0);
    normals->SetComponent(index+2, 1, .707);
    normals->SetComponent(index+2, 2, .707);

    normals->SetComponent(index+3, 0, 0);
    normals->SetComponent(index+3, 1, 0);
    normals->SetComponent(index+3, 2, 1);


    // Create the cell that uses these points
    vtkIdType cellConn[] = {index, index+1, index+2, index+3};
    outputCells->InsertNextCell(4, cellConn);
    }

  // Pass the input point data to the output cell data :)
  outputPoly->GetCellData()->PassData(inputTree->GetVertexData());
  
  // Set the output points and cells
  outputPoly->SetPoints(outputPoints);
  outputPoly->SetPolys(outputCells);

  if( this->AddNormals )
  {
      // Set the point normals
    outputPoly->GetPointData()->AddArray(normals);
    outputPoly->GetPointData()->SetActiveNormals("normals");
  }

  // ======================
  // Texture map stuff
  // ======================
  
  double tcoords[2];
  vtkIdType numPts;
  vtkFloatArray *newTCoords;
  vtkIdType i;
  int j;
  double *bounds;
  double proj, minProj, axis[3], sAxis[3], tAxis[3];
  int dir = 0;
  double s, t, sSf, tSf, p[3];

  vtkDebugMacro(<<"Generating texture coordinates!");

  //  Allocate texture data
  //
  numPts = outputPoly->GetNumberOfPoints();
  
  newTCoords = vtkFloatArray::New();
  newTCoords->SetName("Texture Coordinates");
  newTCoords->SetNumberOfComponents(2);
  newTCoords->SetNumberOfTuples(numPts);

  //  Not using Automatic Mode for now, so assume Normal is valid

    vtkMath::Normalize (this->Normal);

    //  Now project each point onto plane generating s,t texture coordinates
    //
    //  Create local s-t coordinate system.  Need to find the two axes on
    //  the plane and encompassing all the points.  Hence use the bounding
    //  box as a reference.
    //
    for (minProj=1.0, i=0; i<3; i++) 
      {
      axis[0] = axis[1] = axis[2] = 0.0;
      axis[i] = 1.0;
      if ( (proj=fabs(vtkMath::Dot(this->Normal,axis))) < minProj ) 
        {
        minProj = proj;
        dir = i;
        }
      }
    axis[0] = axis[1] = axis[2] = 0.0;
    axis[dir] = 1.0;

    vtkMath::Cross (this->Normal, axis, tAxis);
    vtkMath::Normalize (tAxis);

    vtkMath::Cross (tAxis, this->Normal, sAxis);

    //  Construct projection matrices
    //
    //  Arrange s-t axes so that parametric location of points will fall
    //  between s_range and t_range.  Simplest to do by projecting maximum
    //  corner of bounding box unto plane and backing out scale factors.
    //
    bounds = outputPoly->GetBounds();
    for (i=0; i<3; i++)
      {
      axis[i] = bounds[2*i+1] - bounds[2*i];
      }

    s = vtkMath::Dot(sAxis,axis);
    t = vtkMath::Dot(tAxis,axis);

    sSf = (this->SRange[1] - this->SRange[0]) / s;
    tSf = (this->TRange[1] - this->TRange[0]) / t;

    //  Now can loop over all points, computing parametric coordinates.
    //
    for (i=0; i<numPts; i++) 
      {
      
      outputPoly->GetPoint(i, p);
      for (j=0; j<3; j++)
        {
        axis[j] = p[j] - bounds[2*j];
        }

      tcoords[0] = this->SRange[0] + vtkMath::Dot(sAxis,axis) * sSf;
      tcoords[1] = this->TRange[0] + vtkMath::Dot(tAxis,axis) * tSf;

      newTCoords->SetTuple(i,tcoords);
      }

  
  // Update ourselves
  //
  outputPoly->GetPointData()->AddArray(newTCoords);
  outputPoly->GetPointData()->SetActiveTCoords("Texture Coordinates");
  newTCoords->Delete();


  // Clean up.
  normals->Delete();
  outputPoints->Delete();
  outputCells->Delete();
  
  return 1;
}

void vtkTexturedTreeMapToPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "LevelDeltaZ: " << this->LevelDeltaZ << endl;
  os << indent << "AddNormals: " << this->AddNormals << endl;
}
