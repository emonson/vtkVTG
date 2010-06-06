/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMyContextActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMyContextActor.h"

#include "vtkContext2D.h"
#include "vtkMyOpenGLContextDevice2D.h"
#include "vtkContextScene.h"
#include "vtkTransform2D.h"

#include "vtkViewport.h"
#include "vtkWindow.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkMyContextActor);

//----------------------------------------------------------------------------
vtkMyContextActor::vtkMyContextActor()
{
  this->Context = vtkContext2D::New();
  vtkOpenGLContextDevice2D *pd = vtkMyOpenGLContextDevice2D::New();
  this->Context->Begin(pd);
  pd->Delete();
  pd = NULL;

  this->Scene = vtkContextScene::New();
}

//----------------------------------------------------------------------------
// Destroy an actor2D.
vtkMyContextActor::~vtkMyContextActor()
{
  if (this->Context)
    {
    this->Context->End();
    this->Context->Delete();
    this->Context = NULL;
    }

  if (this->Scene)
    {
    this->Scene->Delete();
    this->Scene = NULL;
    }
}



//----------------------------------------------------------------------------
void vtkMyContextActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Context: " << this->Context << "\n";
  if (this->Context)
    {
    this->Context->PrintSelf(os, indent.GetNextIndent());
    }
}
