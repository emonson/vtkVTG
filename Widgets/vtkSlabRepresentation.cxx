/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSlabRepresentation.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSlabRepresentation.h"
#include "vtkActor.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyData.h"
#include "vtkCallbackCommand.h"
#include "vtkSlab.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkInteractorObserver.h"
#include "vtkMath.h"
#include "vtkCellArray.h"
#include "vtkCellPicker.h"
#include "vtkTransform.h"
#include "vtkDoubleArray.h"
#include "vtkPlanes.h"
#include "vtkCamera.h"
#include "vtkAssemblyPath.h"
#include "vtkWindow.h"
#include "vtkObjectFactory.h"


vtkCxxRevisionMacro(vtkSlabRepresentation, "$Revision: 1.8 $");
vtkStandardNewMacro(vtkSlabRepresentation);

//----------------------------------------------------------------------------
vtkSlabRepresentation::vtkSlabRepresentation()
{
  // The initial state
  this->InteractionState = vtkSlabRepresentation::Outside;

  // Handle size is in pixels for this widget
  this->HandleSize = 5.0;

  // Control orientation of normals
  this->InsideOut = 0;
  this->OutlineFaceWires = 0;
  this->OutlineCursorWires = 1;

  // Set up the initial properties
  this->CreateDefaultProperties();

  // Construct the poly data representing the hex
  this->HexPolyData = vtkPolyData::New();
  this->HexMapper = vtkPolyDataMapper::New();
  this->HexMapper->SetInput(HexPolyData);
  this->HexActor = vtkActor::New();
  this->HexActor->SetMapper(this->HexMapper);
  this->HexActor->SetProperty(this->OutlineProperty);

  // Construct initial points
  this->Points = vtkPoints::New(VTK_DOUBLE);
  this->Points->SetNumberOfPoints(10);//8 corners; 2 faces
  this->HexPolyData->SetPoints(this->Points);
  
  // Construct connectivity for the faces. These are used to perform
  // the picking.
  int i;
  vtkIdType pts[4];
  vtkCellArray *cells = vtkCellArray::New();
  cells->Allocate(cells->EstimateSize(6,4));
  pts[0] = 3; pts[1] = 0; pts[2] = 4; pts[3] = 7;
  cells->InsertNextCell(4,pts);
  pts[0] = 1; pts[1] = 2; pts[2] = 6; pts[3] = 5;
  cells->InsertNextCell(4,pts);
  pts[0] = 0; pts[1] = 1; pts[2] = 5; pts[3] = 4;
  cells->InsertNextCell(4,pts);
  pts[0] = 2; pts[1] = 3; pts[2] = 7; pts[3] = 6;
  cells->InsertNextCell(4,pts);
  pts[0] = 0; pts[1] = 3; pts[2] = 2; pts[3] = 1;
  cells->InsertNextCell(4,pts);
  pts[0] = 4; pts[1] = 5; pts[2] = 6; pts[3] = 7;
  cells->InsertNextCell(4,pts);
  this->HexPolyData->SetPolys(cells);
  cells->Delete();
  this->HexPolyData->BuildCells();
  
  // The face of the hexahedra
  cells = vtkCellArray::New();
  cells->Allocate(cells->EstimateSize(1,4));
  cells->InsertNextCell(4,pts); //temporary, replaced later
  this->HexFacePolyData = vtkPolyData::New();
  this->HexFacePolyData->SetPoints(this->Points);
  this->HexFacePolyData->SetPolys(cells);
  this->HexFaceMapper = vtkPolyDataMapper::New();
  this->HexFaceMapper->SetInput(HexFacePolyData);
  this->HexFace = vtkActor::New();
  this->HexFace->SetMapper(this->HexFaceMapper);
  this->HexFace->SetProperty(this->FaceProperty);
  cells->Delete();

  // Create the outline for the hex
  this->OutlinePolyData = vtkPolyData::New();
  this->OutlinePolyData->SetPoints(this->Points);
  this->OutlineMapper = vtkPolyDataMapper::New();
  this->OutlineMapper->SetInput(this->OutlinePolyData);
  this->HexOutline = vtkActor::New();
  this->HexOutline->SetMapper(this->OutlineMapper);
  this->HexOutline->SetProperty(this->OutlineProperty);
  cells = vtkCellArray::New();
  cells->Allocate(cells->EstimateSize(15,2));
  this->OutlinePolyData->SetLines(cells);
  cells->Delete();

  // Create the outline
  this->GenerateOutline();

  // Create the handles
  this->Handle = new vtkActor* [2];
  this->HandleMapper = new vtkPolyDataMapper* [2];
  this->HandleGeometry = new vtkSphereSource* [2];
  for (i=0; i<2; i++)
    {
    this->HandleGeometry[i] = vtkSphereSource::New();
    this->HandleGeometry[i]->SetThetaResolution(16);
    this->HandleGeometry[i]->SetPhiResolution(8);
    this->HandleMapper[i] = vtkPolyDataMapper::New();
    this->HandleMapper[i]->SetInput(this->HandleGeometry[i]->GetOutput());
    this->Handle[i] = vtkActor::New();
    this->Handle[i]->SetMapper(this->HandleMapper[i]);
    }
  
  // Define the point coordinates
  double bounds[6];
  bounds[0] = -0.5;
  bounds[1] = 0.5;
  bounds[2] = -0.5;
  bounds[3] = 0.5;
  bounds[4] = -0.5;
  bounds[5] = 0.5;
  // Points 8-14 are down by PositionHandles();
  this->BoundingBox = vtkSlab::New();
  this->PlaceWidget(bounds);

  //Manage the picking stuff
  this->HandlePicker = vtkCellPicker::New();
  this->HandlePicker->SetTolerance(0.001);
  for (i=0; i<2; i++)
    {
    this->HandlePicker->AddPickList(this->Handle[i]);
    }
  this->HandlePicker->PickFromListOn();

  this->HexPicker = vtkCellPicker::New();
  this->HexPicker->SetTolerance(0.001);
  this->HexPicker->AddPickList(HexActor);
  this->HexPicker->PickFromListOn();
  
  this->CurrentHandle = NULL;

  // Internal data memebers for performance
  this->Transform = vtkTransform::New();
  this->PlanePoints = vtkPoints::New(VTK_DOUBLE);
  this->PlanePoints->SetNumberOfPoints(6);
  this->PlaneNormals = vtkDoubleArray::New();
  this->PlaneNormals->SetNumberOfComponents(3);
  this->PlaneNormals->SetNumberOfTuples(6);
  this->Matrix = vtkMatrix4x4::New();
}

//----------------------------------------------------------------------------
vtkSlabRepresentation::~vtkSlabRepresentation()
{  
  this->HexActor->Delete();
  this->HexMapper->Delete();
  this->HexPolyData->Delete();
  this->Points->Delete();

  this->HexFace->Delete();
  this->HexFaceMapper->Delete();
  this->HexFacePolyData->Delete();

  this->HexOutline->Delete();
  this->OutlineMapper->Delete();
  this->OutlinePolyData->Delete();
  
  for (int i=0; i<2; i++)
    {
    this->HandleGeometry[i]->Delete();
    this->HandleMapper[i]->Delete();
    this->Handle[i]->Delete();
    }
  delete [] this->Handle;
  delete [] this->HandleMapper;
  delete [] this->HandleGeometry;
  
  this->HandlePicker->Delete();
  this->HexPicker->Delete();

  this->Transform->Delete();
  this->BoundingBox->Delete();
  this->PlanePoints->Delete();
  this->PlaneNormals->Delete();
  this->Matrix->Delete();
  
  this->HandleProperty->Delete();
  this->SelectedHandleProperty->Delete();
  this->FaceProperty->Delete();
  this->SelectedFaceProperty->Delete();
  this->OutlineProperty->Delete();
  this->SelectedOutlineProperty->Delete();
}

//----------------------------------------------------------------------
void vtkSlabRepresentation::GetPolyData(vtkPolyData *pd)
{
  pd->SetPoints(this->HexPolyData->GetPoints());
  pd->SetPolys(this->HexPolyData->GetPolys());
}

//----------------------------------------------------------------------
void vtkSlabRepresentation::StartWidgetInteraction(double e[2])
{
  // Store the start position
  this->StartEventPosition[0] = e[0];
  this->StartEventPosition[1] = e[1];
  this->StartEventPosition[2] = 0.0;

  // Store the start position
  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;

  this->ComputeInteractionState(static_cast<int>(e[0]),static_cast<int>(e[1]),0);
}

//----------------------------------------------------------------------
void vtkSlabRepresentation::WidgetInteraction(double e[2])
{
  // Convert events to appropriate coordinate systems
  vtkCamera *camera = this->Renderer->GetActiveCamera();
  if ( !camera )
    {
    return;
    }
  double focalPoint[4], pickPoint[4], prevPickPoint[4];
  double z, vpn[3];
  camera->GetViewPlaneNormal(vpn);

  // Compute the two points defining the motion vector
  double pos[3];
  if ( this->LastPicker == this->HexPicker )
    {
    this->HexPicker->GetPickPosition(pos);
    }
  else
    {
    this->HandlePicker->GetPickPosition(pos);
    }
  vtkInteractorObserver::ComputeWorldToDisplay(this->Renderer,
                                               pos[0], pos[1], pos[2],
                                               focalPoint);
  z = focalPoint[2];
  vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer,this->LastEventPosition[0],
                                               this->LastEventPosition[1], z, prevPickPoint);
  vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, e[0], e[1], z, pickPoint);

  // Process the motion
  if ( this->InteractionState == vtkSlabRepresentation::MoveF0 )
    {
    this->MoveMinusXFace(prevPickPoint,pickPoint);
    }

  else if ( this->InteractionState == vtkSlabRepresentation::MoveF1 )
    {
    this->MovePlusXFace(prevPickPoint,pickPoint);
    }

  else if ( this->InteractionState == vtkSlabRepresentation::MoveF2 )
    {
    this->MoveMinusYFace(prevPickPoint,pickPoint);
    }

  else if ( this->InteractionState == vtkSlabRepresentation::MoveF3 )
    {
    this->MovePlusYFace(prevPickPoint,pickPoint);
    }

  else if ( this->InteractionState == vtkSlabRepresentation::MoveF4 )
    {
    this->MoveMinusZFace(prevPickPoint,pickPoint);
    }

  else if ( this->InteractionState == vtkSlabRepresentation::MoveF5 )
    {
    this->MovePlusZFace(prevPickPoint,pickPoint);
    }

  else if ( this->InteractionState == vtkSlabRepresentation::TransX )
    {
    this->TranslateX(prevPickPoint,pickPoint);
    }

  else if ( this->InteractionState == vtkSlabRepresentation::TransY )
    {
    this->TranslateY(prevPickPoint,pickPoint);
    }

  else if ( this->InteractionState == vtkSlabRepresentation::TransZ )
    {
    this->TranslateZ(prevPickPoint,pickPoint);
    }

  else if ( this->InteractionState == vtkSlabRepresentation::Translating )
    {
    this->Translate(prevPickPoint, pickPoint);
    }

  else if ( this->InteractionState == vtkSlabRepresentation::Scaling )
    {
    this->Scale(prevPickPoint, pickPoint, 
                static_cast<int>(e[0]), static_cast<int>(e[1]));
    }

  else if ( this->InteractionState == vtkSlabRepresentation::Rotating )
    {
    this->Rotate(static_cast<int>(e[0]), static_cast<int>(e[1]), prevPickPoint, pickPoint, vpn);
    }

  else if ( this->InteractionState == vtkSlabRepresentation::RotAbtX )
    {
    this->RotateAboutX(static_cast<int>(e[0]), static_cast<int>(e[1]), prevPickPoint, pickPoint, vpn);
    }

  else if ( this->InteractionState == vtkSlabRepresentation::RotAbtY )
    {
    this->RotateAboutY(static_cast<int>(e[0]), static_cast<int>(e[1]), prevPickPoint, pickPoint, vpn);
    }

  else if ( this->InteractionState == vtkSlabRepresentation::RotAbtZ )
    {
    this->RotateAboutZ(static_cast<int>(e[0]), static_cast<int>(e[1]), prevPickPoint, pickPoint, vpn);
    }

  // Store the start position
  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::MoveFace(double *p1, double *p2, double *dir, 
                                    double *x1, double *x2, double *x3, double *x4,
                                    double *x5)
  {
  int i;
  double v[3], v2[3];

  for (i=0; i<3; i++)
    {
    v[i] = p2[i] - p1[i];
    v2[i] = dir[i];
    }

  vtkMath::Normalize(v2);
  double f = vtkMath::Dot(v,v2);
  
  for (i=0; i<3; i++)
    {
    v[i] = f*v2[i];
  
    x1[i] += v[i];
    x2[i] += v[i];
    x3[i] += v[i];
    x4[i] += v[i];
    x5[i] += v[i];
    }
  this->PositionHandles();
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::GetDirection(const double Nx[3],const double Ny[3], 
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

#define VTK_AVERAGE(a,b,c) \
  c[0] = (a[0] + b[0])/2.0; \
  c[1] = (a[1] + b[1])/2.0; \
  c[2] = (a[2] + b[2])/2.0;

//----------------------------------------------------------------------------
void vtkSlabRepresentation::MovePlusXFace(double *p1, double *p2)
{
  double *pts =
    static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);

  double *x1 = pts + 3*1;
  double *x2 = pts + 3*2;
  double *x3 = pts + 3*5;
  double *x4 = pts + 3*6;
  double h1[3];
  VTK_AVERAGE(x1,x4,h1);
  
  double dir[3] = { 1 , 0 , 0};
  this->ComputeNormals();
  this->GetDirection(this->N[1],this->N[3],this->N[5],dir);
  this->MoveFace(p1,p2,dir,x1,x2,x3,x4,h1);
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::MoveMinusXFace(double *p1, double *p2)
{
  double *pts =
    static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);

  double *x1 = pts + 3*0;
  double *x2 = pts + 3*3;
  double *x3 = pts + 3*4;
  double *x4 = pts + 3*7;
  double h1[3];
  VTK_AVERAGE(x1,x4,h1);
  
  double dir[3]={-1,0,0};
  this->ComputeNormals();
  this->GetDirection(this->N[0],this->N[4],this->N[2],dir);

  this->MoveFace(p1,p2,dir,x1,x2,x3,x4,h1);
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::TranslateX(double *p1, double *p2)
{
  double *pts =
    static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);

  double *x1 = pts + 3*1;
  double *x2 = pts + 3*2;
  double *x3 = pts + 3*5;
  double *x4 = pts + 3*6;
  double h1[3];
  VTK_AVERAGE(x1,x4,h1);
  
  double *y1 = pts + 3*0;
  double *y2 = pts + 3*3;
  double *y3 = pts + 3*4;
  double *y4 = pts + 3*7;
  double i1[3];
  VTK_AVERAGE(y1,y4,i1);
  
  double dir[3] = { 1 , 0 , 0};
  this->ComputeNormals();
  this->GetDirection(this->N[1],this->N[3],this->N[5],dir);

  this->MoveFace(p1,p2,dir,x1,x2,x3,x4,h1);
  this->MoveFace(p1,p2,dir,y1,y2,y3,y4,i1);
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::MovePlusYFace(double *p1, double *p2)
{
  double *pts =
     static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);

  double *x1 = pts + 3*2;
  double *x2 = pts + 3*3;
  double *x3 = pts + 3*6;
  double *x4 = pts + 3*7;
  double h1[3];
  VTK_AVERAGE(x1,x4,h1);
  
  double dir[3]={0,1,0};
  this->ComputeNormals();
  this->GetDirection(this->N[3],this->N[5],this->N[1],dir);

  this->MoveFace(p1,p2,dir,x1,x2,x3,x4,h1);
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::MoveMinusYFace(double *p1, double *p2)
{
  double *pts =
    static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);

  double *x1 = pts + 3*0;
  double *x2 = pts + 3*1;
  double *x3 = pts + 3*4;
  double *x4 = pts + 3*5;
  double h1[3];
  VTK_AVERAGE(x1,x4,h1);

  double dir[3] = {0, -1, 0};
  this->ComputeNormals();
  this->GetDirection(this->N[2],this->N[0],this->N[4],dir);

  this->MoveFace(p1,p2,dir,x1,x2,x3,x4,h1);
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::TranslateY(double *p1, double *p2)
{
  double *pts =
     static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);

  double *x1 = pts + 3*2;
  double *x2 = pts + 3*3;
  double *x3 = pts + 3*6;
  double *x4 = pts + 3*7;
  double h1[3];
  VTK_AVERAGE(x1,x4,h1);
  
  double *y1 = pts + 3*0;
  double *y2 = pts + 3*1;
  double *y3 = pts + 3*4;
  double *y4 = pts + 3*5;
  double i1[3];
  VTK_AVERAGE(y1,y4,i1);

  double dir[3]={0,1,0};
  this->ComputeNormals();
  this->GetDirection(this->N[3],this->N[5],this->N[1],dir);

  this->MoveFace(p1,p2,dir,x1,x2,x3,x4,h1);
  this->MoveFace(p1,p2,dir,y1,y2,y3,y4,i1);
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::MovePlusZFace(double *p1, double *p2)
{
  double *pts =
    static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);

  double *x1 = pts + 3*4;
  double *x2 = pts + 3*5;
  double *x3 = pts + 3*6;
  double *x4 = pts + 3*7;
  double h1[3];
  VTK_AVERAGE(x2,x4,h1);

  double dir[3]={0,0,1};
  this->ComputeNormals();
  this->GetDirection(this->N[5],this->N[1],this->N[3],dir);

  this->MoveFace(p1,p2,dir,x1,x2,x3,x4,h1);
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::MoveMinusZFace(double *p1, double *p2)
{
  double *pts =
    static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);

  double *x1 = pts + 3*0;
  double *x2 = pts + 3*1;
  double *x3 = pts + 3*2;
  double *x4 = pts + 3*3;
  double h1[3];
  VTK_AVERAGE(x2,x4,h1);

  double dir[3]={0,0,-1};
  this->ComputeNormals();
  this->GetDirection(this->N[4],this->N[2],this->N[0],dir);

  this->MoveFace(p1,p2,dir,x1,x2,x3,x4,h1);
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::TranslateZ(double *p1, double *p2)
{
  double *pts =
    static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);

  double *x1 = pts + 3*4;
  double *x2 = pts + 3*5;
  double *x3 = pts + 3*6;
  double *x4 = pts + 3*7;
  double h1[3];
  VTK_AVERAGE(x2,x4,h1);

  double *y1 = pts + 3*0;
  double *y2 = pts + 3*1;
  double *y3 = pts + 3*2;
  double *y4 = pts + 3*3;
  double i1[3];
  VTK_AVERAGE(y2,y4,i1);

  double dir[3]={0,0,1};
  this->ComputeNormals();
  this->GetDirection(this->N[5],this->N[1],this->N[3],dir);

  this->MoveFace(p1,p2,dir,x1,x2,x3,x4,h1);
  this->MoveFace(p1,p2,dir,y1,y2,y3,y4,i1);
}

//----------------------------------------------------------------------------
// Loop through all points and translate them
void vtkSlabRepresentation::Translate(double *p1, double *p2)
{
  double *pts =
    static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);
  double v[3];

  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];
  
  // Move the corners
  for (int i=0; i<8; i++)
    {
    *pts++ += v[0];
    *pts++ += v[1];
    *pts++ += v[2];
    }
  this->PositionHandles();
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::Scale(double *vtkNotUsed(p1),
                                 double *vtkNotUsed(p2),
                                 int vtkNotUsed(X),
                                 int Y)
{
  double *pts =
      static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);
  double *p0 = pts;
  double *p6 = pts + 3*6;
  double center[3];
  VTK_AVERAGE(p0,p6,center);
  double sf;

  if ( Y > this->LastEventPosition[1] )
    {
    sf = 1.03;
    }
  else
    {
    sf = 0.97;
    }
  
  // Move the corners
  for (int i=0; i<8; i++, pts+=3)
    {
    pts[0] = sf * (pts[0] - center[0]) + center[0];
    pts[1] = sf * (pts[1] - center[1]) + center[1];
    pts[2] = sf * (pts[2] - center[2]) + center[2];
    }
  this->PositionHandles();
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::ComputeNormals()
{
  double *pts =
     static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);
  double *p0 = pts;
  double *px = pts + 3*1;
  double *py = pts + 3*3;
  double *pz = pts + 3*4;
  int i;
  
  for (i=0; i<3; i++)
    {
    this->N[0][i] = p0[i] - px[i];
    this->N[2][i] = p0[i] - py[i];
    this->N[4][i] = p0[i] - pz[i];
    }
  vtkMath::Normalize(this->N[0]);
  vtkMath::Normalize(this->N[2]);
  vtkMath::Normalize(this->N[4]);
  for (i=0; i<3; i++)
    {
    this->N[1][i] = -this->N[0][i];
    this->N[3][i] = -this->N[2][i];
    this->N[5][i] = -this->N[4][i];
    }
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::GetPlanes(vtkPlanes *planes)
{
  if ( ! planes )
    {
    return;
    }
  
  double *pts =
     static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);
  double *p0 = pts;
  double *p1 = pts + 3*1;
  double *p2 = pts + 3*2;
  double *p3 = pts + 3*3;
  //double *p4 = pts + 3*4;
  double *p5 = pts + 3*5;
  double *p6 = pts + 3*6;
  double *p7 = pts + 3*7;
  double x[3];

  VTK_AVERAGE(p0,p7,x);
  this->PlanePoints->SetPoint(0, x);
  VTK_AVERAGE(p1,p6,x);
  this->PlanePoints->SetPoint(1, x);
  VTK_AVERAGE(p0,p5,x);
  this->PlanePoints->SetPoint(2, x);
  VTK_AVERAGE(p2,p7,x);
  this->PlanePoints->SetPoint(3, x);
  VTK_AVERAGE(p1,p3,x);
  this->PlanePoints->SetPoint(4, x);
  VTK_AVERAGE(p5,p7,x);
  this->PlanePoints->SetPoint(5, x);

  this->ComputeNormals();

  // Set the normals and coordinate values
  double factor = (this->InsideOut ? -1.0 : 1.0);
  for (int i=0; i<6; i++)
    {
    this->PlaneNormals->SetTuple3(i, factor*this->N[i][0],
                                  factor*this->N[i][1], factor*this->N[i][2]);
    }
    
  planes->SetPoints(this->PlanePoints);
  planes->SetNormals(this->PlaneNormals);
  planes->Modified();
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::Rotate(int X,
                                  int Y,
                                  double *p1,
                                  double *p2,
                                  double *vpn)
{
  double *pts =
    static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);
  double *p0 = pts;
  double *p6 = pts + 3*6;
  double center[3];
  VTK_AVERAGE(p0,p6,center);
  double v[3]; //vector of motion
  double axis[3]; //axis of rotation
  double theta; //rotation angle
  int i;

  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // Create axis of rotation and angle of rotation
  vtkMath::Cross(vpn,v,axis);
  if ( vtkMath::Normalize(axis) == 0.0 )
    {
    return;
    }
  int *size = this->Renderer->GetSize();
  double l2 = (X-this->LastEventPosition[0])*(X-this->LastEventPosition[0])
             + (Y-this->LastEventPosition[1])*(Y-this->LastEventPosition[1]);
  theta = 360.0 * sqrt(l2/(size[0]*size[0]+size[1]*size[1]));

  //Manipulate the transform to reflect the rotation
  this->Transform->Identity();
  this->Transform->Translate(center[0],center[1],center[2]);
  this->Transform->RotateWXYZ(theta,axis);
  this->Transform->Translate(-center[0],-center[1],-center[2]);

  //Set the corners
  vtkPoints *newPts = vtkPoints::New(VTK_DOUBLE);
  this->Transform->TransformPoints(this->Points,newPts);

  for (i=0; i<8; i++, pts+=3)
    {
    this->Points->SetPoint(i, newPts->GetPoint(i));
    }

  newPts->Delete();
  this->PositionHandles();
}
  
#define VTK_SCALE_VECTOR(a,b,c) \
  c[0] = a[0]*b; \
  c[1] = a[1]*b; \
  c[2] = a[2]*b;
  
//----------------------------------------------------------------------------
void vtkSlabRepresentation::RotateAboutX(int X,
                                         int Y,
                                         double *p1,
                                         double *p2,
                                         double *vpn)
{
  double *pts =
    static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);
  double *p0 = pts;
  double *p6 = pts + 3*6;
  double center[3];
  VTK_AVERAGE(p0,p6,center);
  double v[3]; //vector of motion
  
  // Position the eight points of the box and then update the
  // position of the other handles.
  double initCenter[3];
  double *bounds=this->InitialBounds;
  initCenter[0] = (bounds[1] + bounds[0])/2.0;
  initCenter[1] = (bounds[3] + bounds[2])/2.0;
  initCenter[2] = (bounds[5] + bounds[4])/2.0;
  
  double rotAxis[3] = {1, 0, 0}; // slab axis of rotation 
  double naturalAxis[3]; // natural free axis of rotation for mouse movement
  double rotSign; // scaling for positive or negative axis vector
  double axis[3]; // resutant actual axis of rotation
  this->ComputeNormals();
  this->GetDirection(this->N[1],this->N[3],this->N[5],rotAxis);
  if ( vtkMath::Normalize(rotAxis) == 0.0 )
    {
    return;
    }
  double theta; //rotation angle
  int i;

  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // Compute correct direction of rotation
  vtkMath::Cross(vpn,v,naturalAxis);
  if ( vtkMath::Normalize(naturalAxis) == 0.0 )
    {
    return;
    }
  rotSign = vtkMath::Dot(rotAxis,naturalAxis);
  VTK_SCALE_VECTOR(rotAxis,rotSign,axis);
  
  int *size = this->Renderer->GetSize();
  double l2 = (X-this->LastEventPosition[0])*(X-this->LastEventPosition[0])
             + (Y-this->LastEventPosition[1])*(Y-this->LastEventPosition[1]);
  theta = 360.0 * sqrt(l2/(size[0]*size[0]+size[1]*size[1]));

  //Manipulate the transform to reflect the rotation
  this->Transform->Identity();
  this->Transform->Translate(initCenter[0],initCenter[1],initCenter[2]);
  this->Transform->RotateWXYZ(theta,axis);
  this->Transform->Translate(-initCenter[0],-initCenter[1],-initCenter[2]);

  //Set the corners
  vtkPoints *newPts = vtkPoints::New(VTK_DOUBLE);
  this->Transform->TransformPoints(this->Points,newPts);

  for (i=0; i<8; i++, pts+=3)
    {
    this->Points->SetPoint(i, newPts->GetPoint(i));
    }

  newPts->Delete();
  this->PositionHandles();
}
  
//----------------------------------------------------------------------------
void vtkSlabRepresentation::RotateAboutY(int X,
                                         int Y,
                                         double *p1,
                                         double *p2,
                                         double *vpn)
{
  double *pts =
    static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);
  double *p0 = pts;
  double *p6 = pts + 3*6;
  double center[3];
  VTK_AVERAGE(p0,p6,center);
  double v[3]; //vector of motion

  // Position the eight points of the box and then update the
  // position of the other handles.
  double initCenter[3];
  double *bounds=this->InitialBounds;
  initCenter[0] = (bounds[1] + bounds[0])/2.0;
  initCenter[1] = (bounds[3] + bounds[2])/2.0;
  initCenter[2] = (bounds[5] + bounds[4])/2.0;

  double rotAxis[3] = {0, 1, 0}; // slab axis of rotation 
  double naturalAxis[3]; // natural free axis of rotation for mouse movement
  double rotSign; // scaling for positive or negative axis vector
  double axis[3]; // resutant actual axis of rotation
  this->ComputeNormals();
  this->GetDirection(this->N[3],this->N[5],this->N[1],rotAxis);
  if ( vtkMath::Normalize(rotAxis) == 0.0 )
    {
    return;
    }
  double theta; //rotation angle
  int i;

  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // Compute correct direction of rotation
  vtkMath::Cross(vpn,v,naturalAxis);
  if ( vtkMath::Normalize(naturalAxis) == 0.0 )
    {
    return;
    }
  rotSign = vtkMath::Dot(rotAxis,naturalAxis);
  VTK_SCALE_VECTOR(rotAxis,rotSign,axis);

  int *size = this->Renderer->GetSize();
  double l2 = (X-this->LastEventPosition[0])*(X-this->LastEventPosition[0])
             + (Y-this->LastEventPosition[1])*(Y-this->LastEventPosition[1]);
  theta = 360.0 * sqrt(l2/(size[0]*size[0]+size[1]*size[1]));

  //Manipulate the transform to reflect the rotation
  this->Transform->Identity();
  this->Transform->Translate(initCenter[0],initCenter[1],initCenter[2]);
  this->Transform->RotateWXYZ(theta,axis);
  this->Transform->Translate(-initCenter[0],-initCenter[1],-initCenter[2]);

  //Set the corners
  vtkPoints *newPts = vtkPoints::New(VTK_DOUBLE);
  this->Transform->TransformPoints(this->Points,newPts);

  for (i=0; i<8; i++, pts+=3)
    {
    this->Points->SetPoint(i, newPts->GetPoint(i));
    }

  newPts->Delete();
  this->PositionHandles();
}
  
//----------------------------------------------------------------------------
void vtkSlabRepresentation::RotateAboutZ(int X,
                                         int Y,
                                         double *p1,
                                         double *p2,
                                         double *vpn)
{
  double *pts =
    static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);
  double *p0 = pts;
  double *p6 = pts + 3*6;
  double center[3];
  VTK_AVERAGE(p0,p6,center);
  double v[3]; //vector of motion

  // Position the eight points of the box and then update the
  // position of the other handles.
  double initCenter[3];
  double *bounds=this->InitialBounds;
  initCenter[0] = (bounds[1] + bounds[0])/2.0;
  initCenter[1] = (bounds[3] + bounds[2])/2.0;
  initCenter[2] = (bounds[5] + bounds[4])/2.0;

  double rotAxis[3] = {0, 0, 1}; // slab axis of rotation 
  double naturalAxis[3]; // natural free axis of rotation for mouse movement
  double rotSign; // scaling for positive or negative axis vector
  double axis[3]; // resutant actual axis of rotation
  this->ComputeNormals();
  this->GetDirection(this->N[5],this->N[1],this->N[3],rotAxis);
  if ( vtkMath::Normalize(rotAxis) == 0.0 )
    {
    return;
    }
  double theta; //rotation angle
  int i;

  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // Compute correct direction of rotation
  vtkMath::Cross(vpn,v,naturalAxis);
  if ( vtkMath::Normalize(naturalAxis) == 0.0 )
    {
    return;
    }
  rotSign = vtkMath::Dot(rotAxis,naturalAxis);
  VTK_SCALE_VECTOR(rotAxis,rotSign,axis);
  
  int *size = this->Renderer->GetSize();
  double l2 = (X-this->LastEventPosition[0])*(X-this->LastEventPosition[0])
             + (Y-this->LastEventPosition[1])*(Y-this->LastEventPosition[1]);
  theta = 360.0 * sqrt(l2/(size[0]*size[0]+size[1]*size[1]));

  //Manipulate the transform to reflect the rotation
  this->Transform->Identity();
  this->Transform->Translate(initCenter[0],initCenter[1],initCenter[2]);
  this->Transform->RotateWXYZ(theta,axis);
  this->Transform->Translate(-initCenter[0],-initCenter[1],-initCenter[2]);

  //Set the corners
  vtkPoints *newPts = vtkPoints::New(VTK_DOUBLE);
  this->Transform->TransformPoints(this->Points,newPts);

  for (i=0; i<8; i++, pts+=3)
    {
    this->Points->SetPoint(i, newPts->GetPoint(i));
    }

  newPts->Delete();
  this->PositionHandles();
}
  
#define VTK_ABS(a) ((a >= 0) ? a : -a)

//----------------------------------------------------------------------------
void vtkSlabRepresentation::RotateToXAxis(int test)
{
  double *pts =
    static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);
  vtkTransform *newT = vtkTransform::New();
  double *p0 = pts;
  double *p1 = pts + 3*1;
  double *p6 = pts + 3*6;
  double p14[3];
  VTK_AVERAGE(p0,p6,p14);
  double offset[3], translate[3], scaleX[3], scale[3];
  double initCenter[3];
  int ii;

  // DEBUG
  // printf("vtkSlabRepresentation::RotateToXAxis()\n");

  // The transformation is relative to the initial bounds.
  // Initial bounds are set when PlaceWidget() is invoked.
  
  // Translation
  for (ii=0; ii < 3; ii++)
    {
    initCenter[ii] = (this->InitialBounds[2*ii+1]+this->InitialBounds[2*ii]) / 2.0;
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

  scale[0] = vtkMath::Norm(scaleX) / (this->InitialBounds[1]-this->InitialBounds[0]);
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
  SetTransform(newT);

  newT->Delete();
  this->PositionHandles();
}
  
//----------------------------------------------------------------------------
void vtkSlabRepresentation::RotateToYAxis(int test)
{
  double *pts =
    static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);
  vtkTransform *newT = vtkTransform::New();
  double *p0 = pts;
  double *p1 = pts + 3*1;
  double *p6 = pts + 3*6;
  double p14[3];
  VTK_AVERAGE(p0,p6,p14);
  double offset[3], translate[3], scaleX[3], scale[3];
  double initCenter[3];
  int ii;

  // DEBUG
  // printf("vtkSlabRepresentation::RotateToYAxis()\n");

  // The transformation is relative to the initial bounds.
  // Initial bounds are set when PlaceWidget() is invoked.
  
  // Translation
  for (ii=0; ii < 3; ii++)
    {
    initCenter[ii] = (this->InitialBounds[2*ii+1]+this->InitialBounds[2*ii]) / 2.0;
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

  scale[0] = vtkMath::Norm(scaleX) / (this->InitialBounds[1]-this->InitialBounds[0]);
  scale[1] = (this->InitialBounds[1]-this->InitialBounds[0]) / (this->InitialBounds[3]-this->InitialBounds[2]);
  scale[2] = 1.0;

  // Create the new transform and apply
  newT->Identity();
  // Include initial center for now, and will subtract it off later
  newT->Translate(translate[0], translate[1], translate[2]);
  newT->RotateZ(90);
  newT->Scale(scale[0],scale[1],scale[2]);
  // Add back in the contribution due to non-origin center
  newT->Translate(-initCenter[0], -initCenter[1], -initCenter[2]);
  SetTransform(newT);

  newT->Delete();
  this->PositionHandles();
}
  
//----------------------------------------------------------------------------
void vtkSlabRepresentation::RotateToZAxis(int test)
{
  double *pts =
    static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);
  vtkTransform *newT = vtkTransform::New();
  double *p0 = pts;
  double *p1 = pts + 3*1;
  double *p6 = pts + 3*6;
  double p14[3];
  VTK_AVERAGE(p0,p6,p14);
  double offset[3], translate[3], scaleX[3], scale[3];
  double initCenter[3];
  int ii;

  // DEBUG
  // printf("vtkSlabRepresentation::RotateToZAxis()\n");

  // The transformation is relative to the initial bounds.
  // Initial bounds are set when PlaceWidget() is invoked.
  
  // Translation
  for (ii=0; ii < 3; ii++)
    {
    initCenter[ii] = (this->InitialBounds[2*ii+1]+this->InitialBounds[2*ii]) / 2.0;
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

  scale[0] = vtkMath::Norm(scaleX) / (this->InitialBounds[1]-this->InitialBounds[0]);
  scale[1] = 1.0;
  scale[2] = (this->InitialBounds[1]-this->InitialBounds[0]) / (this->InitialBounds[5]-this->InitialBounds[4]);

  // Create the new transform and apply
  newT->Identity();
  // Include initial center for now, and will subtract it off later
  newT->Translate(translate[0], translate[1], translate[2]);
  newT->RotateY(90);
  newT->Scale(scale[0],scale[1],scale[2]);
  // Add back in the contribution due to non-origin center
  newT->Translate(-initCenter[0], -initCenter[1], -initCenter[2]);
  SetTransform(newT);

  newT->Delete();
  this->PositionHandles();
}
  
//----------------------------------------------------------------------------
void vtkSlabRepresentation::CreateDefaultProperties()
{
  // Handle properties
  this->HandleProperty = vtkProperty::New();
  this->HandleProperty->SetColor(1,1,1);

  this->SelectedHandleProperty = vtkProperty::New();
  this->SelectedHandleProperty->SetColor(1,0,0);

  // Face properties
  this->FaceProperty = vtkProperty::New();
  this->FaceProperty->SetColor(1,1,1);
  this->FaceProperty->SetOpacity(0.0);

  this->SelectedFaceProperty = vtkProperty::New();
  this->SelectedFaceProperty->SetColor(1,1,0);
  this->SelectedFaceProperty->SetOpacity(0.25);
  
  // Outline properties
  this->OutlineProperty = vtkProperty::New();
  this->OutlineProperty->SetRepresentationToWireframe();
  this->OutlineProperty->SetAmbient(1.0);
  this->OutlineProperty->SetAmbientColor(1.0,1.0,1.0);
  this->OutlineProperty->SetLineWidth(2.0);

  this->SelectedOutlineProperty = vtkProperty::New();
  this->SelectedOutlineProperty->SetRepresentationToWireframe();
  this->SelectedOutlineProperty->SetAmbient(1.0);
  this->SelectedOutlineProperty->SetAmbientColor(0.0,1.0,0.0);
  this->SelectedOutlineProperty->SetLineWidth(2.0);
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::PlaceWidget(double bds[6])
{
  int i;
  double bounds[6], center[3];
  
  this->AdjustBounds(bds,bounds,center);
  
  this->Points->SetPoint(0, bounds[0], bounds[2], bounds[4]);
  this->Points->SetPoint(1, bounds[1], bounds[2], bounds[4]);
  this->Points->SetPoint(2, bounds[1], bounds[3], bounds[4]);
  this->Points->SetPoint(3, bounds[0], bounds[3], bounds[4]);
  this->Points->SetPoint(4, bounds[0], bounds[2], bounds[5]);
  this->Points->SetPoint(5, bounds[1], bounds[2], bounds[5]);
  this->Points->SetPoint(6, bounds[1], bounds[3], bounds[5]);
  this->Points->SetPoint(7, bounds[0], bounds[3], bounds[5]);

  for (i=0; i<6; i++)
    {
    this->InitialBounds[i] = bounds[i];
    }
  this->InitialLength = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                             (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                             (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));

  // Setting PointCorners for getting from PV
  this->SetPointCorners();
  this->PositionHandles();
  this->ComputeNormals();
  this->ValidPick = 1; //since we have set up widget
  this->SizeHandles();
}

//----------------------------------------------------------------------
// double *vtkSlabRepresentation::GetPointsArray()
// {
//   this->GetPointsArray(this->PointCorners);
//   return this->PointCorners;
// }
// 
// //----------------------------------------------------------------------
// void vtkSlabRepresentation::GetPointsArray(double pointConers[9]) const
// {
//   this->GetPointsArray(pointConers[0], pointConers[1], pointConers[2],
//                   		pointConers[3], pointConers[4], pointConers[5],
//                   		pointConers[6], pointConers[7], pointConers[8]);
// }
// 
// //----------------------------------------------------------------------
// void vtkSlabRepresentation::GetPointsArray(double &p0x, double &p0y, double &p0z,
//                       						double &p1x, double &p1y, double &p1z,
//                       						double &p6x, double &p6y, double &p6z) const
// {
//   double *pts =
//      static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);
//   double *p0 = pts;
//   double *p1 = pts + 3*1;
//   double *p6 = pts + 3*6;
//   
//   p0x = p0[0];
//   p0y = p0[1];
//   p0z = p0[2];
//   p1x = p1[0];
//   p1y = p1[1];
//   p1z = p1[2];
//   p6x = p6[0];
//   p6y = p6[1];
//   p6z = p6[2];
// }


//----------------------------------------------------------------------------
void vtkSlabRepresentation::GetTransform(vtkTransform *t)
{
  double *pts =
    static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);
  double *p0 = pts;
  double *p1 = pts + 3*1;
  double *p3 = pts + 3*3;
  double *p4 = pts + 3*4;
  double *p6 = pts + 3*6;
  double p14[3];
  VTK_AVERAGE(p0,p6,p14);
  double center[3], translate[3], scale[3], scaleVec[3][3];
  double InitialCenter[3];
  int i;

  // The transformation is relative to the initial bounds.
  // Initial bounds are set when PlaceWidget() is invoked.
  t->Identity();
  
  // Translation
  for (i=0; i<3; i++)
    {
    InitialCenter[i] = 
      (this->InitialBounds[2*i+1]+this->InitialBounds[2*i]) / 2.0;
    center[i] = p14[i] - InitialCenter[i];
    }
  translate[0] = center[0] + InitialCenter[0];
  translate[1] = center[1] + InitialCenter[1];
  translate[2] = center[2] + InitialCenter[2];
  t->Translate(translate[0], translate[1], translate[2]);
  
  // Orientation
  this->Matrix->Identity();
  this->PositionHandles();
  this->ComputeNormals();
  for (i=0; i<3; i++)
    {
    this->Matrix->SetElement(i,0,this->N[1][i]);
    this->Matrix->SetElement(i,1,this->N[3][i]);
    this->Matrix->SetElement(i,2,this->N[5][i]);
    }
  t->Concatenate(this->Matrix);

  // Scale
  for (i=0; i<3; i++)
    {
    scaleVec[0][i] = (p1[i] - p0[i]);
    scaleVec[1][i] = (p3[i] - p0[i]);
    scaleVec[2][i] = (p4[i] - p0[i]);
    }

  scale[0] = vtkMath::Norm(scaleVec[0]);
  if (this->InitialBounds[1] != this->InitialBounds[0])
    {
    scale[0] = scale[0] / (this->InitialBounds[1]-this->InitialBounds[0]);
    }
  scale[1] = vtkMath::Norm(scaleVec[1]);
  if (this->InitialBounds[3] != this->InitialBounds[2])
    {
    scale[1] = scale[1] / (this->InitialBounds[3]-this->InitialBounds[2]);
    }
  scale[2] = vtkMath::Norm(scaleVec[2]);
  if (this->InitialBounds[5] != this->InitialBounds[4])
    {
    scale[2] = scale[2] / (this->InitialBounds[5]-this->InitialBounds[4]);
    }
  t->Scale(scale[0],scale[1],scale[2]);
  
  // Add back in the contribution due to non-origin center
  t->Translate(-InitialCenter[0], -InitialCenter[1], -InitialCenter[2]);
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::SetTransform(vtkTransform* t)
{
  if (!t)
    {
    vtkErrorMacro(<<"vtkTransform t must be non-NULL");
    return;
    }

  double *pts =
     static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);
  double xIn[3];
  // make sure the transform is up-to-date before using it
  t->Update();

  // Position the eight points of the box and then update the
  // position of the other handles.
  double *bounds=this->InitialBounds;

  xIn[0] = bounds[0]; xIn[1] = bounds[2]; xIn[2] = bounds[4];
  t->InternalTransformPoint(xIn,pts);

  xIn[0] = bounds[1]; xIn[1]= bounds[2]; xIn[2] = bounds[4];
  t->InternalTransformPoint(xIn,pts+3);

  xIn[0] = bounds[1]; xIn[1]= bounds[3]; xIn[2] = bounds[4];
  t->InternalTransformPoint(xIn,pts+6);

  xIn[0] = bounds[0]; xIn[1]= bounds[3]; xIn[2] = bounds[4];
  t->InternalTransformPoint(xIn,pts+9);

  xIn[0] = bounds[0]; xIn[1]= bounds[2]; xIn[2] = bounds[5];
  t->InternalTransformPoint(xIn,pts+12);

  xIn[0] = bounds[1]; xIn[1]= bounds[2]; xIn[2] = bounds[5];
  t->InternalTransformPoint(xIn,pts+15);

  xIn[0] = bounds[1]; xIn[1]= bounds[3]; xIn[2] = bounds[5];
  t->InternalTransformPoint(xIn,pts+18);

  xIn[0] = bounds[0]; xIn[1]= bounds[3]; xIn[2] = bounds[5];
  t->InternalTransformPoint(xIn,pts+21);

  
  this->PositionHandles();
  // Setting PointCorners for getting from PV
  this->ComputeNormals();
  this->SetPointCorners();
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::SetPointCorners()
{
  double *pts =
     static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);

  // Setting PointCorners for getting from PV
  this->PointCorners[0] = *(pts + 0);
  this->PointCorners[1] = *(pts + 1);
  this->PointCorners[2] = *(pts + 2);
  this->PointCorners[3] = *(pts + 3);
  this->PointCorners[4] = *(pts + 4);
  this->PointCorners[5] = *(pts + 5);
  this->PointCorners[6] = *(pts + 18);
  this->PointCorners[7] = *(pts + 19);
  this->PointCorners[8] = *(pts + 20);

  // DEBUG
  // printf("vtkSlabRepresentation:Points:%3.2f %3.2f %3.2f\n",this->PointCorners[0], this->PointCorners[1],this->PointCorners[2]);

  this->ComputeNormals();
  
  // Setting PointCorners for getting from PV
  this->XFaceNormals[0] = this->N[1][0];
  this->XFaceNormals[1] = this->N[1][1];
  this->XFaceNormals[2] = this->N[1][2];
  this->XFaceNormals[3] = this->N[3][0];
  this->XFaceNormals[4] = this->N[3][1];
  this->XFaceNormals[5] = this->N[3][2];
  this->XFaceNormals[6] = this->N[5][0];
  this->XFaceNormals[7] = this->N[5][1];
  this->XFaceNormals[8] = this->N[5][2];

  // DEBUG
  // printf("vtkSlabRepresentation:Normals:%3.2f %3.2f %3.2f\n",this->XFaceNormals[0], this->XFaceNormals[1],this->XFaceNormals[2]);

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::SetOutlineFaceWires(int newValue)
{
  if (this->OutlineFaceWires != newValue)
    {
    this->OutlineFaceWires = newValue;
    this->Modified();
    // the outline is dependent on this value, so we have to regen
    this->GenerateOutline();
    }
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::SetOutlineCursorWires(int newValue)
{
  if (this->OutlineCursorWires != newValue)
    {
    this->OutlineCursorWires = newValue;
    this->Modified();
    // the outline is dependent on this value, so we have to regen
    this->GenerateOutline();
    }
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::GenerateOutline()
{
  // Whatever the case may be, we have to reset the Lines of the
  // OutlinePolyData (i.e. nuke all current line data)
  vtkCellArray *cells = this->OutlinePolyData->GetLines();
  cells->Reset();

  // Now the outline lines
  if ( ! this->OutlineFaceWires && ! this->OutlineCursorWires )
    {
    return;
    }

  vtkIdType pts[2];

  if ( this->OutlineFaceWires )
    {
    pts[0] = 0; pts[1] = 7;       //the -x face
    cells->InsertNextCell(2,pts);
    pts[0] = 3; pts[1] = 4;
    cells->InsertNextCell(2,pts);
    pts[0] = 1; pts[1] = 6;       //the +x face
    cells->InsertNextCell(2,pts);
    pts[0] = 2; pts[1] = 5;
    cells->InsertNextCell(2,pts);
    pts[0] = 1; pts[1] = 4;       //the -y face
    cells->InsertNextCell(2,pts);
    pts[0] = 0; pts[1] = 5;
    cells->InsertNextCell(2,pts);
    pts[0] = 3; pts[1] = 6;       //the +y face
    cells->InsertNextCell(2,pts);
    pts[0] = 2; pts[1] = 7;
    cells->InsertNextCell(2,pts);
    pts[0] = 0; pts[1] = 2;       //the -z face
    cells->InsertNextCell(2,pts);
    pts[0] = 1; pts[1] = 3;
    cells->InsertNextCell(2,pts);
    pts[0] = 4; pts[1] = 6;       //the +Z face
    cells->InsertNextCell(2,pts);
    pts[0] = 5; pts[1] = 7;
    cells->InsertNextCell(2,pts);
    }
  if ( this->OutlineCursorWires )
    {
    pts[0] = 8; pts[1] = 9;         //the x cursor line
    cells->InsertNextCell(2,pts);
    pts[0] = 10; pts[1] = 11;       //the y cursor line
    cells->InsertNextCell(2,pts);
    pts[0] = 12; pts[1] = 13;       //the z cursor line
    cells->InsertNextCell(2,pts);
    }
  this->OutlinePolyData->Modified();
  if ( this->OutlineProperty) 
    {
    this->OutlineProperty->SetRepresentationToWireframe();
    this->SelectedOutlineProperty->SetRepresentationToWireframe();
    }
}

//----------------------------------------------------------------------------
int vtkSlabRepresentation::ComputeInteractionState(int X, int Y, int modify)
{
  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then pick the bounding box.
  if (!this->Renderer || !this->Renderer->IsInViewport(X, Y))
    {
    this->InteractionState = vtkSlabRepresentation::Outside;
    return this->InteractionState;
    }
  
  vtkAssemblyPath *path;
  // Try and pick a handle first
  this->LastPicker = NULL;
  this->CurrentHandle = NULL;
  this->HandlePicker->Pick(X,Y,0.0,this->Renderer);
  path = this->HandlePicker->GetPath();
  if ( path != NULL )
    {
    this->ValidPick = 1;
    this->LastPicker = this->HandlePicker;
    this->CurrentHandle =
           reinterpret_cast<vtkActor *>(path->GetFirstNode()->GetViewProp());
    if ( this->CurrentHandle == this->Handle[0] )
      {
      this->InteractionState = vtkSlabRepresentation::MoveF0;
      }
    else if ( this->CurrentHandle == this->Handle[1] )
      {
      this->InteractionState = vtkSlabRepresentation::MoveF1;
      }
    }
  else //see if the hex is picked
    {
    this->HexPicker->Pick(X,Y,0.0,this->Renderer);
    path = this->HexPicker->GetPath();
    this->CurrentHexFace = this->HexPicker->GetCellId();
    if ( path != NULL )
      {
      this->LastPicker = this->HexPicker;
      this->ValidPick = 1;
      if ( !modify )
        {
		if ( this->CurrentHexFace == 0 )
		  {
		  this->InteractionState = vtkSlabRepresentation::TransX;
		  }
		else if ( this->CurrentHexFace == 1 )
		  {
		  this->InteractionState = vtkSlabRepresentation::TransX;
		  }
		else if ( this->CurrentHexFace == 2 )
		  {
		  this->InteractionState = vtkSlabRepresentation::RotAbtZ;
		  }
		else if ( this->CurrentHexFace == 3 )
		  {
		  this->InteractionState = vtkSlabRepresentation::RotAbtZ;
		  }
		else if ( this->CurrentHexFace == 4 )
		  {
		  this->InteractionState = vtkSlabRepresentation::RotAbtY;
		  }
		else if ( this->CurrentHexFace == 5 )
		  {
		  this->InteractionState = vtkSlabRepresentation::RotAbtY;
		  }
		else
		  this->InteractionState = vtkSlabRepresentation::Rotating;
        }
      else
        {
        this->CurrentHandle = this->Handle[6];
        this->InteractionState = vtkSlabRepresentation::Translating;
        }
      }
    else
      {
      this->InteractionState = vtkSlabRepresentation::Outside;
      }
    }

  return this->InteractionState;
}

//----------------------------------------------------------------------
void vtkSlabRepresentation::SetInteractionState(int state)
{
  // Clamp to allowable values
  state = ( state < vtkSlabRepresentation::Outside ? vtkSlabRepresentation::Outside : 
            (state > vtkSlabRepresentation::Scaling ? vtkSlabRepresentation::Scaling : state) );
  
  // Depending on state, highlight appropriate parts of representation
  int handle;
  this->InteractionState = state;
  switch (state)
    {
    case vtkSlabRepresentation::MoveF0:
    case vtkSlabRepresentation::MoveF1:
    case vtkSlabRepresentation::MoveF2:
    case vtkSlabRepresentation::MoveF3:
    case vtkSlabRepresentation::MoveF4:
    case vtkSlabRepresentation::MoveF5:
    case vtkSlabRepresentation::TransX:
    case vtkSlabRepresentation::TransY:
    case vtkSlabRepresentation::TransZ:
      this->HighlightOutline(0);
      handle = this->HighlightHandle(this->CurrentHandle);
      // this->HighlightFace(handle);
      this->HighlightFace(this->CurrentHexFace);
      break;
    case vtkSlabRepresentation::RotAbtX:
    case vtkSlabRepresentation::RotAbtY:
    case vtkSlabRepresentation::RotAbtZ:
    case vtkSlabRepresentation::Rotating:
      this->HighlightOutline(0);
      this->HighlightHandle(NULL);
      this->HighlightFace(this->HexPicker->GetCellId());
      break;
    case vtkSlabRepresentation::Translating:
    case vtkSlabRepresentation::Scaling:
      this->HighlightOutline(1);
      // this->HighlightHandle(this->Handle[6]);
      this->HighlightFace(-1);
      break;
    default:
      this->HighlightOutline(0);
      this->HighlightHandle(NULL);
      this->HighlightFace(-1);
    }
}

//----------------------------------------------------------------------
double *vtkSlabRepresentation::GetBounds()
{
  this->BuildRepresentation();
  this->BoundingBox->SetBounds(this->HexActor->GetBounds());
  return this->BoundingBox->GetBounds();
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::BuildRepresentation()
{
  // Rebuild only if necessary
  if ( this->GetMTime() > this->BuildTime ||
       (this->Renderer && this->Renderer->GetVTKWindow() &&
        (this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime ||
        this->Renderer->GetActiveCamera()->GetMTime() > this->BuildTime)) )
    {
    this->SizeHandles();
    this->BuildTime.Modified();
    }
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::ReleaseGraphicsResources(vtkWindow *w)
{
  this->HexActor->ReleaseGraphicsResources(w);
  this->HexOutline->ReleaseGraphicsResources(w);
  this->HexFace->ReleaseGraphicsResources(w);
  // render the handles
  for (int j=0; j<2; j++)
    {
    this->Handle[j]->ReleaseGraphicsResources(w);
    }

}

//----------------------------------------------------------------------------
int vtkSlabRepresentation::RenderOpaqueGeometry(vtkViewport *v)
{
  int count=0;
  this->BuildRepresentation();
  
  count += this->HexActor->RenderOpaqueGeometry(v);
  count += this->HexOutline->RenderOpaqueGeometry(v);
  count += this->HexFace->RenderOpaqueGeometry(v);
  // render the handles
  for (int j=0; j<2; j++)
    {
    count += this->Handle[j]->RenderOpaqueGeometry(v);
    }

  return count;
}

//----------------------------------------------------------------------------
int vtkSlabRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport *v)
{
  int count=0;
  this->BuildRepresentation();
  
  count += this->HexActor->RenderTranslucentPolygonalGeometry(v);
  count += this->HexOutline->RenderTranslucentPolygonalGeometry(v);
  count += this->HexFace->RenderTranslucentPolygonalGeometry(v);
  // render the handles
  for (int j=0; j<2; j++)
    {
    count += this->Handle[j]->RenderTranslucentPolygonalGeometry(v);
    }

  return count;
}

//----------------------------------------------------------------------------
int vtkSlabRepresentation::HasTranslucentPolygonalGeometry()
{
  int result=0;
  this->BuildRepresentation();

  result |= this->HexActor->HasTranslucentPolygonalGeometry();
  result |= this->HexOutline->HasTranslucentPolygonalGeometry();

  // If the face is not selected, we are not really rendering translucent faces,
  // hence don't bother taking it's opacity into consideration.
  // Look at BUG #7301.
  if (this->HexFace->GetProperty() == this->SelectedFaceProperty)
    {
    result |= this->HexFace->HasTranslucentPolygonalGeometry();
    }

  // render the handles
  for (int j=0; j<2; j++)
    {
    result |= this->Handle[j]->HasTranslucentPolygonalGeometry();
    }

  return result;
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::PositionHandles()
{
  double *pts =
     static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);
  double *p0 = pts;
  double *p1 = pts + 3*1;
  // double *p2 = pts + 3*2;
  // double *p3 = pts + 3*3;
  // double *p4 = pts + 3*4;
  // double *p5 = pts + 3*5;
  double *p6 = pts + 3*6;
  double *p7 = pts + 3*7;
  double x[3];

  VTK_AVERAGE(p0,p7,x);
  this->Points->SetPoint(8, x);
  VTK_AVERAGE(p1,p6,x);
  this->Points->SetPoint(9, x);

  int i;
  for (i = 0; i < 2; ++i)
    {
    this->HandleGeometry[i]->SetCenter(this->Points->GetPoint(8+i));
    }

  this->Points->GetData()->Modified();
  this->HexFacePolyData->Modified();
  this->HexPolyData->Modified();
  this->GenerateOutline();
  this->SetPointCorners();
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::HandlesOn()
{
  for (int i=0; i<2; i++)
    {
    this->Handle[i]->VisibilityOn();
    }
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::HandlesOff()
{
  for (int i=0; i<2; i++)
    {
    this->Handle[i]->VisibilityOff();
    }
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::SizeHandles()
{
  double *pts =
      static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);
  double *p0 = pts;
  double *p6 = pts + 3*6;
  double center[3];
  VTK_AVERAGE(p0,p6,center);
  double radius =
      this->vtkWidgetRepresentation::SizeHandlesInPixels(1.5,center);
  for(int i=0; i<2; i++)
    {
    this->HandleGeometry[i]->SetRadius(radius);
    }
}
#undef VTK_ABS
#undef VTK_AVERAGE
#undef VTK_SCALE_VECTOR

//----------------------------------------------------------------------------
int vtkSlabRepresentation::HighlightHandle(vtkProp *prop)
{
  // first unhighlight anything picked
  this->HighlightOutline(0);
  if ( this->CurrentHandle )
    {
    this->CurrentHandle->SetProperty(this->HandleProperty);
    }

  this->CurrentHandle = static_cast<vtkActor *>(prop);

  if ( this->CurrentHandle )
    {
    this->CurrentHandle->SetProperty(this->SelectedHandleProperty);
    for (int i=0; i<6; i++) //find attached face
      {
      if ( this->CurrentHandle == this->Handle[i] )
        {
        return i;
        }
      }
    }
  
  if ( this->CurrentHandle == this->Handle[6] )
    {
    this->HighlightOutline(1);
    return 6;
    }
  
  return -1;
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::HighlightFace(int cellId)
{
  if ( cellId >= 0 )
    {
    vtkIdType npts;
    vtkIdType *pts;
    vtkCellArray *cells = this->HexFacePolyData->GetPolys();
    this->HexPolyData->GetCellPoints(cellId, npts, pts);
    this->HexFacePolyData->Modified();
    cells->ReplaceCell(0,npts,pts);
    this->CurrentHexFace = cellId;
    this->HexFace->SetProperty(this->SelectedFaceProperty);
    if ( !this->CurrentHandle )
      {
      this->CurrentHandle = this->HexFace;
      }
    }
  else
    {
    this->HexFace->SetProperty(this->FaceProperty);
    this->CurrentHexFace = -1;
    }
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::HighlightOutline(int highlight)
{
  if ( highlight )
    {
    this->HexActor->SetProperty(this->SelectedOutlineProperty);
    this->HexOutline->SetProperty(this->SelectedOutlineProperty);
    }
  else
    {
    this->HexActor->SetProperty(this->OutlineProperty);
    this->HexOutline->SetProperty(this->OutlineProperty);
    }
}

//----------------------------------------------------------------------------
void vtkSlabRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  double *bounds=this->InitialBounds;
  os << indent << "Initial Bounds: "
     << "(" << bounds[0] << "," << bounds[1] << ") "
     << "(" << bounds[2] << "," << bounds[3] << ") " 
     << "(" << bounds[4] << "," << bounds[5] << ")\n";

  if ( this->HandleProperty )
    {
    os << indent << "Handle Property: " << this->HandleProperty << "\n";
    }
  else
    {
    os << indent << "Handle Property: (none)\n";
    }
  if ( this->SelectedHandleProperty )
    {
    os << indent << "Selected Handle Property: " 
       << this->SelectedHandleProperty << "\n";
    }
  else
    {
    os << indent << "SelectedHandle Property: (none)\n";
    }

  if ( this->FaceProperty )
    {
    os << indent << "Face Property: " << this->FaceProperty << "\n";
    }
  else
    {
    os << indent << "Face Property: (none)\n";
    }
  if ( this->SelectedFaceProperty )
    {
    os << indent << "Selected Face Property: " 
       << this->SelectedFaceProperty << "\n";
    }
  else
    {
    os << indent << "Selected Face Property: (none)\n";
    }

  if ( this->OutlineProperty )
    {
    os << indent << "Outline Property: " << this->OutlineProperty << "\n";
    }
  else
    {
    os << indent << "Outline Property: (none)\n";
    }
  if ( this->SelectedOutlineProperty )
    {
    os << indent << "Selected Outline Property: " 
       << this->SelectedOutlineProperty << "\n";
    }
  else
    {
    os << indent << "Selected Outline Property: (none)\n";
    }

  os << indent << "Outline Face Wires: "
     << (this->OutlineFaceWires ? "On\n" : "Off\n");
  os << indent << "Outline Cursor Wires: "
     << (this->OutlineCursorWires ? "On\n" : "Off\n");
  os << indent << "Inside Out: " << (this->InsideOut ? "On\n" : "Off\n");
}
