/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMyOpenGLContextDevice2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMyOpenGLContextDevice2D.h"

#ifdef VTK_USE_QT
# include <QApplication>
# include "vtkQtLabelRenderStrategy.h"
#endif
#include "vtkFreeTypeLabelRenderStrategy.h"

#include "vtkVector.h"
#include "vtkPen.h"
#include "vtkPoints2D.h"
#include "vtkMatrix3x3.h"
#include "vtkFloatArray.h"
#include "vtkSmartPointer.h"

#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include "vtkViewport.h"
#include "vtkWindow.h"

#include "vtkTexture.h"
#include "vtkImageData.h"

#include "vtkRenderer.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkShaderProgram2.h"
#include "vtkgl.h"

#include "vtkObjectFactory.h"
#include "vtkContextBufferId.h"
#include "vtkOpenGLContextBufferId.h"


//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMyOpenGLContextDevice2D);

//-----------------------------------------------------------------------------
vtkMyOpenGLContextDevice2D::vtkMyOpenGLContextDevice2D()
{
}

//-----------------------------------------------------------------------------
vtkMyOpenGLContextDevice2D::~vtkMyOpenGLContextDevice2D()
{
}


//-----------------------------------------------------------------------------
void vtkMyOpenGLContextDevice2D::DrawImage(float *p, int, vtkImageData *image)
{
  vtkTexture *tex =vtkTexture::New();
  tex->SetInput(image);
  tex->Render(this->Renderer);
  int *extent = image->GetExtent();
  float points[] = { p[0]          , p[1],
                     p[0]+2.0*extent[1], p[1],
                     p[0]+2.0*extent[1], p[1]+2.0*extent[3],
                     p[0]          , p[1]+2.0*extent[3] };

  float texCoord[] = { 0.0, 0.0,
                       1.0, 0.0,
                       1.0, 1.0,
                       0.0, 1.0 };

  glColor4ub(255, 255, 255, 255);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glVertexPointer(2, GL_FLOAT, 0, &points[0]);
  glTexCoordPointer(2, GL_FLOAT, 0, &texCoord[0]);
  glDrawArrays(GL_QUADS, 0, 4);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);

  tex->PostRender(this->Renderer);
  glDisable(GL_TEXTURE_2D);
  tex->Delete();
}


//-----------------------------------------------------------------------------
void vtkMyOpenGLContextDevice2D::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Renderer: ";
  if (this->Renderer)
  {
    os << endl;
    this->Renderer->PrintSelf(os, indent.GetNextIndent());
  }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "Text Renderer: ";
  if (this->Renderer)
  {
    os << endl;
    this->TextRenderer->PrintSelf(os, indent.GetNextIndent());
  }
  else
    {
    os << "(none)" << endl;
    }
}
