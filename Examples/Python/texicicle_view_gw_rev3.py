import vtk
import vtk.util.numpy_support as VN
import numpy as N
import scipy.io
import os
import sys
sys.path.append("/Users/emonson/Programming/VTK_cvs/vtkVTG/build/bin")
import libvtkvtgViewsPython as vtgV

ICEHEIGHT = 0.8
SHRINK = 0.1

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
view.SetShrinkPercentage(SHRINK)
view.SetLayerThickness(ICEHEIGHT)
view.UseGradientColoringOff()
view.Update()

# print view.GetRepresentation(0)

# TESTING
pnmReader = vtk.vtkTIFFReader()
pnmReader.SetFileName("/Users/emonson/Programming/Python/VTK/VSItext2c.tif")
# pnmReader.SetFileName("/Users/emonson/Programming/VTK_cvs/VTKData/Data/beach.tif")
pnmReader.SetOrientationType(4)
# pnmReader.Update()
# END TESTING

tex = vtk.vtkTexture()
# tex.SetInput(WCimageData)
tex.SetInputConnection(pnmReader.GetOutputPort())
# tex.SetLookupTable(lut)

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

istyle = view.GetInteractorStyle()
ren1 = view.GetRenderer()
# Ren has an actor2d which is a scalar bar (edge)
acts = ren1.GetActors()
# Acts has two actors -- graph blocks (0) and labels (1)
act0 = acts.GetItemAsObject(0)
map0 = act0.GetMapper()
texPlane = map0.GetInputConnection(0,0).GetProducer()

picker = vtk.vtkCellPicker()
picker.AddPickList(act0)
picker.SetTolerance(0.01)



def selectionChangedCallback(caller,eventId):
	x,y = istyle.GetInteractor().GetEventPosition()
	z = 0
	ren1.SetDisplayPoint(x,y,0)
	ren1.DisplayToWorld()
	# print (x,y)
	# print ren1.GetWorldPoint()
	
	picker.Pick(x,y,0,ren1)
	print picker.GetCellId()
	
	# List of offsets for creating frustrum corners from point
	o = []			# offsets
	t = 1.0			# amount
	o.append((-1*t,-1*t,-1.0))
	o.append([-1*t,-1*t,+1.0])
	o.append([-1*t,+1*t,-1.0])
	o.append([-1*t,+1*t,+1.0])
	o.append((+1*t,-1*t,-1.0))
	o.append([+1*t,-1*t,+1.0])
	o.append([+1*t,+1*t,-1.0])
	o.append([+1*t,+1*t,+1.0])
	
	frustcorners = vtk.vtkDoubleArray()
	frustcorners.SetNumberOfComponents(4)
	frustcorners.SetNumberOfTuples(8)
	
	for ii in range(8):
		ren1.SetDisplayPoint(x+o[ii][0],y+o[ii][1],z+o[ii][2])
		ren1.DisplayToWorld()
		(m,n,l,w) = ren1.GetWorldPoint()
		frustcorners.SetTuple4(ii,m,n,l,w)
	
	# Enumerator:	(content type)
		# SELECTIONS	
		# GLOBALIDS		
		# PEDIGREEIDS	
		# VALUES	
		# INDICES	
		# FRUSTUM	
		# LOCATIONS		
		# THRESHOLDS	
		# BLOCKS 
	# Enumerator: (field type)
		# CELL	
		# POINT		
		# FIELD		
		# VERTEX	
		# EDGE	
		# ROW	
		
	sel = vtk.vtkSelection()
	node = vtk.vtkSelectionNode()
	node.SetContentType(5)		# FRUSTRUM
	node.SetFieldType(0)		# CELL
	node.SetSelectionList(frustcorners)
	sel.AddNode(node);
	sel.Update()
	print 'Node prop:', node.GetSelectedProp()
	
	cs = vtk.vtkConvertSelection()
	outSel = cs.ToIndexSelection(sel, texPlane.GetOutput())	# INDICES
	outNode = outSel.GetNode(0)
	print 'Number of Tuples in outSelNode0: ', outNode.GetSelectionList().GetNumberOfTuples()
	print 'Tuple0: ', outNode.GetSelectionList().GetValue(0)
	
	cs2 = vtk.vtkConvertSelection()
	outSel2 = cs.ToPedigreeIdSelection(sel, texPlane.GetOutput())	# INDICES
	outNode2 = outSel.GetNode(0)
	print 'Number of Tuples in outSelNode0: ', outNode2.GetSelectionList().GetNumberOfTuples()
	print 'Tuple0: ', outNode2.GetSelectionList().GetValue(0)
	print 'Prop: ', outNode2.GetProperties().Get(outNode2.PROP())
	print 'Prop: ', outNode2.GetSelectedProp(), '\n'


istyle.AddObserver('SelectionChangedEvent', selectionChangedCallback)


view.ResetCamera()
view.Render()
view.GetInteractor().Start()

