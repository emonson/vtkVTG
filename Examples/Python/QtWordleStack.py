"""
simple_graph_view.py

Example using PyQt4 & VTK Infovis

10 Sept 2009 -- E Monson

"""

from PyQt4 import QtCore, QtGui, QtSvg
from PyQt4.QtGui import QApplication
import vtk
import vtkvtg
import sys
import numpy as N
from vtk.util import numpy_support as VN
import scipy.io

app = QApplication(sys.argv)

WordleView = vtkvtg.vtkQtWordleView()

data_dir = '/Users/emonson/Data/Fodava/EMoGWDataSets/'
data_file = data_dir + 'sciNews_20110216.mat'
	
print 'Trying to load data set from .mat file...'

try:
	MatInput = scipy.io.loadmat(data_file, struct_as_record=True, chars_as_strings=True)
except:
	raise IOError, "Can't load supplied matlab file"

mat_terms = MatInput['terms'].T[0]

WavBases = []	# Wavelet bases
Centers = [] # Center of each node
# NodeWavCoeffs = []
# NodeScalCoeffs = []
for ii in range(MatInput['PointsInNet'].shape[1]):
	WavBases.append(N.mat(MatInput['WavBases'][0,ii]))			# matrix
	Centers.append(N.mat(MatInput['Centers'][0,ii][0]))		# matrix

terms = vtk.vtkStringArray()
terms.SetName('dictionary')
terms.SetNumberOfComponents(1)
for term in mat_terms:
	terms.InsertNextValue(term[0])

basis_idx = 0

coeffs = VN.numpy_to_vtk(WavBases[basis_idx][::-1,0]*100, deep=True)
coeffs.SetName('coefficient')
c_sign = VN.numpy_to_vtk(N.sign(WavBases[basis_idx][::-1,0]), deep=True)
c_sign.SetName('sign')

# Create a table with some points in it...
table = vtk.vtkTable()
table.AddColumn(terms)
table.AddColumn(coeffs)
table.AddColumn(c_sign)

vt = vtk.vtkViewTheme()
lut = vtk.vtkLookupTable()
lut.SetHueRange(0, 0.66)
lut.SetValueRange(0.7, 0.7)
lut.SetSaturationRange(1, 1)
lut.Build()
# Set value for no color by array
vt.SetPointColor(0,0,0)
# Set LUT for color by array
vt.SetPointLookupTable(lut)
# ViewTheme Background color is black by default
vt.SetBackgroundColor(1,1,1)

WordleView.AddRepresentationFromInput(table)
WordleView.SetFieldType(vtkvtg.vtkQtWordleView.ROW_DATA)
WordleView.SetColorByArray(True)
WordleView.SetColorArrayName('sign')
WordleView.SetTermsArrayName('dictionary')
WordleView.SetSizeArrayName('coefficient')
WordleView.ApplyViewTheme(vt)
WordleView.SetMaxNumberOfWords(50);
WordleView.SetFontFamily("Rockwell")
WordleView.SetFontStyle(vtkvtg.vtkQtWordleView.StyleNormal)
WordleView.SetFontWeight(99)
# WordleView.SetOrientation(vtkvtg.vtkQtWordleView.HORIZONTAL)
WordleView.SetOrientation(vtkvtg.vtkQtWordleView.MOSTLY_HORIZONTAL)
# WordleView.SetOrientation(vtkvtg.vtkQtWordleView.HALF_AND_HALF)
# WordleView.SetOrientation(vtkvtg.vtkQtWordleView.MOSTLY_VERTICAL)
# WordleView.SetOrientation(vtkvtg.vtkQtWordleView.VERTICAL)
WordleView.SetLayoutPathShape(vtkvtg.vtkQtWordleView.SQUARE_PATH)

# Do first update before changing to debug mode...
WordleView.Update()
WordleView.ZoomToBounds()

imgAppend = vtk.vtkImageAppend()
imgAppend.SetAppendAxis(2)	# Z

imgList = []

for ii in range(20):
	
	coeffs = VN.numpy_to_vtk(WavBases[ii][::-1,0]*100, deep=True)
	coeffs.SetName('coefficient')
	c_sign = VN.numpy_to_vtk(N.sign(WavBases[ii][::-1,0]), deep=True)
	c_sign.SetName('sign')
	
	table.RemoveColumn(2)
	table.RemoveColumn(1)
	table.AddColumn(coeffs)
	table.AddColumn(c_sign)
	WordleView.RemoveAllRepresentations()
	WordleView.AddRepresentationFromInput(table)
	
	table.Modified()
	# WordleView.Update()
	
	img = vtk.vtkImageData()
	img.DeepCopy(WordleView.GetImageData())
	imgList.append(img)
	imgAppend.AddInput(imgList[ii])

writer = vtk.vtkXMLImageDataWriter()
writer.SetFileName("out.vti")
writer.SetInputConnection(imgAppend.GetOutputPort())
writer.Write()

