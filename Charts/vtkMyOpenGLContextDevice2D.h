/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMyOpenGLContextDevice2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkMyOpenGLContextDevice2D - Class for drawing 2D primitives using OpenGL.
//
// .SECTION Description
// This class takes care of drawing the 2D primitives for the vtkContext2D class.
// In general this class should not be used directly, but called by vtkContext2D
// which takes care of many of the higher level details.

#ifndef __vtkMyOpenGLContextDevice2D_h
#define __vtkMyOpenGLContextDevice2D_h

#include "vtkOpenGLContextDevice2D.h"

class vtkWindow;
class vtkViewport;
class vtkRenderer;
class vtkLabelRenderStrategy;
class vtkOpenGLRenderWindow;
class vtkOpenGLExtensionManager;

class VTK_CHARTS_EXPORT vtkMyOpenGLContextDevice2D : public vtkOpenGLContextDevice2D
{
public:
  vtkTypeMacro(vtkMyOpenGLContextDevice2D, vtkOpenGLContextDevice2D);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a 2D Painter object.
  static vtkMyOpenGLContextDevice2D *New();

  // Description:
  // Draw the supplied image at the given x, y (p[0], p[1]) location(s) (bottom corner).
  virtual void DrawImage(float *p, int n, vtkImageData *image);


//BTX
protected:
  vtkMyOpenGLContextDevice2D();
  virtual ~vtkMyOpenGLContextDevice2D();


private:
  vtkMyOpenGLContextDevice2D(const vtkMyOpenGLContextDevice2D &); // Not implemented.
  void operator=(const vtkMyOpenGLContextDevice2D &);   // Not implemented.

//ETX
};

#endif //__vtkMyOpenGLContextDevice2D_h
