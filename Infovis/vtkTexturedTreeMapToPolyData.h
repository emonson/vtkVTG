/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkTexturedTreeMapToPolyData.h,v $

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
// .NAME vtkTexturedTreeMapToPolyData - converts a tree to a polygonal data representing a tree map
//
// .SECTION Description
// This algorithm requires that the vtkTreeMapLayout filter has already applied to the
// data in order to create the quadruple array (min x, max x, min y, max y) of
// bounds for each vertex of the tree.

#ifndef __vtkTexturedTreeMapToPolyData_h
#define __vtkTexturedTreeMapToPolyData_h

#include "vtkPolyDataAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkTexturedTreeMapToPolyData : public vtkPolyDataAlgorithm 
{
public:
  static vtkTexturedTreeMapToPolyData *New();
  vtkTypeRevisionMacro(vtkTexturedTreeMapToPolyData,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The field containing quadruples of the form (min x, max x, min y, max y)
  // representing the bounds of the rectangles for each vertex.
  // This array may be added to the tree using vtkTreeMapLayout.
  virtual void SetRectanglesArrayName(const char* name)
    { this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, name); }

  // Description:
  // The field containing the level of each tree node.
  // This can be added using vtkTreeLevelsFilter before this filter.
  // If this is not present, the filter simply calls tree->GetLevel(v) for
  // each vertex, which will produce the same result, but
  // may not be as efficient.
  virtual void SetLevelArrayName(const char* name)
    { this->SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, name); }

  // Description:
  // The spacing along the z-axis between tree map levels.
  vtkGetMacro(LevelDeltaZ, double);
  vtkSetMacro(LevelDeltaZ, double);

  // Description:
  // The spacing along the z-axis between tree map levels.
  vtkGetMacro(AddNormals, bool);
  vtkSetMacro(AddNormals, bool);

  // Texture Map stuff
  
  // Description:
  // Specify a point defining the origin of the plane. Used in conjunction with
  // the Point1 and Point2 ivars to specify a map plane.
  vtkSetVector3Macro(Origin,double);
  vtkGetVectorMacro(Origin,double,3);

  // Description:
  // Specify plane normal. An alternative way to specify a map plane. Using
  // this method, the object will scale the resulting texture coordinate
  // between the SRange and TRange specified.
  vtkSetVector3Macro(Normal,double);
  vtkGetVectorMacro(Normal,double,3);

  // Description:
  // Specify s-coordinate range for texture s-t coordinate pair.
  vtkSetVector2Macro(SRange,double);
  vtkGetVectorMacro(SRange,double,2);

  // Description:
  // Specify t-coordinate range for texture s-t coordinate pair.
  vtkSetVector2Macro(TRange,double);
  vtkGetVectorMacro(TRange,double,2);
  
  int FillInputPortInformation(int port, vtkInformation* info);

protected:
  vtkTexturedTreeMapToPolyData();
  ~vtkTexturedTreeMapToPolyData();

  double LevelDeltaZ;
  bool AddNormals;

  // Texture map stuff
  double Origin[3];
  double Normal[3];
  double SRange[2];
  double TRange[2];

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
private:
  vtkTexturedTreeMapToPolyData(const vtkTexturedTreeMapToPolyData&);  // Not implemented.
  void operator=(const vtkTexturedTreeMapToPolyData&);  // Not implemented.
};

#endif
