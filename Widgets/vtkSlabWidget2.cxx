/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSlabWidget2.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSlabWidget2.h"
#include "vtkSlabRepresentation.h"
#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkWidgetEventTranslator.h"
#include "vtkWidgetCallbackMapper.h" 
#include "vtkEvent.h"
#include "vtkWidgetEvent.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"


vtkCxxRevisionMacro(vtkSlabWidget2, "$Revision: 1.3 $");
vtkStandardNewMacro(vtkSlabWidget2);

//----------------------------------------------------------------------------
vtkSlabWidget2::vtkSlabWidget2()
{
  this->WidgetState = vtkSlabWidget2::Start;
  this->ManagesCursor = 1;

  this->TranslationEnabled = 1;
  this->ScalingEnabled = 1;
  this->RotationEnabled = 1;

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkEvent::NoModifier,
                                          0, 0, NULL,
                                          vtkWidgetEvent::Select,
                                          this, 
                                          vtkSlabWidget2::SelectAction);
                                          
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
                                          vtkEvent::NoModifier,
                                          0, 0, NULL,
                                          vtkWidgetEvent::EndSelect,
                                          this, 
                                          vtkSlabWidget2::EndSelectAction);
                                          
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonPressEvent,
                                          vtkWidgetEvent::Translate,
                                          this, 
                                          vtkSlabWidget2::TranslateAction);
                                          
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonReleaseEvent,
                                          vtkWidgetEvent::EndTranslate,
                                          this, 
                                          vtkSlabWidget2::EndSelectAction);
                                          
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkEvent::ControlModifier,
                                          0, 0, NULL,
                                          vtkWidgetEvent::Translate,
                                          this, 
                                          vtkSlabWidget2::TranslateAction);
                                          
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
                                          vtkEvent::ControlModifier,
                                          0, 0, NULL,
                                          vtkWidgetEvent::EndTranslate,
                                          this, 
                                          vtkSlabWidget2::EndSelectAction);
                                          
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkEvent::ShiftModifier,
                                          0, 0, NULL,
                                          vtkWidgetEvent::Translate,
                                          this, 
                                          vtkSlabWidget2::TranslateAction);
                                          
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
                                          vtkEvent::ShiftModifier,
                                          0, 0, NULL,
                                          vtkWidgetEvent::EndTranslate,
                                          this, 
                                          vtkSlabWidget2::EndSelectAction);
                                          
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonPressEvent,
                                          vtkWidgetEvent::Scale,
                                          this, 
                                          vtkSlabWidget2::ScaleAction);
                                          
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonReleaseEvent,
                                          vtkWidgetEvent::EndScale,
                                          this, 
                                          vtkSlabWidget2::EndSelectAction);
                                          
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, 
                                          vtkSlabWidget2::MoveAction);
}

//----------------------------------------------------------------------------
vtkSlabWidget2::~vtkSlabWidget2()
{  
}

//----------------------------------------------------------------------
void vtkSlabWidget2::SelectAction(vtkAbstractWidget *w)
{
  // We are in a static method, cast to ourself
  vtkSlabWidget2 *self = reinterpret_cast<vtkSlabWidget2*>(w);
  // DEBUG
  // printf("SlabWidget2::SelectAction()\n");

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  
  // Okay, make sure that the pick is in the current renderer
  if ( !self->CurrentRenderer || 
       !self->CurrentRenderer->IsInViewport(X,Y) )
    {
    self->WidgetState = vtkSlabWidget2::Start;
    return;
    }
  
  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if ( interactionState == vtkSlabRepresentation::Outside )
    {
    return;
    }
  
  // We are definitely selected
  self->WidgetState = vtkSlabWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);
  
  // The SetInteractionState has the side effect of highlighting the widget
  reinterpret_cast<vtkSlabRepresentation*>(self->WidgetRep)->
    SetInteractionState(interactionState);
 
  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  self->Render();
}

//----------------------------------------------------------------------
void vtkSlabWidget2::TranslateAction(vtkAbstractWidget *w)
{
  // We are in a static method, cast to ourself
  vtkSlabWidget2 *self = reinterpret_cast<vtkSlabWidget2*>(w);
  // DEBUG
  // printf("SlabWidget2::TranslateAction()\n");

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  
  // Okay, make sure that the pick is in the current renderer
  if ( !self->CurrentRenderer || 
       !self->CurrentRenderer->IsInViewport(X,Y) )
    {
    self->WidgetState = vtkSlabWidget2::Start;
    return;
    }
  
  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if ( interactionState == vtkSlabRepresentation::Outside )
    {
    return;
    }
  
  // We are definitely selected
  self->WidgetState = vtkSlabWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);
  reinterpret_cast<vtkSlabRepresentation*>(self->WidgetRep)->
    SetInteractionState(vtkSlabRepresentation::Translating);
  
  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  self->Render();
}

//----------------------------------------------------------------------
void vtkSlabWidget2::ScaleAction(vtkAbstractWidget *w)
{
  // We are in a static method, cast to ourself
  vtkSlabWidget2 *self = reinterpret_cast<vtkSlabWidget2*>(w);
  // DEBUG
  // printf("SlabWidget2::ScaleAction()\n");

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  
  // Okay, make sure that the pick is in the current renderer
  if ( !self->CurrentRenderer || 
       !self->CurrentRenderer->IsInViewport(X,Y) )
    {
    self->WidgetState = vtkSlabWidget2::Start;
    return;
    }
  
  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if ( interactionState == vtkSlabRepresentation::Outside )
    {
    return;
    }
  
  // We are definitely selected
  self->WidgetState = vtkSlabWidget2::Active;
  self->GrabFocus(self->EventCallbackCommand);
  reinterpret_cast<vtkSlabRepresentation*>(self->WidgetRep)->
    SetInteractionState(vtkSlabRepresentation::Scaling);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  self->Render();
}

//----------------------------------------------------------------------
void vtkSlabWidget2::MoveAction(vtkAbstractWidget *w)
{
  vtkSlabWidget2 *self = reinterpret_cast<vtkSlabWidget2*>(w);
  // DEBUG
  // printf("SlabWidget2::MoveAction()\n");

  // See whether we're active
  if ( self->WidgetState == vtkSlabWidget2::Start )
    {
    return;
    }
  
  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, adjust the representation
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->WidgetInteraction(e);

  // moving something
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  self->Render();
}

//----------------------------------------------------------------------
void vtkSlabWidget2::EndSelectAction(vtkAbstractWidget *w)
{
  vtkSlabWidget2 *self = reinterpret_cast<vtkSlabWidget2*>(w);
  // DEBUG
  // printf("SlabWidget2::EndSelectAction()\n");

  if ( self->WidgetState == vtkSlabWidget2::Start )
    {
    return;
    }
  
  // Return state to not active
  self->WidgetState = vtkSlabWidget2::Start;
  reinterpret_cast<vtkSlabRepresentation*>(self->WidgetRep)->
    SetInteractionState(vtkSlabRepresentation::Outside);
  self->ReleaseFocus();

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  self->Render();
}

//----------------------------------------------------------------------
void vtkSlabWidget2::CreateDefaultRepresentation()
{
  if ( ! this->WidgetRep )
    {
    this->WidgetRep = vtkSlabRepresentation::New();
    }
}

//----------------------------------------------------------------------------
void vtkSlabWidget2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Translation Enabled: " << (this->TranslationEnabled ? "On\n" : "Off\n");
  os << indent << "Scaling Enabled: " << (this->ScalingEnabled ? "On\n" : "Off\n");
  os << indent << "Rotation Enabled: " << (this->RotationEnabled ? "On\n" : "Off\n");
}


