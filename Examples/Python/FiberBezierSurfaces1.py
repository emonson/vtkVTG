#!/usr/bin/env python

# Demonstrate how to use the vtkBoxWidget to translate, scale, and
# rotate actors.  The basic idea is that the box widget controls an
# actor's transform. A callback which modifies the transform is
# invoked as the box widget is manipulated.

import vtk
import sys
sys.path.append("/Users/emonson/Programming/VTK_cvs/vtkVTG/build/bin")
import libvtkvtgFilteringPython as vtgF
# sys.path.append("/Users/emonson/Programming/Xdmf/build/bin")
# import libvtkXdmfPython as vtkX
from numpy import array
import vtk.util.numpy_support as VN

reader = vtk.vtkUnstructuredGridReader()
reader.SetFileName('/Users/emonson/Programming/VTK_cvs/vtkVTG/Examples/Python/SingleFiberPoints1.vtk')
reader.Update()

ug = reader.GetOutputDataObject(0)
points = ug.GetPoints()
ptArray = VN.vtk_to_numpy(points.GetData())
pdata = ug.GetPointData()
dir = pdata.GetArray('fibers_fibers_direction')
dirArray = VN.vtk_to_numpy(dir)

line = vtk.vtkLineSource()
line.SetPoint1(0,0,0)
line.SetPoint2(1,0,0)
line.SetResolution(2)

vert = vtk.vtkGlyphSource2D()
vert.SetGlyphTypeToVertex()
vert.SetCenter(1,0,0)

lineGlyph = vtk.vtkGlyph3D()
lineGlyph.SetInputConnection(reader.GetOutputPort(0))
lineGlyph.SetSource(line.GetOutput())
lineGlyph.ScalingOn()
lineGlyph.SetScaleModeToScaleByVector()
lineGlyph.SetScaleFactor(1.0)
lineGlyph.OrientOn()
lineGlyph.SetVectorModeToUseVector()

tube = vtk.vtkTubeFilter()
tube.SetInputConnection(lineGlyph.GetOutputPort())
tube.SetRadius(0.05)
tube.SetNumberOfSides(8)
tube.CappingOn()

vertGlyph = vtk.vtkGlyph3D()
vertGlyph.SetInputConnection(reader.GetOutputPort(0))
vertGlyph.SetSource(line.GetOutput())
vertGlyph.ScalingOn()
vertGlyph.SetScaleModeToScaleByVector()
vertGlyph.SetScaleFactor(0.6)
vertGlyph.OrientOn()
vertGlyph.SetVectorModeToUseVector()

d3 = vtk.vtkDelaunay3D()
d3.SetAlpha(0.0)
d3.SetTolerance(0.001)
d3.SetOffset(50.0)
d3.BoundingTriangulationOff()
d3.SetInputConnection(vertGlyph.GetOutputPort(0))

ex = vtk.vtkDataSetSurfaceFilter()
ex.SetPieceInvariant(1)
ex.SetInputConnection(d3.GetOutputPort(0))
ex.Update()

tris = ex.GetOutputDataObject(0)

offset = ptArray[0]
center = ptArray[0]

surfaces = vtk.vtkAppendPolyData()
controls = vtk.vtkAppendPolyData()

for cc in range(tris.GetNumberOfCells()):
	triPts = VN.vtk_to_numpy(tris.GetCell(cc).GetPoints().GetData())
	# print triPts
	# print center
	
	# Create bezier surface source
	bs = vtgF.vtkBezierSurfaceSource()
	bs.SetDimensions(20,20)
	bs.SetNumberOfControlPoints(4,4)
	
	# Set control point positions for triangular cubic bezier patch
	# Subtract off center point for calculation (something in conversion...)
	b003 = triPts[0]-offset	# X
	b102 = center-offset	# Xx Y
	b201 = center-offset	# X yY
	b300 = triPts[1]-offset	# Y
	b210 = center-offset	# Yy Z
	b120 = center-offset	# Y zZ
	b030 = triPts[2]-offset	# Z
	b021 = center-offset	# Zz X
	b012 = center-offset	# Z xX
	b111 = center-offset + 0.5*(triPts.mean(0)-offset)	# center
	
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
	
	# Add back in offset before setting control point coordinates
	for ii in range(4):
		for jj in range(4):
			bs.SetControlPoint(ii,jj,vars()['p'+str(ii)+str(jj)]+offset)
			# print 'p'+str(ii)+str(jj), vars()['p'+str(ii)+str(jj)]+offset
			
	bs.Update()
	surfaces.AddInput(bs.GetOutput(0))
	controls.AddInput(bs.GetOutput(1))

cpMapper = vtk.vtkPolyDataMapper()
cpMapper.SetInputConnection(controls.GetOutputPort(0))
cpActor = vtk.vtkActor()
cpActor.SetMapper(cpMapper)
cpActor.GetProperty().SetLineWidth(1.0)
cpActor.GetProperty().SetColor(0.5,0.4,0.4)

bsMapper = vtk.vtkPolyDataMapper()
bsMapper.SetInputConnection(surfaces.GetOutputPort(0))
bsActor = vtk.vtkActor()
bsActor.SetMapper(bsMapper)
bsActor.GetProperty().SetColor(0.3, 0.7, 0.3)
# bsActor.VisibilityOff()
bsActor.GetProperty().SetOpacity(1.0)



glMapper = vtk.vtkPolyDataMapper()
glMapper.SetInputConnection(tube.GetOutputPort(0))
glMapper.ScalarVisibilityOff()
glActor = vtk.vtkActor()
glActor.SetMapper(glMapper)
glActor.GetProperty().SetColor(0.3, 0.7, 0.3)
# glActor.GetProperty().SetOpacity(0.5)

# Create the RenderWindow, Renderer and both Actors
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
istyle = vtk.vtkInteractorStyleTrackballCamera()
iren.SetInteractorStyle(istyle)

def writeImage():
    # Look for any file names matching the pattern and auto-increment ending number
    import glob, re
    existingFiles = glob.glob('BezierFiberJunctions*.png')
    
    if (len(existingFiles) == 0):
    	outName = 'BezierFiberJunctions1.png'
    else:
		pat = re.compile('BezierFiberJunctions(\d+).png')
		numList = []
		for name in existingFiles:
			match = re.search(pat,name)
			if match:
				numList.append(int(match.group(1)))
		numList.sort()
		outName = 'BezierFiberJunctions' + str(numList[-1]+1) + '.png'
			
    wif = vtk.vtkWindowToImageFilter()
    wif.SetInput(renWin)
    wif.Modified()
    
    png = vtk.vtkPNGWriter()
    png.SetInputConnection(wif.GetOutputPort(0))
    png.SetFileName(outName)
    png.Write()
    

def keyPress(obj, event):
    key = obj.GetKeySym()
    print key
    # for some reason one time key ended up being w???byek when w was pressed...
    if key.startswith('f'):
        writeImage()


iren.RemoveObservers("KeyPressEvent")
iren.AddObserver("KeyPressEvent", keyPress)

# Add the actors to the renderer, set the background and window size.
# ren.AddActor(cpActor)
ren.AddActor(bsActor)
ren.AddActor(glActor)
ren.SetBackground(1,1,1)
renWin.SetSize(300, 300)

iren.Initialize() 
ren.ResetCamera()
renWin.Render()
iren.Start()
