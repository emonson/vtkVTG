/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMSlabRepresentationProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSlabRepresentationProxy.h"

#include "vtkSlabRepresentation.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkSMSlabRepresentationProxy);
vtkCxxRevisionMacro(vtkSMSlabRepresentationProxy, "$Revision: 1.2 $");
//----------------------------------------------------------------------------
vtkSMSlabRepresentationProxy::vtkSMSlabRepresentationProxy()
{
}

//----------------------------------------------------------------------------
vtkSMSlabRepresentationProxy::~vtkSMSlabRepresentationProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMSlabRepresentationProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects();

  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << this->GetID()
          << "SetTransform"
          << this->GetSubProxy("Transform")->GetID()
          << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->GetConnectionID(),
    this->GetServers(), 
    stream);
}

//----------------------------------------------------------------------------
void vtkSMSlabRepresentationProxy::UpdateVTKObjects(vtkClientServerStream& stream)
{
  if (this->InUpdateVTKObjects)
    {
    return;
    }

  int something_changed = this->ArePropertiesModified();

  this->Superclass::UpdateVTKObjects(stream);

  if (something_changed)
    {
    stream  << vtkClientServerStream::Invoke
            << this->GetID()
            << "SetTransform"
            << this->GetSubProxy("Transform")->GetID()
            << vtkClientServerStream::End;
    }
}

//----------------------------------------------------------------------------
void vtkSMSlabRepresentationProxy::UpdatePropertyInformation()
{
  vtkSlabRepresentation* repr = vtkSlabRepresentation::SafeDownCast(
    this->GetClientSideObject());
  vtkTransform* transform = vtkTransform::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetObjectFromID(
      this->GetSubProxy("Transform")->GetID()));
  repr->GetTransform(transform);

  this->Superclass::UpdatePropertyInformation();
}

//----------------------------------------------------------------------------
void vtkSMSlabRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


