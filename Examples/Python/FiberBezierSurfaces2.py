#!/usr/bin/env python

# Demonstrate how to use the vtkBoxWidget to translate, scale, and
# rotate actors.  The basic idea is that the box widget controls an
# actor's transform. A callback which modifies the transform is
# invoked as the box widget is manipulated.

import vtk
import sys
# sys.path.append("/Users/emonson/Programming/VTK_cvs/vtkVTG/build/bin")
# import libvtkvtgFilteringPython as vtgF
# sys.path.append("/Users/emonson/Programming/Xdmf/build/bin")
# import libvtkXdmfPython as vtkX
from numpy import array
import vtk.util.numpy_support as VN

stromaScaleFactor = 0.75
tubesOn = True;
tubeRadius = 0.15
tubeSides = 4
bezierDim = 8		# number of divisions in each direction for bezier surface

reader = vtk.vtkXMLUnstructuredGridReader()
reader.SetFileName('/Users/emonson/Programming/VTK_git/vtkVTG/Examples/Python/X_Output/test1_fibers.vtu')
reader.Update()

# Try (non-merging) clipping with scalar
import vtkvtg
box = vtk.vtkBox()
box.SetBounds(0,320,0,320,140,200)
clip = vtkvtg.vtkMyClipDataSet()
clip.SetInputConnection(reader.GetOutputPort(0))
# clip.SetValue(1.5)
clip.SetClipFunction(box)
clip.InsideOutOn()

# Convert UnstructuredGrid into PolyData for points merge
ugToPoly = vtk.vtkDataSetSurfaceFilter()
ugToPoly.SetInputConnection(clip.GetOutputPort(0))

# Merge points to find fiber bunch centers
cleanPoly = vtk.vtkCleanPolyData()
cleanPoly.SetInputConnection(ugToPoly.GetOutputPort(0))
cleanPoly.ToleranceIsAbsoluteOff()
cleanPoly.SetTolerance(0)
cleanPoly.ConvertLinesToPointsOff()
cleanPoly.ConvertPolysToLinesOff()
cleanPoly.ConvertStripsToPolysOff()
cleanPoly.PointMergingOn()
cleanPoly.Update()

stroma = cleanPoly.GetOutputDataObject(0)
stromaPointData = stroma.GetPointData()
# Not dealing correctly with possibility that the scalars I want are not the active ones...
stromaPointArray = stromaPointData.GetScalars()

# Need non-merging point locator since orgins of fibers are exactly coincident
locator = vtk.vtkNonMergingPointLocator()
locator.SetDataSet(reader.GetOutput(0))
locator.BuildLocator()

# Glyph sources for later work	
line = vtk.vtkLineSource()
line.SetPoint1(0,0,0)
line.SetPoint2(1,0,0)
line.SetResolution(2)

vert = vtk.vtkGlyphSource2D()
vert.SetGlyphTypeToVertex()
vert.SetCenter(1,0,0)

surfaces = vtk.vtkAppendPolyData()
tubes = vtk.vtkAppendPolyData()
	
for pp in range(stroma.GetNumberOfPoints()):
# for pp in range(100):
	print pp
	centerTuple = stroma.GetPoint(pp)
	
	sn = vtk.vtkSelectionNode()
	sn.SetFieldType(1)		# POINTS (vtkSelectionNode enum)
	sn.SetContentType(4)	# INDICES ('')
	s = vtk.vtkSelection()
	idsArray = vtk.vtkIdTypeArray()
	
	pointsList = vtk.vtkIdList()
	locator.FindPointsWithinRadius(0.01, centerTuple, pointsList)
	
	for ii in range(pointsList.GetNumberOfIds()):
		idsArray.InsertNextValue(pointsList.GetId(ii))
	
	sn.SetSelectionList(idsArray)
	s.AddNode(sn)
	
	# Extract just this one group of vertices for one fiber bundle (stromal cell)
	ex = vtk.vtkExtractSelection()
	ex.SetInput(0,reader.GetOutputDataObject(0))
	ex.SetInput(1,s)
	ex.PreserveTopologyOff()
	
	# Glyphing uncovered fibers for lines or tubes display
	lineGlyph = vtk.vtkGlyph3D()
	lineGlyph.SetInputConnection(ex.GetOutputPort(0))
	lineGlyph.SetSource(line.GetOutput())
	lineGlyph.ScalingOn()
	lineGlyph.SetScaleModeToScaleByVector()
	lineGlyph.SetScaleFactor(1.0)
	lineGlyph.OrientOn()
	lineGlyph.SetVectorModeToUseVector()
	lineGlyph.SetColorModeToColorByScalar()
		
	tube = vtk.vtkTubeFilter()
	tube.SetInputConnection(lineGlyph.GetOutputPort())
	tube.SetRadius(tubeRadius)
	tube.SetNumberOfSides(tubeSides)
	tube.CappingOn()
	
	# Add lines (or tubes) to polydata group for later display
	# If want tubes instead of lines, uncomment tube filter above and change this Input
	if tubesOn:
		tubes.AddInput(tube.GetOutput(0))
	else:
		tubes.AddInput(lineGlyph.GetOutput(0))
	
	# Placing verices at the ends of each fiber bunch for finding triangulation
	vertGlyph = vtk.vtkGlyph3D()
	vertGlyph.SetInputConnection(ex.GetOutputPort(0))
	vertGlyph.SetSource(vert.GetOutput())
	vertGlyph.ScalingOn()
	vertGlyph.SetScaleModeToScaleByVector()
	vertGlyph.SetScaleFactor(stromaScaleFactor)
	vertGlyph.OrientOn()
	vertGlyph.SetVectorModeToUseVector()
	
	# Find convex hull (tetrahedra) which covers most end points in fiber bundle
	d3 = vtk.vtkDelaunay3D()
	d3.SetAlpha(0.0)
	d3.SetTolerance(0.001)
	d3.SetOffset(2.5)
	d3.BoundingTriangulationOff()
	d3.SetInputConnection(vertGlyph.GetOutputPort(0))
		
	# Need triangles instead of tets 
	triSurface = vtk.vtkDataSetSurfaceFilter()
	triSurface.SetPieceInvariant(1)
	triSurface.SetInputConnection(d3.GetOutputPort(0))
	triSurface.Update()
		
	# Update needs to be called on the pipeline before this so tris is populated
	#	correctly for the upcoming (cc) loop
	tris = triSurface.GetOutputDataObject(0)
	
	center = array(centerTuple)
	offset = array(centerTuple)
		
	for cc in range(tris.GetNumberOfCells()):
		triPts = VN.vtk_to_numpy(tris.GetCell(cc).GetPoints().GetData())
		# print triPts
		# print center
		
		# Create bezier surface source
		bs = vtkvtg.vtkBezierSurfaceSource()
		bs.SetDimensions(bezierDim,bezierDim)
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
		bs_out = bs.GetOutputDataObject(0)
		bs_numPts = bs_out.GetPoints().GetNumberOfPoints()
		
		# Assuming copying only one scalar for now
		centerValue = stromaPointArray.GetTuple1(pp)
		
		# Create the correct type of array
		pt_array = getattr(vtk,stromaPointArray.GetClassName())()
		pt_array.SetNumberOfComponents(1)
		pt_array.SetName(stromaPointArray.GetName())
		
		# Copy stromal cell value to all points in array
		for tt in range(bs_numPts):
			pt_array.InsertNextTuple1(centerValue)
			
		# Add array to bezier surface polydata
		bs_out.GetPointData().AddArray(pt_array)
		bs_out.GetPointData().SetActiveScalars(pt_array.GetName())
		
		norms = vtk.vtkPolyDataNormals()
		norms.SetFeatureAngle(180)
		norms.SetInput(bs_out)
		
		# Add bezier surface to polydata group for later display
		surfaces.AddInput(norms.GetOutput())


# Create fibers lookup table (LUT)
lut = vtk.vtkLookupTable()
lutNum = 256
lut.SetNumberOfTableValues(lutNum)
ctf = vtk.vtkColorTransferFunction()
ctf.SetColorSpaceToHSV()
ctf.AddRGBPoint(0.0, 0.070588235294117646, 0.090196078431372548, 0.070588235294117646)
ctf.AddRGBPoint(0.5, 0.32549019607843138, 0.41568627450980394, 0.31764705882352939)
ctf.AddRGBPoint(1.0, 0.8784313725490196, 0.94117647058823528, 0.87450980392156863)
for ii,ss in enumerate([float(xx)/float(lutNum) for xx in range(lutNum)]):
	cc = ctf.GetColor(ss)
	lut.SetTableValue(ii,cc[0],cc[1],cc[2],1.0)

# Bezier (stromal cell) surfaces
bsMapper = vtk.vtkPolyDataMapper()
bsMapper.SetInputConnection(surfaces.GetOutputPort(0))
bsMapper.ScalarVisibilityOn()
bsMapper.SetLookupTable(lut)
bsMapper.SetScalarRange((0,2))
bsActor = vtk.vtkLODActor()
bsActor.SetMapper(bsMapper)
# bsActor.GetProperty().SetColor(0.3, 0.7, 0.3)
# bsActor.VisibilityOff()
# bsActor.GetProperty().SetOpacity(1.0)

# Line or tube glyphs for uncovered fibers
glMapper = vtk.vtkPolyDataMapper()
glMapper.SetInputConnection(tubes.GetOutputPort(0))
glMapper.ScalarVisibilityOn()
glMapper.SetLookupTable(lut)
glMapper.SetScalarRange((0,2))
glActor = vtk.vtkLODActor()
glActor.SetMapper(glMapper)
if tubesOn:
	glActor.GetProperty().SetLineWidth(1.0)
# glActor.GetProperty().SetColor(0.3, 0.5, 0.3)
# glActor.GetProperty().SetOpacity(0.5)

# Create the RenderWindow, Renderer and both Actors
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
# renWin.SetAAFrames(16)
# renWin.LineSmoothingOn()
# renWin.PolygonSmoothingOn()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
istyle = vtk.vtkInteractorStyleTrackballCamera()
iren.SetInteractorStyle(istyle)

def writeImage(obj,event):
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
    
    # Save line glyphs in vtk XML PolyData file format (.vtp)
    tube_writer = vtk.vtkXMLPolyDataWriter()
    tube_writer.SetFileName('BFJ_fibers' + str(numList[-1]+1) + '.vtp')
    tube_writer.SetDataModeToAppended()
    tube_writer.SetInputConnection(tubes.GetOutputPort(0))
    tube_writer.Write()
    
    # Save bezier surfaces in vtk XML PolyData file format (.vtp)
    stroma_writer = vtk.vtkXMLPolyDataWriter()
    stroma_writer.SetFileName('BFJ_stroma' + str(numList[-1]+1) + '.vtp')
    stroma_writer.SetDataModeToAppended()
    stroma_writer.SetInputConnection(surfaces.GetOutputPort(0))
    stroma_writer.Write()
    

def keyPress(obj, event):
    key = obj.GetKeySym()
    print key
    # for some reason one time key ended up being w???byek when w was pressed...
    if key.startswith('f'):
        writeImage(obj,event)


# iren.RemoveObservers("KeyPressEvent")
# iren.AddObserver("KeyPressEvent", keyPress)
iren.RemoveObservers("UserEvent")
iren.AddObserver("UserEvent", writeImage)

# Add the actors to the renderer, set the background and window size.
# ren.AddActor(cpActor)
ren.AddActor(bsActor)
ren.AddActor(glActor)
ren.SetBackground(0,0,0)
renWin.SetSize(300, 300)

iren.Initialize() 
ren.ResetCamera()
renWin.Render()
iren.Start()
