import vtk
import vtk.util.numpy_support as VN
import numpy as N
import scipy.io
import os
import sys
sys.path.append("/Users/emonson/Programming/VTK_cvs/vtkVTG/build/bin")
import libvtkvtgViewsPython as vtgV

ICEHEIGHT = 0.25

# TODO: Some sort of problem with duplicate ids... In 1k_20100324 have two 111s, and that's
# the index at which selection problems start...

# ----------
# Load and construct whole graph and multi-resolution data from Matlab structure
dataDir = '/Users/emonson/Data/Fodava/EMoGWDataSets/'
filename = dataDir + 'mnist1_5c_20100324.mat'
# filename = dataDir + 'mnist1_5k_20100320.mat'

print 'Loading data set from .mat file...'
MatInput = scipy.io.loadmat(filename,struct_as_record=True)

# Get variables out of Matlab structure
print 'Transferring variables from Matlab structures'
gW = MatInput['gW']
Data = MatInput['Data']
X = MatInput['X']
# WC = MatInput['WavCoeffs']
# Instead of using WC, which has already been ordered within Matlab
WavCoeffsOrig = Data['MatWavCoeffs'][0,0].T

# Structures are accessed like this:
PROJ = Data['Projections'][0,0]
CP = gW['cp'][0,0][0]	# CP ends up as 'uint16'... Not sure why 16-bit...
# Making a list of numpy arrays because cell array structure is a pain...
PIN = []	# Points In Net
NIN = []	# Number In Net
vertex_id = vtk.vtkIdTypeArray()
vertex_id.SetName('vertex_ids')
for ii in range(gW['PointsInNet'][0,0].shape[1]):
	PIN.append(gW['PointsInNet'][0,0][0,ii][0]-1)	# 0-based indices
	NIN.append(gW['PointsInNet'][0,0][0,ii][0].size)
	vertex_id.InsertNextValue(ii)
	
NINarray = N.array(NIN)
NINvtk = VN.numpy_to_vtk(NINarray)
NINvtk.SetName('num_in_vertex')
SCALESvtk = VN.numpy_to_vtk(gW['Scales'][0,0][0])
SCALESvtk.SetName('scale')

# Build tree out of CP list of "is a child of"
#   remembering that Matlab indices are 1-based and numpy/VTK 0-based
print 'Building graph'
dg = vtk.vtkMutableDirectedGraph()
edge_id = vtk.vtkIdTypeArray()
edge_id.SetName('edge_ids')
for ii in range(CP.size):
	dg.AddVertex()
for ii in range(CP.size):
	tmp = CP[ii]-1		# 0-based indices
	if tmp > 0:
		dg.AddGraphEdge(tmp,ii)		# Method for use with wrappers -- AddEdge() in C++
		edge_id.InsertNextValue(ii)

dg.GetVertexData().AddArray(NINvtk)
dg.GetVertexData().AddArray(SCALESvtk)
dg.GetVertexData().AddArray(vertex_id)
dg.GetVertexData().SetActiveScalars('scale')
dg.GetVertexData().SetActivePedigreeIds('vertex_ids')
dg.GetEdgeData().AddArray(edge_id)
dg.GetEdgeData().SetActivePedigreeIds('edge_ids')

tree = vtk.vtkTree()
tree.CheckedShallowCopy(dg)

view = vtgV.vtkTexturedIcicleView()
view.SetTreeFromInput(tree)
# view.SetRepresentationFromInputConnection(aa1.GetOutputPort(0))
view.SetAreaSizeArrayName("num_in_vertex")
view.SetAreaColorArrayName("level")
view.SetAreaLabelArrayName("vertex_ids")
view.SetLabelPriorityArrayName("scale")
view.SetAreaLabelVisibility(True)
view.SetAreaHoverArrayName("vertex_ids")
view.SetDisplayHoverText(True)
view.SetShrinkPercentage(0.04)
view.SetLayerThickness(ICEHEIGHT)
view.UseGradientColoringOff()
view.Update()

tree_rep = view.GetRepresentation(0)
tree_to_poly = tree_rep.GetAreaToPolyData()
tree_to_poly.Update()

# Need to find proper ordering of wavelet coeffs based on icicle layout
# tree.GetVertexData().GetArray('area') is 4-component (Xmin,Xmax,Ymin,Ymax)
print 'Reordering wavelet coeffs'
out_polys = tree_to_poly.GetOutputDataObject(0)
isleaf = VN.vtk_to_numpy(out_polys.GetCellData().GetArray('leaf'))
poly_bounds = VN.vtk_to_numpy(out_polys.GetCellData().GetArray('area'))
vertex_ids = VN.vtk_to_numpy(out_polys.GetCellData().GetArray('vertex_ids'))

LeafIds = vertex_ids[isleaf>0]
LeafXmins = poly_bounds[isleaf>0,0]
XOrderedLeafIds = LeafIds[LeafXmins.argsort()]


# Create an array holding the indices of the leaf vertices in the proper order
SortedLeafIdxArray = N.array([],dtype='uint16')
for ii in range(XOrderedLeafIds.size):
	SortedLeafIdxArray = N.concatenate((SortedLeafIdxArray,PIN[XOrderedLeafIds[ii]]))
	
# And then reorder the Wavelet Coefficients matrix according to this
WCsorted = WavCoeffsOrig[:,SortedLeafIdxArray]

# Create vtkImageData out of WavCoeffs for texturing icicle view tree
# WCvtk = VN.numpy_to_vtk(WC.ravel()[::-1].copy())
WCvtk = vtk.vtkDoubleArray()
WCvtk.SetNumberOfComponents(1)
WCvtk.SetNumberOfTuples(WCsorted.size)
ind = 0
WCtmp = WCsorted[::-1,:].copy()
for ii in range(WCtmp.shape[0]):
	for jj in range(WCtmp.shape[1]):
		# WCvtk.SetTuple1(ind, N.abs(WCtmp[ii,jj]))	# NOTE: somehow problems when values negative
		zz = 1000.0*N.random.randn()
		WCvtk.SetTuple1(ind, zz)
		ind += 1
WCvtk.SetName('Coeffs')

WCimageData = vtk.vtkImageData()
WCimageData.SetOrigin(0,0,0)
WCimageData.SetSpacing(1,1,1)
WCimageData.SetDimensions(WavCoeffsOrig.shape[1],WavCoeffsOrig.shape[0],1)
# WCimageData.GetPointData().AddArray(WCvtk)
# WCimageData.GetPointData().SetActiveScalars('Coeffs')

WCext = abs(WavCoeffsOrig.min()) if (abs(WavCoeffsOrig.min()) > abs(WavCoeffsOrig.max())) else abs(WavCoeffsOrig.max())
print WCext

# Create blue to white to red lookup table
lut = vtk.vtkLookupTable()
lutNum = 256
lut.SetNumberOfTableValues(lutNum)
lut.Build()
ctf = vtk.vtkColorTransferFunction()
ctf.SetColorSpaceToDiverging()
ctf.AddRGBPoint(0.0, 0, 0, 1.0)
ctf.AddRGBPoint(1.0, 1.0, 0, 0)
for ii,ss in enumerate([float(xx)/float(lutNum) for xx in range(lutNum)]):
	cc = ctf.GetColor(ss)
	lut.SetTableValue(ii,cc[0],cc[1],cc[2],1.0)
lut.SetRange(-WCext,WCext)

# Try mapping scalars through lut before assigning to imagedata
outRGBA = lut.MapScalars(WCvtk,0,0)
outRGB = vtk.vtkUnsignedCharArray()
outRGB.SetNumberOfComponents(3)
outRGB.SetNumberOfTuples(outRGBA.GetNumberOfTuples())
outRGB.SetName('Map Colors')
for ii in range(outRGBA.GetNumberOfTuples()):
	uc = outRGBA.GetTuple4(ii)
	outRGB.SetTuple3(ii,uc[0],uc[1],uc[2])
	
WCimageData.GetPointData().AddArray(outRGB)
WCimageData.GetPointData().SetActiveScalars('Map Colors')
WCimageData.GetIncrements()		# calls ComputeIncrements()

# TESTING
pnmReader = vtk.vtkTIFFReader()
# pnmReader.SetFileName("/Users/emonson/Programming/Python/VTK/VSItext2c.tif")
pnmReader.SetFileName("/Users/emonson/Programming/VTK_cvs/VTKData/Data/beach.tif")
pnmReader.SetOrientationType(4)
pnmReader.Update()
# END TESTING

tex = vtk.vtkTexture()
# tex.SetInput(WCimageData)
tex.SetInput(pnmReader.GetOutputDataObject(0))
# tex.SetLookupTable(lut)
print tex

# Apply texture to polydata in vtkTexturedTreeAreaRepresentation through View
view.SetTexture(tex)

# Apply a theme to the views
theme = vtk.vtkViewTheme.CreateMellowTheme()
theme.SetPointHueRange(0,0)
theme.SetPointSaturationRange(0,0)
theme.SetPointValueRange(0.75,0.75)
theme.SetPointAlphaRange(1,1)
view.ApplyViewTheme(theme)
theme.FastDelete()

# TEST second layout
# Create the RenderWindow, Renderer and both Actors
# ren1 = vtk.vtkRenderer()
# renWin = vtk.vtkRenderWindow()
# renWin.AddRenderer(ren1)
# iren = vtk.vtkRenderWindowInteractor()
# iren.SetRenderWindow(renWin)
# istyle = vtk.vtkInteractorStyleRubberBandZoom()
# iren.SetInteractorStyle(istyle)
# mapper = vtk.vtkPolyDataMapper()
# mapper.SetInputConnection(texPlane.GetOutputPort(0))
# mapper.SetScalarVisibility(0)
# actor = vtk.vtkActor()
# actor.SetMapper(mapper)
# actor.SetTexture(tex)
# ren1.AddActor(actor)
# renWin.SetSize(400, 400)
# iren.Initialize() 
# ren1.ResetCamera()
# renWin.Render()
# END TEST second layout

view.ResetCamera()
view.Render()
view.GetInteractor().Start()

