/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMSlabRepresentationProxy.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSlabRepresentationProxy - proxy for vtkSlabRepresentation
// .SECTION Description
// vtkSMSlabRepresentationProxy is a proxy for vtkSlabRepresentation. A
// specialization is needed to set the tranform on the vtkSlabRepresentation.

#ifndef __vtkSMSlabRepresentationProxy_h
#define __vtkSMSlabRepresentationProxy_h

#include "vtkSMWidgetRepresentationProxy.h"

class VTK_EXPORT vtkSMSlabRepresentationProxy : public vtkSMWidgetRepresentationProxy
{
public:
  static vtkSMSlabRepresentationProxy* New();
  vtkTypeRevisionMacro(vtkSMSlabRepresentationProxy, vtkSMWidgetRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void UpdateVTKObjects()
    {this->Superclass::UpdateVTKObjects();}
  virtual void UpdatePropertyInformation();
  virtual void UpdatePropertyInformation(vtkSMProperty* prop)
    { this->Superclass::UpdatePropertyInformation(prop); }

//BTX
protected:
  vtkSMSlabRepresentationProxy();
  ~vtkSMSlabRepresentationProxy();

  // This method is overridden to set the transform on the vtkWidgetRepresentation.
  virtual void CreateVTKObjects();
  virtual void UpdateVTKObjects(vtkClientServerStream&);

private:
  vtkSMSlabRepresentationProxy(const vtkSMSlabRepresentationProxy&); // Not implemented
  void operator=(const vtkSMSlabRepresentationProxy&); // Not implemented
//ETX
};

#endif

