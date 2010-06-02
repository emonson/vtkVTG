/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSlabRepresentation.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSlabRepresentation - a class defining the representation for the vtkSlabWidget2
// .SECTION Description
// This class is a concrete representation for the vtkSlabWidget2. It
// represents a box with seven handles: one on each of the six faces, plus a
// center handle. Through interaction with the widget, the box
// representation can be arbitrarily positioned in the 3D space.
//
// To use this representation, you normally use the PlaceWidget() method
// to position the widget at a specified region in space.
//
// .SECTION Caveats
// This class, and vtkSlabWidget2, are second generation VTK
// widgets. An earlier version of this functionality was defined in the
// class vtkSlabWidget.

// .SECTION See Also
// vtkSlabWidget2 vtkSlabWidget


#ifndef __vtkSlabRepresentation_h
#define __vtkSlabRepresentation_h

#include "vtkWidgetRepresentation.h"

class vtkActor;
class vtkPolyDataMapper;
class vtkLineSource;
class vtkSphereSource;
class vtkCellPicker;
class vtkProperty;
class vtkPolyData;
class vtkPoints;
class vtkPolyDataAlgorithm;
class vtkPointHandleRepresentation3D;
class vtkTransform;
class vtkPlanes;
class vtkSlab;
class vtkDoubleArray;
class vtkMatrix4x4;


class VTK_WIDGETS_EXPORT vtkSlabRepresentation : public vtkWidgetRepresentation
{
public:
  // Description:
  // Instantiate the class.
  static vtkSlabRepresentation *New();

  // Description:
  // Standard methods for the class.
  vtkTypeRevisionMacro(vtkSlabRepresentation,vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the planes describing the implicit function defined by the box
  // widget. The user must provide the instance of the class vtkPlanes. Note
  // that vtkPlanes is a subclass of vtkImplicitFunction, meaning that it can
  // be used by a variety of filters to perform clipping, cutting, and
  // selection of data.  (The direction of the normals of the planes can be
  // reversed enabling the InsideOut flag.)
  void GetPlanes(vtkPlanes *planes);

  // Description:
  // Set/Get the InsideOut flag. This data memeber is used in conjunction
  // with the GetPlanes() method. When off, the normals point out of the
  // box. When on, the normals point into the hexahedron.  InsideOut is off
  // by default.
  vtkSetMacro(InsideOut,int);
  vtkGetMacro(InsideOut,int);
  vtkBooleanMacro(InsideOut,int);

  // Description:
  // Retrieve a linear transform characterizing the transformation of the
  // box. Note that the transformation is relative to where PlaceWidget()
  // was initially called. This method modifies the transform provided. The
  // transform can be used to control the position of vtkProp3D's, as well as
  // other transformation operations (e.g., vtkTranformPolyData).
  virtual void GetTransform(vtkTransform *t);

  // Description:
  // Set the position, scale and orientation of the box widget using the
  // transform specified. Note that the transformation is relative to 
  // where PlaceWidget() was initially called (i.e., the original bounding
  // box). 
  virtual void SetTransform(vtkTransform* t);

  void RotateToXAxis(int test);
  void RotateToYAxis(int test);
  void RotateToZAxis(int test);
  vtkGetVectorMacro(PointCorners,double,9);
  vtkGetVectorMacro(InitialBounds,double,6);
  vtkGetVectorMacro(XFaceNormals,double,9);
  void SetPointCorners();
//   double *GetPointsArray();
//   void GetPointsArray(double pointCorners[9]) const;
//   void GetPointsArray(double &p0x, double &p0y, double &p0z,
//                       double &p1x, double &p1y, double &p1z,
//                       double &p6x, double &p6y, double &p6z) const;

  // Description:
  // Grab the polydata (including points) that define the box widget. The
  // polydata consists of 6 quadrilateral faces and 15 points. The first
  // eight points define the eight corner vertices; the next six define the
  // -x,+x, -y,+y, -z,+z face points; and the final point (the 15th out of 15
  // points) defines the center of the box. These point values are guaranteed
  // to be up-to-date when either the widget's corresponding InteractionEvent
  // or EndInteractionEvent events are invoked. The user provides the
  // vtkPolyData and the points and cells are added to it.
  void GetPolyData(vtkPolyData *pd);

  // Description:
  // Get the handle properties (the little balls are the handles). The 
  // properties of the handles, when selected or normal, can be 
  // specified.
  vtkGetObjectMacro(HandleProperty,vtkProperty);
  vtkGetObjectMacro(SelectedHandleProperty,vtkProperty);

  // Description:
  // Get the face properties (the faces of the box). The 
  // properties of the face when selected and normal can be 
  // set.
  vtkGetObjectMacro(FaceProperty,vtkProperty);
  vtkGetObjectMacro(SelectedFaceProperty,vtkProperty);
  
  // Description:
  // Get the outline properties (the outline of the box). The 
  // properties of the outline when selected and normal can be 
  // set.
  vtkGetObjectMacro(OutlineProperty,vtkProperty);
  vtkGetObjectMacro(SelectedOutlineProperty,vtkProperty);
  
  // Description:
  // Control the representation of the outline. This flag enables
  // face wires. By default face wires are off.
  void SetOutlineFaceWires(int);
  vtkGetMacro(OutlineFaceWires,int);
  void OutlineFaceWiresOn() {this->SetOutlineFaceWires(1);}
  void OutlineFaceWiresOff() {this->SetOutlineFaceWires(0);}

  // Description:
  // Control the representation of the outline. This flag enables
  // the cursor lines running between the handles. By default cursor
  // wires are on.
  void SetOutlineCursorWires(int);
  vtkGetMacro(OutlineCursorWires,int);
  void OutlineCursorWiresOn() {this->SetOutlineCursorWires(1);}
  void OutlineCursorWiresOff() {this->SetOutlineCursorWires(0);}

  // Description:
  // Switches handles (the spheres) on or off by manipulating the underlying
  // actor visibility.
  void HandlesOn();
  void HandlesOff();
  
  // Description:
  // These are methods that satisfy vtkWidgetRepresentation's API.
  virtual void PlaceWidget(double bounds[6]);
  virtual void BuildRepresentation();
  virtual int ComputeInteractionState(int X, int Y, int modify=0);
  virtual void StartWidgetInteraction(double e[2]);
  virtual void WidgetInteraction(double e[2]);
  virtual double *GetBounds();
  
  // Description:
  // Methods supporting, and required by, the rendering process.
  virtual void ReleaseGraphicsResources(vtkWindow*);
  virtual int RenderOpaqueGeometry(vtkViewport*);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport*);
  virtual int HasTranslucentPolygonalGeometry();
  
//BTX - used to manage the state of the widget (code relies on Scaling being last)
  enum {Outside=0,MoveF0,MoveF1,MoveF2,MoveF3,MoveF4,MoveF5,TransX,TransY,TransZ,RotAbtX,RotAbtY,RotAbtZ,Translating,Rotating,Scaling};
//ETX

  // Description:
  // The interaction state may be set from a widget (e.g., vtkSlabWidget2) or
  // other object. This controls how the interaction with the widget
  // proceeds. Normally this method is used as part of a handshaking
  // process with the widget: First ComputeInteractionState() is invoked that
  // returns a state based on geometric considerations (i.e., cursor near a
  // widget feature), then based on events, the widget may modify this
  // further.
  void SetInteractionState(int state);

protected:
  vtkSlabRepresentation();
  ~vtkSlabRepresentation();

  // Manage how the representation appears
  double LastEventPosition[3];
  
  // the hexahedron (6 faces)
  vtkActor          *HexActor;
  vtkPolyDataMapper *HexMapper;
  vtkPolyData       *HexPolyData;
  vtkPoints         *Points;  //used by others as well
  double			PointCorners[9];
  double			XFaceNormals[9];
  double             N[6][3]; //the normals of the faces

  // A face of the hexahedron
  vtkActor          *HexFace;
  vtkPolyDataMapper *HexFaceMapper;
  vtkPolyData       *HexFacePolyData;

  // glyphs representing hot spots (e.g., handles)
  vtkActor          **Handle;
  vtkPolyDataMapper **HandleMapper;
  vtkSphereSource   **HandleGeometry;
  virtual void PositionHandles();
  int HighlightHandle(vtkProp *prop); //returns cell id
  void HighlightFace(int cellId);
  void HighlightOutline(int highlight);
  void ComputeNormals();
  virtual void SizeHandles();
  
  // wireframe outline
  vtkActor          *HexOutline;
  vtkPolyDataMapper *OutlineMapper;
  vtkPolyData       *OutlinePolyData;

  // Do the picking
  vtkCellPicker *HandlePicker;
  vtkCellPicker *HexPicker;
  vtkActor *CurrentHandle;
  int      CurrentHexFace;
  vtkCellPicker *LastPicker;
  
  // Transform the hexahedral points (used for rotations)
  vtkTransform *Transform;
  
  // Support GetBounds() method
  vtkSlab *BoundingBox;
  
  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty *HandleProperty;
  vtkProperty *SelectedHandleProperty;
  vtkProperty *FaceProperty;
  vtkProperty *SelectedFaceProperty;
  vtkProperty *OutlineProperty;
  vtkProperty *SelectedOutlineProperty;
  void CreateDefaultProperties();
  
  // Control the orientation of the normals
  int InsideOut;
  int OutlineFaceWires;
  int OutlineCursorWires;
  void GenerateOutline();
  
  // Helper methods
  virtual void Translate(double *p1, double *p2);
  virtual void Scale(double *p1, double *p2, int X, int Y);
  virtual void Rotate(int X, int Y, double *p1, double *p2, double *vpn);
  virtual void RotateAboutX(int X, int Y, double *p1, double *p2, double *vpn);
  virtual void RotateAboutY(int X, int Y, double *p1, double *p2, double *vpn);
  virtual void RotateAboutZ(int X, int Y, double *p1, double *p2, double *vpn);
  void MovePlusXFace(double *p1, double *p2);
  void MoveMinusXFace(double *p1, double *p2);
  void MovePlusYFace(double *p1, double *p2);
  void MoveMinusYFace(double *p1, double *p2);
  void MovePlusZFace(double *p1, double *p2);
  void MoveMinusZFace(double *p1, double *p2);
  void TranslateX(double *p1, double *p2);
  void TranslateY(double *p1, double *p2);
  void TranslateZ(double *p1, double *p2);

  // Internal ivars for performance
  vtkPoints      *PlanePoints;
  vtkDoubleArray *PlaneNormals;
  vtkMatrix4x4   *Matrix;

  //"dir" is the direction in which the face can be moved i.e. the axis passing
  //through the center
  void MoveFace(double *p1, double *p2, double *dir, 
                double *x1, double *x2, double *x3, double *x4,
                double *x5);
  //Helper method to obtain the direction in which the face is to be moved.
  //Handles special cases where some of the scale factors are 0.
  void GetDirection(const double Nx[3],const double Ny[3], 
                    const double Nz[3], double dir[3]);


private:
  vtkSlabRepresentation(const vtkSlabRepresentation&);  //Not implemented
  void operator=(const vtkSlabRepresentation&);  //Not implemented
};

#endif
