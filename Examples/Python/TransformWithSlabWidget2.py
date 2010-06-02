#!/usr/bin/env python

# Demonstrate how to use the vtkBoxWidget to translate, scale, and
# rotate actors.  The basic idea is that the box widget controls an
# actor's transform. A callback which modifies the transform is
# invoked as the box widget is manipulated.

import vtk
import sys
sys.path.append("/Users/emonson/Programming/VTK_cvs/vtkVTG/build/bin")
import libvtkvtgWidgetsPython as vtgW

# Start by creating some simple geometry; in this case a mace.
sphere = vtk.vtkSphereSource()
cone = vtk.vtkConeSource()
glyph = vtk.vtkGlyph3D()
glyph.SetInputConnection(sphere.GetOutputPort())
glyph.SetSource(cone.GetOutput())
glyph.SetVectorModeToUseNormal()
glyph.SetScaleModeToScaleByVector()
glyph.SetScaleFactor(0.25)
appendData = vtk.vtkAppendPolyData()
appendData.AddInput(glyph.GetOutput())
appendData.AddInput(sphere.GetOutput())
maceMapper = vtk.vtkPolyDataMapper()
maceMapper.SetInputConnection(appendData.GetOutputPort())
maceActor = vtk.vtkLODActor()
maceActor.SetMapper(maceMapper)
maceActor.VisibilityOn()

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

# Place the interactor initially. The actor is used to place and scale
# the interactor. An observer is added to the box widget to watch for
# interaction events. This event is captured and used to set the
# transformation matrix of the actor.
slabRep.PlaceWidget(maceActor.GetBounds())
slabWidget.AddObserver("InteractionEvent", TransformActor)

iren.Initialize() 
ren.ResetCamera()
renWin.Render()
iren.Start()
