/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConvertIds.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkConvertIds - Map values in an input array to different values in
//   an output array of (possibly) different type.

// .SECTION Description
// vtkConvertIds allows you to associate certain values of an attribute array
// (on either a vertex, edge, point, or cell) with different values in a
// newly created attribute array. 
//
// vtkConvertIds manages an internal STL map of vtkVariants that can be added to
// or cleared. When this filter executes, each "key" is searched for in the
// input array and the indices of the output array at which there were matches
// the set to the mapped "value".
//
// You can control whether the input array values are passed to the output
// before the mapping occurs (using PassArray) or, if not, what value to set 
// the unmapped indices to (using FillValue). 
//
// One application of this filter is to help address the dirty data problem.
// For example, using vtkConvertIds you could associate the vertex values 
// "Foo, John", "Foo, John.", and "John Foo" with a single entity.

#ifndef __vtkConvertIds_h
#define __vtkConvertIds_h

#include "vtkPassInputTypeAlgorithm.h"
#include "vtkvtgRenderingWin32Header.h"

class vtkMapType;
class vtkVariant;

class VTK_VTG_RENDERING_EXPORT vtkConvertIds : public vtkPassInputTypeAlgorithm
{
public:
  vtkTypeMacro(vtkConvertIds,vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkConvertIds *New();

  // Description: 
  // Set/Get the name of the output array. Default is "ConvertedIds".
  vtkSetStringMacro(OutputArrayName);
  vtkGetStringMacro(OutputArrayName);


protected:

  vtkConvertIds();
  virtual ~vtkConvertIds();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int FillInputPortInformation(int, vtkInformation *);

  char* OutputArrayName;

private:
  vtkConvertIds(const vtkConvertIds&);  // Not implemented.
  void operator=(const vtkConvertIds&);  // Not implemented.
};

#endif
