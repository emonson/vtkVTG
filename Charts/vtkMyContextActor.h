/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMyContextActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMyContextActor - provides a vtkProp derived object.
// .SECTION Description
// This object provides the entry point for the vtkContextScene to be rendered
// in a vtkRenderer. Uses the RenderOverlay pass to render the 2D
// vtkContextScene.

#ifndef __vtkMyContextActor_h
#define __vtkMyContextActor_h

#include "vtkContextActor.h"

class vtkContext2D;
class vtkContextScene;

class VTK_CHARTS_EXPORT vtkMyContextActor : public vtkContextActor
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeMacro(vtkMyContextActor,vtkContextActor);

  static vtkMyContextActor* New();


protected:
  vtkMyContextActor();
  ~vtkMyContextActor();


private:
  vtkMyContextActor(const vtkMyContextActor&);  // Not implemented.
  void operator=(const vtkMyContextActor&);  // Not implemented.
};

#endif
