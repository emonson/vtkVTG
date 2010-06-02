#!/usr/bin/env python

# Demonstrate how to use the vtkBoxWidget to translate, scale, and
# rotate actors.  The basic idea is that the box widget controls an
# actor's transform. A callback which modifies the transform is
# invoked as the box widget is manipulated.

import vtk
import sys
sys.path.append("/Users/emonson/Programming/VTK_cvs/vtkVTG/build/bin")
import libvtkvtgWidgetsPython as vtgW

# Create a mace out of filters.
sphere = vtk.vtkSphereSource()
cone = vtk.vtkConeSource()
glyph = vtk.vtkGlyph3D()
glyph.SetInputConnection(sphere.GetOutputPort())
glyph.SetSource(cone.GetOutput())
glyph.SetVectorModeToUseNormal()
glyph.SetScaleModeToScaleByVector()
glyph.SetScaleFactor(0.25)

# The sphere and spikes are appended into a single polydata. This just
# makes things simpler to manage.
apd = vtk.vtkAppendPolyData()
apd.AddInput(glyph.GetOutput())
apd.AddInput(sphere.GetOutput())
at = vtk.vtkTransform()
at.Identity()
at.Translate([4,8,12])
at.Scale([1.0,2.0,3.0])
pdf = vtk.vtkTransformPolyDataFilter()
pdf.SetInputConnection(apd.GetOutputPort())
pdf.SetTransform(at)
maceMapper = vtk.vtkPolyDataMapper()
maceMapper.SetInputConnection(pdf.GetOutputPort())
maceActor = vtk.vtkLODActor()
maceActor.SetMapper(maceMapper)
maceActor.GetProperty().SetOpacity(0.2)
maceActor.VisibilityOn()

# This portion of the code clips the mace with the vtkPlanes implicit
# function.  The clipped region is colored green.
planes = vtk.vtkPlanes()
clipper = vtk.vtkClipPolyData()
clipper.SetInputConnection(pdf.GetOutputPort())
clipper.SetClipFunction(planes)
clipper.InsideOutOn()
selectMapper = vtk.vtkPolyDataMapper()
selectMapper.SetInputConnection(clipper.GetOutputPort())
selectActor = vtk.vtkLODActor()
selectActor.SetMapper(selectMapper)
selectActor.GetProperty().SetColor(0.3, 0.7, 0.3)
selectActor.VisibilityOff()
# selectActor.SetScale(1.01, 1.01, 1.01)

# Create the RenderWindow, Renderer and both Actors
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
istyle = vtk.vtkInteractorStyleTrackballCamera()
iren.SetInteractorStyle(istyle)

# Create the Orientation marker
# axes = vtk.vtkAxesActor()
# wid = vtk.vtkOrientationMarkerWidget()
# wid.SetOrientationMarker(axes)
# wid.SetInteractor(iren)
# wid.On()
# wid.InteractiveOff()

# The box widget observes the events invoked by the render window
# interactor.  These events come from user interaction in the render
# window.
slabWidget = vtgW.vtkSlabWidget2()
slabWidget.SetInteractor(iren)
slabWidget.SetScalingEnabled(0)
slabWidget.SetPriority(1)
slabRep = slabWidget.GetRepresentation()
slabRep.SetPlaceFactor(1.25)

# Add the actors to the renderer, set the background and window size.
ren.AddActor(maceActor)
ren.AddActor(selectActor)
ren.SetBackground(0.1, 0.2, 0.4)
renWin.SetSize(300, 300)

# As the box widget is interacted with, it produces a transformation
# matrix that is set on the actor.
t = vtk.vtkTransform()
def TransformActor(obj, event):
    global t, maceActor
    obj.GetRepresentation().GetTransform(t)
    print obj.GetRepresentation().GetInteractionState()
    maceActor.SetUserTransform(t)

# This callback funciton does the actual work: updates the vtkPlanes
# implicit function.  This in turn causes the pipeline to update.
def SelectPolygons(object, event):
    # object will be the boxWidget
    global selectActor, planes
    object.GetRepresentation().GetPlanes(planes)
    selectActor.VisibilityOn()

def manualOnY(object, event):
    # object will not be the boxWidget
    global slabWidget, slabRep, renWin
    currT = vtk.vtkTransform()
    slabRep.GetTransform(currT)
    currPos = currT.GetPosition()
    maxval = 0
    for coord in currPos:
        if (abs(coord) > abs(maxval)):
            maxval = coord
    currScale = currT.GetScale()
    newT = vtk.vtkTransform()
    newT.Identity()
    if (maxval > 0):
        newT.Translate((0,vtk.vtkMath.Norm(currPos),0))
    else:
        newT.Translate((0,-vtk.vtkMath.Norm(currPos),0))
    newT.RotateZ(90)
    newT.Scale(currScale)
    slabRep.SetTransform(newT)
    slabWidget.Render()
    slabRep.GetPlanes(planes)
    selectActor.VisibilityOn()
    renWin.Render()

def onY(object, event):
    # object will not be the boxWidget
    global slabWidget, slabRep, renWin
    slabRep.RotateToYAxis()
    slabWidget.Render()
    slabRep.GetPlanes(planes)
    selectActor.VisibilityOn()
    renWin.Render()


# Place the interactor initially. The actor is used to place and scale
# the interactor. An observer is added to the box widget to watch for
# interaction events. This event is captured and used to set the
# transformation matrix of the actor.
# slabRep.PlaceWidget(maceActor.GetBounds())
# slabWidget.AddObserver("InteractionEvent", TransformActor)

# Place the interactor initially. The input to a 3D widget is used to
# initially position and scale the widget. The "EndInteractionEvent" is
# observed which invokes the SelectPolygons callback.
slabRep.PlaceWidget(maceActor.GetBounds())
slabWidget.AddObserver("EndInteractionEvent", SelectPolygons)

iren.RemoveObservers("UserEvent")
iren.AddObserver("UserEvent", onY)

iren.Initialize() 
ren.ResetCamera()
renWin.Render()
iren.Start()
