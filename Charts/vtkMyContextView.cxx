/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMyContextView.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMyContextView.h"

#include "vtkContext2D.h"
#include "vtkMyOpenGLContextDevice2D.h"
#include "vtkContextScene.h"

#include "vtkViewport.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorStyle.h"
#include "vtkMyContextActor.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkMyContextView);

//----------------------------------------------------------------------------
vtkMyContextView::vtkMyContextView()
{
  this->Context = vtkContext2D::New();
  vtkMyOpenGLContextDevice2D *pd = vtkMyOpenGLContextDevice2D::New();
  this->Context->Begin(pd);
  pd->Delete();

  vtkMyContextActor *actor = vtkMyContextActor::New();
  this->Renderer->AddActor(actor);
  actor->Delete();
  this->Scene = actor->GetScene(); // We keep a pointer to this for convenience
  this->Scene->Register(this);
  // Should not need to do this...
  this->Scene->SetRenderer(this->Renderer);

  // Set up our view to render on move, 2D interaction style
  // NOTE: These don't work now that vtkContextView inherits from vtkRenderViewBase
  // this->SetDisplayHoverText(false);
  // this->RenderOnMouseMoveOn();
  // this->SetInteractionModeTo2D();

  // Single color background
  this->Renderer->SetBackground(1.0, 1.0, 1.0);
  this->Renderer->SetBackground2(1.0, 1.0, 1.0);
}

//----------------------------------------------------------------------------
vtkMyContextView::~vtkMyContextView()
{
  if (this->Context)
    {
    this->Context->Delete();
    this->Context = NULL;
    }
  // The scene is owned by the context actor
  if (this->Scene)
    {
    this->Scene->Delete();
    this->Scene = NULL;
    }
}


//----------------------------------------------------------------------------
void vtkMyContextView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Context: " << this->Context << "\n";
  if (this->Context)
    {
    this->Context->PrintSelf(os, indent.GetNextIndent());
    }
}
