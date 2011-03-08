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
		
		vt = vtk.vtkViewTheme()
		lut = vtk.vtkLookupTable()
		lut.SetHueRange(0,0)
		lut.SetValueRange(0,1)
		lut.SetSaturationRange(1,1)
		lut.Build()
		vt.SetPointLookupTable(lut)

		self.WordleView.AddRepresentationFromInput(self.table)
		self.WordleView.SetFieldType(vtkvtg.vtkQtWordleView.ROW_DATA)
		self.WordleView.SetColorByArray(True)
		self.WordleView.SetColorArrayName('coefficient')
		self.WordleView.SetTermsArrayName('dictionary')
		self.WordleView.SetSizeArrayName('coefficient')
		self.WordleView.ApplyViewTheme(vt)
		self.WordleView.SetMaxNumberOfWords(50);
		self.WordleView.SetFontFamily("Rockwell")
		self.WordleView.SetFontStyle(vtkvtg.vtkQtWordleView.StyleNormal)
		self.WordleView.SetFontWeight(99)
		# self.WordleView.SetOrientation(vtkvtg.vtkQtWordleView.HORIZONTAL)
		self.WordleView.SetOrientation(vtkvtg.vtkQtWordleView.MOSTLY_HORIZONTAL)
		# self.WordleView.SetOrientation(vtkvtg.vtkQtWordleView.HALF_AND_HALF)
		# self.WordleView.SetOrientation(vtkvtg.vtkQtWordleView.MOSTLY_VERTICAL)
		# self.WordleView.SetOrientation(vtkvtg.vtkQtWordleView.VERTICAL)

		QtCore.QObject.connect(self.ui.actionExit, QtCore.SIGNAL("triggered()"), self.fileExit)


		self.WordleView.Update()
		self.WordleView.ZoomToBounds()
				
	def keyPressEvent(self, event):
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
