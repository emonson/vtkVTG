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

from QtSimpleView import Ui_MainWindow

class SimpleView(QtGui.QMainWindow):
	
	def __init__(self, parent = None):
	
		QtGui.QMainWindow.__init__(self, parent)
		self.ui = Ui_MainWindow()
		self.ui.setupUi(self)
		
		self.WordleView = vtkvtg.vtkQtWordleView()
		
		# Set widget for the wordle view
		self.ui.centralWidget.layout().addWidget(self.WordleView.GetWidget())
		
		data_dir = '/Users/emonson/Data/Fodava/EMoGWDataSets/'
		data_file = data_dir + 'sciNews_20110216.mat'
			
		print 'Trying to load data set from .mat file...'

		try:
			MatInput = scipy.io.loadmat(data_file, struct_as_record=True, chars_as_strings=True)
		except:
			raise IOError, "Can't load supplied matlab file"
		
		self.mat_terms = MatInput['terms'].T[0]
		
		self.WavBases = []	# Wavelet bases
		self.Centers = [] # Center of each node
		# NodeWavCoeffs = []
		# NodeScalCoeffs = []
		for ii in range(MatInput['PointsInNet'].shape[1]):
			self.WavBases.append(N.mat(MatInput['WavBases'][0,ii]))			# matrix
			self.Centers.append(N.mat(MatInput['Centers'][0,ii][0]))		# matrix

		terms = vtk.vtkStringArray()
		terms.SetName('dictionary')
		terms.SetNumberOfComponents(1)
		for term in self.mat_terms:
			terms.InsertNextValue(term[0])
		
		self.basis_idx = 0
		
		coeffs = VN.numpy_to_vtk(self.WavBases[self.basis_idx][::-1,0]*100, deep=True)
		coeffs.SetName('coefficient')
		c_sign = VN.numpy_to_vtk(N.sign(self.WavBases[self.basis_idx][::-1,0]), deep=True)
		c_sign.SetName('sign')
		
		# Create a table with some points in it...
		self.table = vtk.vtkTable()
		self.table.AddColumn(terms)
		self.table.AddColumn(coeffs)
		self.table.AddColumn(c_sign)
		
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

		self.WordleView.AddRepresentationFromInput(self.table)
		self.WordleView.SetFieldType(vtkvtg.vtkQtWordleView.ROW_DATA)
		self.WordleView.SetColorByArray(True)
		self.WordleView.SetColorArrayName('sign')
		self.WordleView.SetTermsArrayName('dictionary')
		self.WordleView.SetSizeArrayName('coefficient')
		self.WordleView.ApplyViewTheme(vt)
		self.WordleView.SetMaxNumberOfWords(150);
		self.WordleView.SetFontFamily("Rockwell")
		self.WordleView.SetFontStyle(vtkvtg.vtkQtWordleView.StyleNormal)
		self.WordleView.SetFontWeight(99)
		# self.WordleView.SetOrientation(vtkvtg.vtkQtWordleView.HORIZONTAL)
		self.WordleView.SetOrientation(vtkvtg.vtkQtWordleView.MOSTLY_HORIZONTAL)
		# self.WordleView.SetOrientation(vtkvtg.vtkQtWordleView.HALF_AND_HALF)
		# self.WordleView.SetOrientation(vtkvtg.vtkQtWordleView.MOSTLY_VERTICAL)
		# self.WordleView.SetOrientation(vtkvtg.vtkQtWordleView.VERTICAL)

		self.WordleView.SetLayoutPathShape(vtkvtg.vtkQtWordleView.CIRCULAR_PATH)
		# self.WordleView.SetLayoutPathShape(vtkvtg.vtkQtWordleView.SQUARE_PATH)

		QtCore.QObject.connect(self.ui.actionExit, QtCore.SIGNAL("triggered()"), self.fileExit)

		# Do first update before changing to debug mode...
		self.WordleView.Update()
		self.WordleView.ZoomToBounds()
		

	def keyPressEvent(self, event):
		if event.key() == QtCore.Qt.Key_L:
			self.basis_idx += 1
			coeffs = VN.numpy_to_vtk(self.WavBases[self.basis_idx][::-1,0]*100, deep=True)
			coeffs.SetName('coefficient')
			c_sign = VN.numpy_to_vtk(N.sign(self.WavBases[self.basis_idx][::-1,0]), deep=True)
			c_sign.SetName('sign')
			
			self.table.RemoveColumn(2)
			self.table.RemoveColumn(1)
			self.table.AddColumn(coeffs)
			self.table.AddColumn(c_sign)
			self.WordleView.RemoveAllRepresentations()
			self.WordleView.AddRepresentationFromInput(self.table)

			self.table.Modified()
			self.WordleView.Update()
		
		if event.key() == QtCore.Qt.Key_Space:
			if event.modifiers() == QtCore.Qt.NoModifier:
				self.WordleView.Modified()
				self.WordleView.Update()
			elif event.modifiers() == QtCore.Qt.ShiftModifier:
				self.table.Modified()
				self.WordleView.Update()

		# Write PNG (n)
		# Trying to use a integer-based QImage
		if event.key() == QtCore.Qt.Key_N:
			scene = self.WordleView.GetScene()
			image = QtGui.QImage(256,256,QtGui.QImage.Format_ARGB32)
			painter = QtGui.QPainter(image)
			painter.setRenderHint(QtGui.QPainter.Antialiasing)
			scene.render(painter)
			painter.end()
			image.save("out.png")
		
		# Write PDF (p)
		if event.key() == QtCore.Qt.Key_P:
			scene = self.WordleView.GetScene()
			printer = QtGui.QPrinter()
			printer.setOutputFormat(QtGui.QPrinter.PdfFormat)
			printer.setOutputFileName("out.pdf")
			pdfPainter = QtGui.QPainter(printer)
			scene.render(pdfPainter)
			pdfPainter.end()
		
		# Write SVG (s)
		if event.key() == QtCore.Qt.Key_S:
			scene = self.WordleView.GetScene()
			svggen = QtSvg.QSvgGenerator()
			svggen.setFileName("out.svg")
			svggen.setSize(QtCore.QSize(600, 600))
			svggen.setViewBox(QtCore.QRect(0, 0, 600, 600))
			svggen.setTitle("SVG Generator Example Drawing")
			svggen.setDescription("An SVG drawing created by the SVG Generator")
			svgPainter = QtGui.QPainter(svggen)
			scene.render(svgPainter)
			svgPainter.end()
	
	def resizeEvent(self, event):
		self.WordleView.ZoomToBounds()
		
	def fileExit(self):

		# Usually would use the qApp global variable qApp.quit(), but wasn't working...
		QtGui.QApplication.instance().quit()

if __name__ == "__main__":

	app = QApplication(sys.argv)
	window = SimpleView()
	window.show()
	sys.exit(app.exec_())
