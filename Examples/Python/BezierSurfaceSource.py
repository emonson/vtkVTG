#!/usr/bin/env python

# Demonstrate how to use the vtkBoxWidget to translate, scale, and
# rotate actors.  The basic idea is that the box widget controls an
# actor's transform. A callback which modifies the transform is
# invoked as the box widget is manipulated.

import vtk
import sys
sys.path.append("/Users/emonson/Programming/VTK_cvs/vtkVTG/build/bin")
import libvtkvtgFilteringPython as vtgF
from numpy import array

def tup_add(t1,t2):
	return tuple([ii+jj for ii,jj in zip(t1,t2)])
	
def tup_mult(cc,tt):
	return tuple([cc*ii for ii in tt])
	
def tup_div(tt,cc):
	return tuple([ii/cc for ii in tt])

# Create a mace out of filters.
bs = vtgF.vtkBezierSurfaceSource()
bs.SetDimensions(8,8)
bs.SetNumberOfControlPoints(4,4)

# Set control point positions for triangular cubic bezier patch
b003 = array((0.7,0.2,0))	# X
b102 = array((0,0,0))	# Xx Y
b201 = array((0,0,0))	# X yY
b300 = array((0,1,0))	# Y
b210 = array((0,0,0))	# Yy Z
b120 = array((0,0,0))	# Y zZ
b030 = array((0,-0.2,1.2))	# Z
b021 = array((0,0,0))	# Zz X
b012 = array((0,0,0))	# Z xX
b111 = array((0,0,0))	# center

# Conversion from Graphics Gems III, edited by David Kirk
# pp 260-261, V.11 Converting Bezier Triangles into Rectangular Patches, Dani Lischinski
p00 = b003
p01 = b102
p02 = b201
p03 = b300
p10 = b012
p11 = (b012 + 2.0*b111)/3.0
p12 = (2.0*b111 + b210)/3.0
p13 = b210
p20 = b021
p21 = (2.0*b021 + b120)/3.0
p22 = (b021 + 3.0*b120)/3.0
p23 = b120
p30 = b030
p31 = b030
p32 = b030
p33 = b030

for ii in range(4):
	for jj in range(4):
		bs.SetControlPoint(ii,jj,vars()['p'+str(ii)+str(jj)])

cpMapper = vtk.vtkPolyDataMapper()
cpMapper.SetInputConnection(bs.GetOutputPort(1))
cpActor = vtk.vtkLODActor()
cpActor.SetMapper(cpMapper)
cpActor.GetProperty().SetLineWidth(2.0)
cpActor.GetProperty().SetColor(0.5,0.4,0.4)

bsMapper = vtk.vtkPolyDataMapper()
bsMapper.SetInputConnection(bs.GetOutputPort(0))
bsActor = vtk.vtkLODActor()
bsActor.SetMapper(bsMapper)
bsActor.GetProperty().SetColor(0.3, 0.7, 0.3)
# bsActor.VisibilityOff()
bsActor.GetProperty().SetOpacity(0.8)

# Create the RenderWindow, Renderer and both Actors
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
istyle = vtk.vtkInteractorStyleTrackballCamera()
iren.SetInteractorStyle(istyle)


# Add the actors to the renderer, set the background and window size.
ren.AddActor(bsActor)
ren.AddActor(cpActor)
ren.SetBackground(0.1, 0.2, 0.4)
renWin.SetSize(300, 300)

iren.Initialize() 
ren.ResetCamera()
renWin.Render()
iren.Start()
