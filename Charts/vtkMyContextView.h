/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMyContextView.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMyContextView - provides a view of the vtkContextScene.
//
// .SECTION Description
// This class is derived from vtkRenderView and provides a view of a
// vtkContextScene, with a default interactor style, renderer etc.

#ifndef __vtkMyContextView_h
#define __vtkMyContextView_h

#include "vtkContextView.h"

class vtkContext2D;
class vtkContextScene;
class vtkRenderWindowInteractor;

class VTK_CHARTS_EXPORT vtkMyContextView : public vtkContextView
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeMacro(vtkMyContextView,vtkContextView);

  static vtkMyContextView* New();


protected:
  vtkMyContextView();
  ~vtkMyContextView();


private:
  vtkMyContextView(const vtkMyContextView&);  // Not implemented.
  void operator=(const vtkMyContextView&);  // Not implemented.
};

#endif
