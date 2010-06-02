/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqSlabWidget.cxx,v $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqSlabWidget.h"
#include "ui_pqSlabWidget.h"

// Server Manager Includes.
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"

// VTK Includes
#include "vtkTransform.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"

// Qt Includes.
#include <QDoubleValidator>
#include <QString>

// ParaView Includes.
#include "pq3DWidgetFactory.h"
#include "pqApplicationCore.h"
#include "pqPropertyLinks.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"

class pqSlabWidget::pqImplementation : public Ui::pqSlabWidget
{
public:
  pqPropertyLinks Links;
};


#define PVBOXWIDGET_TRIGGER_RENDER(ui)  \
  QObject::connect(this->Implementation->ui,\
    SIGNAL(editingFinished()),\
    this, SLOT(render()), Qt::QueuedConnection);
//-----------------------------------------------------------------------------
pqSlabWidget::pqSlabWidget(vtkSMProxy* refProxy, vtkSMProxy* pxy, QWidget* _parent) :
  Superclass(refProxy, pxy, _parent)
{
  this->Implementation = new pqImplementation();
  this->Implementation->setupUi(this);
  this->Implementation->show3DWidget->setChecked(this->widgetVisible());  

  // Setup validators for all line edits.
  QDoubleValidator* validator = new QDoubleValidator(this);
  this->Implementation->positionX->setValidator(validator);
  this->Implementation->positionY->setValidator(validator);
  this->Implementation->positionZ->setValidator(validator);
  this->Implementation->scaleX->setValidator(validator);
  this->Implementation->scaleY->setValidator(validator);
  this->Implementation->scaleZ->setValidator(validator);
  this->Implementation->rotationX->setValidator(validator);
  this->Implementation->rotationY->setValidator(validator);
  this->Implementation->rotationZ->setValidator(validator);

  PVBOXWIDGET_TRIGGER_RENDER(positionX);
  PVBOXWIDGET_TRIGGER_RENDER(positionY);
  PVBOXWIDGET_TRIGGER_RENDER(positionZ);
  PVBOXWIDGET_TRIGGER_RENDER(scaleX);
  PVBOXWIDGET_TRIGGER_RENDER(scaleY);
  PVBOXWIDGET_TRIGGER_RENDER(scaleZ);
  PVBOXWIDGET_TRIGGER_RENDER(rotationX);
  PVBOXWIDGET_TRIGGER_RENDER(rotationY);
  PVBOXWIDGET_TRIGGER_RENDER(rotationZ);

  QObject::connect(this->Implementation->show3DWidget,
    SIGNAL(toggled(bool)), this, SLOT(setWidgetVisible(bool)));

  QObject::connect(this, SIGNAL(widgetVisibilityChanged(bool)),
    this, SLOT(onWidgetVisibilityChanged(bool)));

  QObject::connect(this->Implementation->xAxis,
    SIGNAL(clicked()), this, SLOT(onXAxis()));
  QObject::connect(this->Implementation->yAxis,
    SIGNAL(clicked()), this, SLOT(onYAxis()));
  QObject::connect(this->Implementation->zAxis,
    SIGNAL(clicked()), this, SLOT(onZAxis()));

  this->SliderDivisions = 100;
  this->Implementation->slider->setRange(-(this->SliderDivisions/2.0),this->SliderDivisions/2.0);
  this->Implementation->slider->setValue(0);
  this->PreviousSliderValue = 0;
  QObject::connect(this->Implementation->slider,
    SIGNAL(valueChanged(int)), this, SLOT(onTranslate(int)));

  QObject::connect(this->Implementation->resetBounds,
    SIGNAL(clicked()), this, SLOT(resetBounds()));

  QObject::connect(this, SIGNAL(widgetInteraction()),
    this, SLOT(updateTranslationSlider()));

  QObject::connect(&this->Implementation->Links, SIGNAL(qtWidgetChanged()),
    this, SLOT(setModified()));

  // QObject::connect(&this->Implementation->Links, SIGNAL(qtWidgetChanged()),
  //   this, SLOT(updateTranslationSlider()));

  pqServerManagerModel* smmodel =
    pqApplicationCore::instance()->getServerManagerModel();
  this->createWidget(smmodel->findServer(refProxy->GetConnectionID()));
  
  this->updateTranslationSlider();
}

//-----------------------------------------------------------------------------
pqSlabWidget::~pqSlabWidget()
{
  delete this->Implementation;
}

#define PVBOXWIDGET_LINK(ui, smproperty, index)\
{\
  this->Implementation->Links.addPropertyLink(\
    this->Implementation->ui, "text2",\
    SIGNAL(textChanged(const QString&)),\
    widget, widget->GetProperty(smproperty), index);\
}

//-----------------------------------------------------------------------------
void pqSlabWidget::createWidget(pqServer* server)
{
  vtkSMNewWidgetRepresentationProxy* widget =
    pqApplicationCore::instance()->get3DWidgetFactory()->
    get3DWidget("SlabWidgetRepresentation", server);
  this->setWidgetProxy(widget);

  widget->UpdateVTKObjects();
  widget->UpdatePropertyInformation();

  PVBOXWIDGET_LINK(positionX, "Position", 0);
  PVBOXWIDGET_LINK(positionY, "Position", 1);
  PVBOXWIDGET_LINK(positionZ, "Position", 2);

  PVBOXWIDGET_LINK(rotationX, "Rotation", 0);
  PVBOXWIDGET_LINK(rotationY, "Rotation", 1);
  PVBOXWIDGET_LINK(rotationZ, "Rotation", 2);

  PVBOXWIDGET_LINK(scaleX, "Scale", 0);
  PVBOXWIDGET_LINK(scaleY, "Scale", 1);
  PVBOXWIDGET_LINK(scaleZ, "Scale", 2);
}

//-----------------------------------------------------------------------------
// update widget bounds.
void pqSlabWidget::select()
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  double input_bounds[6];
  if (widget  && this->getReferenceInputBounds(input_bounds))
    {
    vtkSMPropertyHelper(widget, "PlaceWidget").Set(input_bounds, 6);
    widget->UpdateVTKObjects();
    }

  this->Superclass::select();
}

#define VTK_ABS(a) ((a >= 0) ? a : -a)

#define VTK_AVERAGE(a,b,c) \
  c[0] = (a[0] + b[0])/2.0; \
  c[1] = (a[1] + b[1])/2.0; \
  c[2] = (a[2] + b[2])/2.0;

//-----------------------------------------------------------------------------
void pqSlabWidget::onXAxis()
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  
  double pts[9];
  vtkSMPropertyHelper(widget, "PointsInfo").Get(pts,9);
  double InitialBounds[6];
  vtkSMPropertyHelper(widget, "InitialBoundsInfo").Get(InitialBounds,6);

  vtkTransform *newT = vtkTransform::New();
  double *p0 = pts;
  double *p1 = pts + 3*1;
  double *p6 = pts + 3*2;
  double p14[3];
  VTK_AVERAGE(p0,p6,p14);
  double offset[3], translate[3], scaleX[3], scale[3];
  double initCenter[3];
  int ii;

  // The transformation is relative to the initial bounds.
  // Initial bounds are set when PlaceWidget() is invoked.
  
  // Translation
  for (ii=0; ii < 3; ii++)
    {
    initCenter[ii] = (InitialBounds[2*ii+1]+InitialBounds[2*ii]) / 2.0;
    offset[ii] = p14[ii] - initCenter[ii];
    }

  double maxVal = 0;
  for (ii=0; ii < 3; ii++) 
    {
    if (VTK_ABS(offset[ii]) > VTK_ABS(maxVal)) 
      {
      maxVal = offset[ii];
      }
    }
  double sign = (maxVal > 0) ? 1.0 : -1.0;

  translate[0] = sign*vtkMath::Norm(offset) + initCenter[0];
  translate[1] = initCenter[1];
  translate[2] = initCenter[2];

  // Scaling
  for (ii=0; ii < 3; ii++)
    {
    scaleX[ii] = (p1[ii] - p0[ii]);
    }

  scale[0] = vtkMath::Norm(scaleX) / (InitialBounds[1]-InitialBounds[0]);
  scale[1] = 1.0;
  scale[2] = 1.0;

  // Create the new transform and apply
  newT->Identity();
  // Include initial center for now, and will subtract it off later
  newT->Translate(translate[0], translate[1], translate[2]);
  newT->RotateY(0);
  newT->Scale(scale[0],scale[1],scale[2]);
  // Add back in the contribution due to non-origin center
  newT->Translate(-initCenter[0], -initCenter[1], -initCenter[2]);

  double pos[3], rot[3], sca[3];
  newT->GetPosition(pos);
  newT->GetOrientation(rot);
  newT->GetScale(sca);
  
  this->Implementation->positionX->setText(QVariant(pos[0]).toString());
  this->Implementation->positionY->setText(QVariant(pos[1]).toString());
  this->Implementation->positionZ->setText(QVariant(pos[2]).toString());
  this->Implementation->rotationX->setText(QVariant(rot[0]).toString());
  this->Implementation->rotationY->setText(QVariant(rot[1]).toString());
  this->Implementation->rotationZ->setText(QVariant(rot[2]).toString());
  this->Implementation->scaleX->setText(QVariant(sca[0]).toString());
  this->Implementation->scaleY->setText(QVariant(sca[1]).toString());
  this->Implementation->scaleZ->setText(QVariant(sca[2]).toString());
  
  newT->Delete();
  widget->UpdateVTKObjects();
  widget->UpdatePropertyInformation();
  pqApplicationCore::instance()->render();
  this->updateTranslationSlider();
  this->setModified();
}

//-----------------------------------------------------------------------------
void pqSlabWidget::onYAxis()
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  
  double pts[9];
  vtkSMPropertyHelper(widget, "PointsInfo").Get(pts,9);
  double InitialBounds[6];
  vtkSMPropertyHelper(widget, "InitialBoundsInfo").Get(InitialBounds,6);

  vtkTransform *newT = vtkTransform::New();
  double *p0 = pts;
  double *p1 = pts + 3*1;
  double *p6 = pts + 3*2;
  double p14[3];
  VTK_AVERAGE(p0,p6,p14);
  double offset[3], translate[3], scaleX[3], scale[3];
  double initCenter[3];
  int ii;

  // The transformation is relative to the initial bounds.
  // Initial bounds are set when PlaceWidget() is invoked.
  
  // Translation
  for (ii=0; ii < 3; ii++)
    {
    initCenter[ii] = (InitialBounds[2*ii+1]+InitialBounds[2*ii]) / 2.0;
    offset[ii] = p14[ii] - initCenter[ii];
    }

  double maxVal = 0;
  for (ii=0; ii < 3; ii++) 
    {
    if (VTK_ABS(offset[ii]) > VTK_ABS(maxVal)) 
      {
      maxVal = offset[ii];
      }
    }
  double sign = (maxVal > 0) ? 1.0 : -1.0;

  translate[0] = initCenter[0];
  translate[1] = sign*vtkMath::Norm(offset) + initCenter[1];
  translate[2] = initCenter[2];

  // Scaling
  for (ii=0; ii < 3; ii++)
    {
    scaleX[ii] = (p1[ii] - p0[ii]);
    }

  scale[0] = vtkMath::Norm(scaleX) / (InitialBounds[1]-InitialBounds[0]);
  scale[1] = (InitialBounds[1]-InitialBounds[0]) / (InitialBounds[3]-InitialBounds[2]);
  scale[2] = 1.0;

  // Create the new transform and apply
  newT->Identity();
  // Include initial center for now, and will subtract it off later
  newT->Translate(translate[0], translate[1], translate[2]);
  newT->RotateZ(90);
  newT->Scale(scale[0],scale[1],scale[2]);
  // Add back in the contribution due to non-origin center
  newT->Translate(-initCenter[0], -initCenter[1], -initCenter[2]);
  
  double pos[3], rot[3], sca[3];
  newT->GetPosition(pos);
  newT->GetOrientation(rot);
  newT->GetScale(sca);
  
  this->Implementation->positionX->setText(QVariant(pos[0]).toString());
  this->Implementation->positionY->setText(QVariant(pos[1]).toString());
  this->Implementation->positionZ->setText(QVariant(pos[2]).toString());
  this->Implementation->rotationX->setText(QVariant(rot[0]).toString());
  this->Implementation->rotationY->setText(QVariant(rot[1]).toString());
  this->Implementation->rotationZ->setText(QVariant(rot[2]).toString());
  this->Implementation->scaleX->setText(QVariant(sca[0]).toString());
  this->Implementation->scaleY->setText(QVariant(sca[1]).toString());
  this->Implementation->scaleZ->setText(QVariant(sca[2]).toString());
  
  newT->Delete();
  widget->UpdateVTKObjects();
  widget->UpdatePropertyInformation();
  pqApplicationCore::instance()->render();
  this->updateTranslationSlider();
  this->setModified();
}

//-----------------------------------------------------------------------------
void pqSlabWidget::onZAxis()
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  
  double pts[9];
  vtkSMPropertyHelper(widget, "PointsInfo").Get(pts,9);
  double InitialBounds[6];
  vtkSMPropertyHelper(widget, "InitialBoundsInfo").Get(InitialBounds,6);

  vtkTransform *newT = vtkTransform::New();
  double *p0 = pts;
  double *p1 = pts + 3*1;
  double *p6 = pts + 3*2;
  double p14[3];
  VTK_AVERAGE(p0,p6,p14);
  double offset[3], translate[3], scaleX[3], scale[3];
  double initCenter[3];
  int ii;

  // The transformation is relative to the initial bounds.
  // Initial bounds are set when PlaceWidget() is invoked.
  
  // Translation
  for (ii=0; ii < 3; ii++)
    {
    initCenter[ii] = (InitialBounds[2*ii+1]+InitialBounds[2*ii]) / 2.0;
    offset[ii] = p14[ii] - initCenter[ii];
    }

  double maxVal = 0;
  for (ii=0; ii < 3; ii++) 
    {
    if (VTK_ABS(offset[ii]) > VTK_ABS(maxVal)) 
      {
      maxVal = offset[ii];
      }
    }
  double sign = (maxVal > 0) ? 1.0 : -1.0;

  translate[0] = initCenter[0];
  translate[1] = initCenter[1];
  translate[2] = sign*vtkMath::Norm(offset) + initCenter[2];

  // Scaling
  for (ii=0; ii < 3; ii++)
    {
    scaleX[ii] = (p1[ii] - p0[ii]);
    }

  scale[0] = vtkMath::Norm(scaleX) / (InitialBounds[1]-InitialBounds[0]);
  scale[1] = 1.0;
  scale[2] = (InitialBounds[1]-InitialBounds[0]) / (InitialBounds[5]-InitialBounds[4]);

  // Create the new transform and apply
  newT->Identity();
  // Include initial center for now, and will subtract it off later
  newT->Translate(translate[0], translate[1], translate[2]);
  newT->RotateY(90);
  newT->Scale(scale[0],scale[1],scale[2]);
  // Add back in the contribution due to non-origin center
  newT->Translate(-initCenter[0], -initCenter[1], -initCenter[2]);
  
  double pos[3], rot[3], sca[3];
  newT->GetPosition(pos);
  newT->GetOrientation(rot);
  newT->GetScale(sca);
  
  this->Implementation->positionX->setText(QVariant(pos[0]).toString());
  this->Implementation->positionY->setText(QVariant(pos[1]).toString());
  this->Implementation->positionZ->setText(QVariant(pos[2]).toString());
  this->Implementation->rotationX->setText(QVariant(rot[0]).toString());
  this->Implementation->rotationY->setText(QVariant(rot[1]).toString());
  this->Implementation->rotationZ->setText(QVariant(rot[2]).toString());
  this->Implementation->scaleX->setText(QVariant(sca[0]).toString());
  this->Implementation->scaleY->setText(QVariant(sca[1]).toString());
  this->Implementation->scaleZ->setText(QVariant(sca[2]).toString());
  
  newT->Delete();
  widget->UpdateVTKObjects();
  widget->UpdatePropertyInformation();
  pqApplicationCore::instance()->render();
  this->updateTranslationSlider();
  this->setModified();
}

//-----------------------------------------------------------------------------
void pqSlabWidget::onTranslate(int value)
{
  this->Implementation->slider->blockSignals(true);
  
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  
  double pts[9];
  vtkSMPropertyHelper(widget, "PointsInfo").Get(pts,9);
  
  // DEBUG
  // printf("pqSlabWidget:Points:%3.2f %3.2f %3.2f\n",pts[0], pts[1], pts[2]);

  double *p0 = pts;
  double *p1 = pts + 3*1;
  double *p6 = pts + 3*2;
  double p14[3];
  VTK_AVERAGE(p0,p6,p14);
  double offset[3], translate[3], rotate[3], scale[3];
  double initCenter[3];
  int ii;

  double InitialBounds[6];
  vtkSMPropertyHelper(widget, "InitialBoundsInfo").Get(InitialBounds,6);

  double Normals[9], N[3][3];
  vtkSMPropertyHelper(widget, "NormalsInfo").Get(Normals,9);

  // DEBUG
  // printf("pqSlabWidget:Normals:%3.2f %3.2f %3.2f\n",Normals[0], Normals[1], Normals[2]);

  // The transformation is relative to the initial bounds.
  // Initial bounds are set when PlaceWidget() is invoked.
  
  // Translation
  for (ii=0; ii < 3; ii++)
    {
    initCenter[ii] = (InitialBounds[2*ii+1]+InitialBounds[2*ii]) / 2.0;
    offset[ii] = p14[ii] - initCenter[ii];
    }

  for (ii=0; ii<3; ii++)
    {
    N[ii][0] = Normals[0+3*ii]; 
    N[ii][1] = Normals[1+3*ii]; 
    N[ii][2] = Normals[2+3*ii];
    }
  double mag = (float)value - (float)this->PreviousSliderValue;
  double vel[3];
  
  double dir[3] = { 1 , 0 , 0};
  // Routine stolen from vtkBoxRepresentation to calculate direction to move
  this->GetDirection(N[0],N[1],N[2],dir);

  // DEBUG
  // this->Implementation->label_n0->setText(QVariant(dir[0]).toString());
  // this->Implementation->label_n1->setText(QVariant(dir[1]).toString());
  // this->Implementation->label_n2->setText(QVariant(dir[2]).toString());
  

  //  Move offset point in same direction as dir 
  for (ii=0; ii<3; ii++)
    {
    // Scale mag of movement here if using more divisions in slider range than microns
    vel[ii] = (mag/1.0)*dir[ii];
    }
  // Routine adapted from vtkBoxRepresentation
  this->MovePoint(vel,dir,offset);
  
  // Start building transformation matrix with new offset vector
  // Include initial center for now, and will subtract it off later
  for (ii=0; ii<3; ii++)
    {
    translate[ii] = offset[ii] + initCenter[ii];
    }

  // Instead of calculating, just grab scale from current transform
  scale[0] = this->Implementation->scaleX->text().toDouble();
  scale[1] = this->Implementation->scaleY->text().toDouble();
  scale[2] = this->Implementation->scaleZ->text().toDouble();

  // Create the new transform and apply
  vtkTransform *newT = vtkTransform::New();
  vtkMatrix4x4 *Matrix = vtkMatrix4x4::New();
  
  newT->Identity();
  // Include initial center for now, and will subtract it off later
  newT->Translate(translate[0], translate[1], translate[2]);

  // Orientation
  Matrix->Identity();
  for (ii=0; ii<3; ii++)
    {
    Matrix->SetElement(ii,0,N[0][ii]);
    Matrix->SetElement(ii,1,N[1][ii]);
    Matrix->SetElement(ii,2,N[2][ii]);
    }
  newT->Concatenate(Matrix);

  newT->Scale(scale[0],scale[1],scale[2]);
  // Add back in the contribution due to non-origin center
  newT->Translate(-initCenter[0], -initCenter[1], -initCenter[2]);
  
  double pos[3], rot[3], sca[3];
  newT->GetPosition(pos);
  newT->GetOrientation(rot);
  newT->GetScale(sca);
  
  this->Implementation->positionX->setText(QVariant(pos[0]).toString());
  this->Implementation->positionY->setText(QVariant(pos[1]).toString());
  this->Implementation->positionZ->setText(QVariant(pos[2]).toString());
  this->Implementation->rotationX->setText(QVariant(rot[0]).toString());
  this->Implementation->rotationY->setText(QVariant(rot[1]).toString());
  this->Implementation->rotationZ->setText(QVariant(rot[2]).toString());
  this->Implementation->scaleX->setText(QVariant(sca[0]).toString());
  this->Implementation->scaleY->setText(QVariant(sca[1]).toString());
  this->Implementation->scaleZ->setText(QVariant(sca[2]).toString());
  
  newT->Delete();
  this->PreviousSliderValue = value;
  widget->UpdateVTKObjects();
  widget->UpdatePropertyInformation();
  pqApplicationCore::instance()->render();
  this->setModified();

  this->Implementation->slider->blockSignals(false);

}

//-----------------------------------------------------------------------------
void pqSlabWidget::updateTranslationSlider()
{
  this->Implementation->slider->blockSignals(true);
  
  // Calculate the position of the slab wrt bounds in normal direction
  // and set slider value to correct relative position
  // Also, maybe set range to a reasonable value based on bounds...
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  
  double pts[9];
  vtkSMPropertyHelper(widget, "PointsInfo").Get(pts,9);
  
  double *p0 = pts;
  double *p1 = pts + 3*1;
  double *p6 = pts + 3*2;
  double p14[3];
  VTK_AVERAGE(p0,p6,p14);
  double offset[3], translate[3], rotate[3], scale[3];
  double initCenter[3];
  int ii;

  double InitialBounds[6];
  vtkSMPropertyHelper(widget, "InitialBoundsInfo").Get(InitialBounds,6);

  double Normals[9], N[3][3];
  vtkSMPropertyHelper(widget, "NormalsInfo").Get(Normals,9);

  // The transformation is relative to the initial bounds.
  // Initial bounds are set when PlaceWidget() is invoked.
  
  // Translation
  for (ii=0; ii < 3; ii++)
    {
    initCenter[ii] = (InitialBounds[2*ii+1]+InitialBounds[2*ii]) / 2.0;
    offset[ii] = p14[ii] - initCenter[ii];
    }

  for (ii=0; ii<3; ii++)
    {
    N[ii][0] = Normals[0+3*ii]; 
    N[ii][1] = Normals[1+3*ii]; 
    N[ii][2] = Normals[2+3*ii];
    }
  
  double dir[3] = { 1 , 0 , 0};
  // Routine stolen from vtkBoxRepresentation to calculate direction to move
  this->GetDirection(N[0],N[1],N[2],dir);

  // Determine how much the slab is offset in the face normal direction
  double offsetAmt = vtkMath::Dot(dir,offset);
  
  // Find the InitialBounds corners with the largest offset in the "dir" direction
  // for calculating outside bounds on the translation slider
  double xIn[8][3];
  xIn[0][0] = InitialBounds[0]-initCenter[0]; xIn[0][1] = InitialBounds[2]-initCenter[1]; xIn[0][2] = InitialBounds[4]-initCenter[2];
  xIn[1][0] = InitialBounds[1]-initCenter[0]; xIn[1][1] = InitialBounds[2]-initCenter[1]; xIn[1][2] = InitialBounds[4]-initCenter[2];
  xIn[2][0] = InitialBounds[1]-initCenter[0]; xIn[2][1] = InitialBounds[3]-initCenter[1]; xIn[2][2] = InitialBounds[4]-initCenter[2];
  xIn[3][0] = InitialBounds[0]-initCenter[0]; xIn[3][1] = InitialBounds[3]-initCenter[1]; xIn[3][2] = InitialBounds[4]-initCenter[2];
  xIn[4][0] = InitialBounds[0]-initCenter[0]; xIn[4][1] = InitialBounds[2]-initCenter[1]; xIn[4][2] = InitialBounds[5]-initCenter[2];
  xIn[5][0] = InitialBounds[1]-initCenter[0]; xIn[5][1] = InitialBounds[2]-initCenter[1]; xIn[5][2] = InitialBounds[5]-initCenter[2];
  xIn[6][0] = InitialBounds[1]-initCenter[0]; xIn[6][1] = InitialBounds[3]-initCenter[1]; xIn[6][2] = InitialBounds[5]-initCenter[2];
  xIn[7][0] = InitialBounds[0]-initCenter[0]; xIn[7][1] = InitialBounds[3]-initCenter[1]; xIn[7][2] = InitialBounds[5]-initCenter[2];
  
  double maxDot = 0, minDot = 0;
  int maxDotIdx = -1, minDotIdx = -1;
  double dotResult;
  for (ii=0; ii<8; ii++)
    {
    dotResult = vtkMath::Dot(dir,xIn[ii]);
    if (dotResult > maxDot)
      {
      maxDot = dotResult;
      maxDotIdx = ii;
      }
    if (dotResult < minDot)
      {
      minDot = dotResult;
      minDotIdx = ii;
      }
    }
  
  // DEBUG
  printf("minDot = %3.2f, maxDot = %3.2f, offsetAmt = %3.2f\n", minDot, maxDot, offsetAmt);
  
  // If offsetAmt is outside of greatest bounds corners, then expand range of slider
  if ((offsetAmt > maxDot) || (offsetAmt < minDot))
    {
    this->Implementation->slider->setRange(-(int)VTK_ABS(offsetAmt),(int)VTK_ABS(offsetAmt));
    this->Implementation->slider->setValue((int)offsetAmt);
    this->PreviousSliderValue = (int)offsetAmt;    
    this->SliderDivisions = (int)(2*offsetAmt);
    }
  // else set slider range to bounds range, and place handle at correct relative position
  else
    {
    this->Implementation->slider->setRange((int)minDot,(int)maxDot);
    this->Implementation->slider->setValue((int)offsetAmt);
    this->PreviousSliderValue = (int)offsetAmt;
    this->SliderDivisions = (int)(maxDot - minDot);
    }
    
  this->Implementation->slider->blockSignals(false);
}

//-----------------------------------------------------------------------------
void pqSlabWidget::resetBounds(double input_bounds[6])
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  vtkSMPropertyHelper(widget, "PlaceWidget").Set(input_bounds, 6);
  widget->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqSlabWidget::cleanupWidget()
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  if(widget)
    {
    pqApplicationCore::instance()->get3DWidgetFactory()->
      free3DWidget(widget);
    }
  this->setWidgetProxy(0);
}

//-----------------------------------------------------------------------------
void pqSlabWidget::onWidgetVisibilityChanged(bool visible)
{
  this->Implementation->show3DWidget->blockSignals(true);
  this->Implementation->show3DWidget->setChecked(visible);
  this->Implementation->show3DWidget->blockSignals(false);
}

//-----------------------------------------------------------------------------
void pqSlabWidget::accept()
{
  this->Superclass::accept();
  this->hideHandles();
}

//-----------------------------------------------------------------------------
void pqSlabWidget::reset()
{
  this->Superclass::reset();
  this->hideHandles();
}

//-----------------------------------------------------------------------------
void pqSlabWidget::showHandles()
{
  /*
  vtkSMProxy* proxy = this->getWidgetProxy();
  if (proxy)
    {
    pqSMAdaptor::setElementProperty(proxy->GetProperty("HandleVisibility"), 1);
    proxy->UpdateVTKObjects();
    }
    */
}

//-----------------------------------------------------------------------------
void pqSlabWidget::hideHandles()
{
  /*
  vtkSMProxy* proxy = this->getWidgetProxy();
  if (proxy)
    {
    pqSMAdaptor::setElementProperty(proxy->GetProperty("HandleVisibility"), 1);
    proxy->UpdateVTKObjects();
    }
  */
}

//----------------------------------------------------------------------------
void pqSlabWidget::MovePoint(double *vel, double *dir, double *x)
  {
  int i;
  double v[3],v2[3];

  for (i=0; i<3; i++)
    {
    v[i] = vel[i];
    v2[i] = dir[i];
    }

  vtkMath::Normalize(v2);
  double f = vtkMath::Dot(v,v2);
  
  for (i=0; i<3; i++)
    {
    v[i] = f*v2[i];
  
    x[i] += v[i];
    }
}

//----------------------------------------------------------------------------
void pqSlabWidget::GetDirection(const double Nx[3],const double Ny[3], 
                                        const double Nz[3], double dir[3])
{
  double dotNy, dotNz;
  double y[3];

  if(vtkMath::Dot(Nx,Nx)!=0)
    {
    dir[0] = Nx[0];
    dir[1] = Nx[1];
    dir[2] = Nx[2];
    }
  else 
    {
    dotNy = vtkMath::Dot(Ny,Ny);
    dotNz = vtkMath::Dot(Nz,Nz);
    if(dotNy != 0 && dotNz != 0)
      {
      vtkMath::Cross(Ny,Nz,dir);
      }
    else if(dotNy != 0)
      {
      //dir must have been initialized to the 
      //corresponding coordinate direction before calling
      //this method
      vtkMath::Cross(Ny,dir,y);
      vtkMath::Cross(y,Ny,dir);
      }
    else if(dotNz != 0)
      {
      //dir must have been initialized to the 
      //corresponding coordinate direction before calling
      //this method
      vtkMath::Cross(Nz,dir,y);
      vtkMath::Cross(y,Nz,dir);
      }
    }
}

