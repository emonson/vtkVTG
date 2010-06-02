/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqSlabWidget.h,v $

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
#ifndef __pqSlabWidget_h 
#define __pqSlabWidget_h

#include "pq3DWidget.h"

class pqServer;

/// Provides UI for Slab Widget (based on Box Widget)
class PQCOMPONENTS_EXPORT pqSlabWidget : public pq3DWidget
{
  Q_OBJECT
  typedef pq3DWidget Superclass;
public:
  pqSlabWidget(vtkSMProxy* refProxy, vtkSMProxy* proxy, QWidget* p = 0);
  virtual ~pqSlabWidget();

  /// Resets the bounds of the 3D widget to the reference proxy bounds.
  /// This typically calls PlaceWidget on the underlying 3D Widget 
  /// with reference proxy bounds.
  /// This should be explicitly called after the panel is created
  /// and the widget is initialized i.e. the reference proxy, controlled proxy
  /// and hints have been set.
  virtual void resetBounds(double bounds[6]);
  virtual void resetBounds()
    { this->Superclass::resetBounds(); }

  /// accept the changes. Overridden to hide handles.
  virtual void accept();

  /// reset the changes. Overridden to hide handles.
  virtual void reset();

  /// Overridden to update widget placement based on data bounds.
  virtual void select();

public slots:
  void onXAxis();
  void onYAxis();
  void onZAxis();
  void onTranslate(int value);
  void updateTranslationSlider();

protected:
  /// Internal method to create the widget.
  void createWidget(pqServer*);
  
  /// Internal method to cleanup widget.
  void cleanupWidget();

private slots:
  /// Called when the user changes widget visibility
  void onWidgetVisibilityChanged(bool visible);

  void showHandles();
  void hideHandles();
private:
  int SliderDivisions;	// For translation slider (int value and range)
  int PreviousSliderValue;
  pqSlabWidget(const pqSlabWidget&); // Not implemented.
  void operator=(const pqSlabWidget&); // Not implemented.
  //"dir" is the direction in which the face can be moved i.e. the axis passing
  //through the center
  void MovePoint(double *vel, double *dir, double *x);
  //Helper method to obtain the direction in which the face is to be moved.
  //Handles special cases where some of the scale factors are 0.
  void GetDirection(const double Nx[3],const double Ny[3], 
                                        const double Nz[3], double dir[3]);

  class pqImplementation;
  pqImplementation* Implementation;
};

#endif


