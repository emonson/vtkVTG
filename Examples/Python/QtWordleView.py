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

from QtSimpleView import Ui_MainWindow

class SimpleView(QtGui.QMainWindow):
	
	def __init__(self, parent = None):
	
		QtGui.QMainWindow.__init__(self, parent)
		self.ui = Ui_MainWindow()
		self.ui.setupUi(self)
		
		self.WordleView = vtkvtg.vtkQtWordleView()
		
		# Set widget for the wordle view
		self.ui.centralWidget.layout().addWidget(self.WordleView.GetWidget())
		
		term_list = [
			"straw",
			"gold",
			"name",
			"miller",
			"manikin",
			"daughter",
			"little",
			"queen",
			"man",
			"came",
			"girl",
			"give",
			"spin",
			"full",
			"whirr",
			"spun",
			"night",
			"one",
			"child",
			"king",
			"room",
			"answered",
			"morning",
			"names",
			"took",
			"began",
			"thought",
			"time",
			"find",
			"world",
			"day",
			"leg",
			"whole",
			"another",
			"three",
			"rumpelstiltskin",
			"knew",
			"left",
			"help",
			"still",
			"crying",
			"gave",
			"good",
			"round",
			"wheel",
			"second",
			"opened",
			"messenger",
			"ever",
			"told",
			"quite",
			"alone",
			"mistress",
			"larger",
			"became",
			"first",
			"brought",
			"put",
			"ring",
			"devil",
			"next",
			"house",
			"taken",
			"life",
			"door",
			"fire",
			"reel",
			"glad",
			"must",
			"beautiful",
			"heard",
			"promised",
			"saw",
			"perhaps",
			"also",
			"poor",
			"shall",
			"third",
			"wife",
			"necklace",
			"bring",
			"daybreak",
			"earth",
			"hopped",
			"inquiries",
			"cried",
			"dearer",
			"front",
			"far",
			"bid",
			"early",
			"every",
			"hands",
			"foot",
			"course",
			"always",
			"cry",
			"become",
			"conrad",
			"art",
			"hare",
			"hard",
			"back",
			"alive",
			"die",
			"curious",
			"bake",
			"even",
			"appear",
			"fox",
			"enough",
			"finger",
			"harry",
			"caspar",
			"greedy",
			"found",
			"country",
			"days",
			"asked",
			"already",
			"afterwards",
			"burning",
			"imagine",
			"delighted",
			"frightened",
			"appeared",
			"idea",
			"clever",
			"else",
			"alas",
			"astonished",
			"grew",
			"happened",
			"heart",
			"deep",
			"high",
			"evening",
			"anger",
			"commanded",
			"happen",
			"beyond",
			"end",
			"able",
			"forest",
			"jumping",
			"brew",
			"important",
			"balthazar",
			"inquire",
			"glittering"
			]

		size_list = [
			12.0,
			10.0,
			10.0,
			9.0,
			9.0,
			9.0,
			9.0,
			8.0,
			8.0,
			8.0,
			7.0,
			7.0,
			6.0,
			6.0,
			6.0,
			5.0,
			5.0,
			5.0,
			5.0,
			5.0,
			5.0,
			4.0,
			4.0,
			4.0,
			4.0,
			4.0,
			4.0,
			4.0,
			3.0,
			3.0,
			3.0,
			3.0,
			3.0,
			3.0,
			3.0,
			3.0,
			3.0,
			3.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			2.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			1.0,
			]

		terms = vtk.vtkStringArray()
		terms.SetName('dictionary')
		terms.SetNumberOfComponents(1)
		for term in term_list:
			terms.InsertNextValue(term)
		
		coeffs = vtk.vtkDoubleArray()
		coeffs.SetName('coefficient')
		coeffs.SetNumberOfComponents(1)
		for size in size_list:
			coeffs.InsertNextValue(size)
		
		# Create a table with some points in it...
		self.table = vtk.vtkTable()
		self.table.AddColumn(terms)
		self.table.AddColumn(coeffs)
		
		self.vt = vtk.vtkViewTheme()
		lut = vtk.vtkLookupTable()
		lut.SetHueRange(0,0)
		lut.SetValueRange(0,1)
		lut.SetSaturationRange(1,1)
		lut.Build()
		# Set value for no color by array
		self.vt.SetPointColor(0,0,0)
		# Set LUT for color by array
		self.vt.SetPointLookupTable(lut)
		# ViewTheme Background color is black by default
		self.vt.SetBackgroundColor(1,1,1)

		self.vt2 = vtk.vtkViewTheme()
		lut2 = vtk.vtkLookupTable()
		lut2.SetHueRange(0, 0.66)
		lut2.SetValueRange(0.7, 0.7)
		lut2.SetSaturationRange(1, 1)
		lut2.Build()
		# Set value for no color by array
		self.vt2.SetPointColor(0,0,0)
		# Set LUT for color by array
		self.vt2.SetPointLookupTable(lut2)
		# ViewTheme Background color is black by default
		self.vt2.SetBackgroundColor(1,1,1)

		self.WordleView.AddRepresentationFromInput(self.table)
		self.WordleView.SetFieldType(vtkvtg.vtkQtWordleView.ROW_DATA)
		self.WordleView.SetColorByArray(True)
		self.WordleView.SetColorArrayName('coefficient')
		self.WordleView.SetTermsArrayName('dictionary')
		self.WordleView.SetSizeArrayName('coefficient')
		self.WordleView.SetOutputImageDataDimensions(200, 200)
		self.WordleView.ApplyViewTheme(self.vt)
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

		self.WordleView.Update()
		self.WordleView.ZoomToBounds()
				
		self.color_by_array = True
		self.font_flag = True

		QtCore.QObject.connect(self.ui.actionExit, QtCore.SIGNAL("triggered()"), self.fileExit)


	def keyPressEvent(self, event):
		if event.key() == QtCore.Qt.Key_Space:
			if event.modifiers() == QtCore.Qt.NoModifier:
				self.WordleView.Modified()
				self.WordleView.Update()
			elif event.modifiers() == QtCore.Qt.ShiftModifier:
				self.table.Modified()
				self.WordleView.Update()
		
		# Change Fonts (f)
		if event.key() == QtCore.Qt.Key_F:
			if self.font_flag:
				self.WordleView.SetFontFamily("Tekton Pro")
			else:
				self.WordleView.SetFontFamily("Rockwell")
			self.font_flag = not self.font_flag
			self.WordleView.Update()
		
		# Write PNG (n)
		# Trying to use a integer-based QImage
		if event.key() == QtCore.Qt.Key_N:
			self.WordleView.SaveImage("out.png")
		
		# Grab ImageData (i)
		if event.key() == QtCore.Qt.Key_I:
			image = self.WordleView.GetImageData()
			# print image
			iw = vtk.vtkXMLImageDataWriter()
			iw.SetInput(image)
			iw.SetFileName("out.vti")
			iw.Write()
		
		# Write PDF (p)
		if event.key() == QtCore.Qt.Key_P:
			self.WordleView.SavePDF("out.pdf")
		
		# Write SVG (s)
		# SVG generation not compiled in by default...
		# if event.key() == QtCore.Qt.Key_S:
		# 	self.WordleView.SaveSVG("out.svg")
	
		# Switch only colors
		if event.key() == QtCore.Qt.Key_C:
			self.color_by_array = not self.color_by_array
			if event.modifiers() == QtCore.Qt.NoModifier:
				self.WordleView.SetColorByArray(self.color_by_array)
				self.WordleView.Update()
			elif event.modifiers() == QtCore.Qt.ShiftModifier:
				if self.color_by_array:
					self.WordleView.ApplyViewTheme(self.vt)
				else:
					self.WordleView.ApplyViewTheme(self.vt2)
				self.WordleView.Update()

	def resizeEvent(self, event):
		self.WordleView.ZoomToBounds()
		
	def fileExit(self):

		QtGui.QApplication.instance().quit()

if __name__ == "__main__":

	app = QApplication(sys.argv)
	window = SimpleView()
	window.show()
	sys.exit(app.exec_())
