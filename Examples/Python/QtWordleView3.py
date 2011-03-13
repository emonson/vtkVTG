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

from QtSimpleView3 import Ui_MainWindow

class SimpleView(QtGui.QMainWindow):
	
	def __init__(self, parent = None):
	
		QtGui.QMainWindow.__init__(self, parent)
		self.ui = Ui_MainWindow()
		self.ui.setupUi(self)
		
		self.WordleView = vtkvtg.vtkQtWordleView()
		
		# Set widget for the wordle view
		self.ui.centralWidget.layout().addWidget(self.WordleView.GetWidget(),0,1)
		
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
		
		coeffs2 = vtk.vtkDoubleArray()
		coeffs2.SetName('neg_coeff')
		coeffs2.SetNumberOfComponents(1)
		for size in size_list:
			coeffs2.InsertNextValue(N.sqrt(size))
		
		# Create a table with some points in it...
		self.table = vtk.vtkTable()
		self.table.AddColumn(terms)
		self.table.AddColumn(coeffs)
		self.table.AddColumn(coeffs2)
		
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

		self.ui.doubleSpinBox_xbuffer.setValue(self.WordleView.Getxbuffer())
		self.ui.doubleSpinBox_ybuffer.setValue(self.WordleView.Getybuffer())
		self.ui.doubleSpinBox_randSpread.setValue(self.WordleView.GetrandSpread())
		self.ui.doubleSpinBox_thetaMult.setValue(self.WordleView.GetthetaMult())
		self.ui.doubleSpinBox_thetaPow.setValue(self.WordleView.GetthetaPow())
		self.ui.doubleSpinBox_rMult.setValue(self.WordleView.GetrMult())
		self.ui.doubleSpinBox_rPow.setValue(self.WordleView.GetrPow())
		self.ui.doubleSpinBox_dMult.setValue(self.WordleView.GetdMult())
		self.ui.doubleSpinBox_dPow.setValue(self.WordleView.GetdPow())
		self.ui.doubleSpinBox_wordSizePower.setValue(self.WordleView.GetWordSizePower())
		self.ui.comboBox_pathShape.setCurrentIndex(self.WordleView.GetLayoutPathShape())

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

		QtCore.QObject.connect(self.ui.actionExit, QtCore.SIGNAL("triggered()"), self.fileExit)

		self.WordleView.Update()
		self.WordleView.ZoomToBounds()
		
# 		fams = vtk.vtkStringArray()
# 		self.WordleView.GetAllFontFamilies(fams)
# 		for ii in range(fams.GetNumberOfValues()):
# 			print fams.GetValue(ii)
				
		# DEBUG
# 		self.WordleView.SetWatchLayout(True)
# 		self.WordleView.SetWatchCollision(True)
# 		self.WordleView.SetWatchQuadTree(True)
# 		self.WordleView.SetWatchDelay(50000)
		self.color_by_array = True
		self.font_flag = True


	@QtCore.pyqtSlot(float)
	def on_doubleSpinBox_xbuffer_valueChanged(self,val):
		self.WordleView.Setxbuffer(val)
		self.table.Modified()
		self.WordleView.Update()
	
	@QtCore.pyqtSlot(float)
	def on_doubleSpinBox_ybuffer_valueChanged(self,val):
		self.WordleView.Setybuffer(val)
		self.table.Modified()
		self.WordleView.Update()
	
	@QtCore.pyqtSlot(float)
	def on_doubleSpinBox_randSpread_valueChanged(self,val):
		self.WordleView.SetrandSpread(val)
		self.table.Modified()
		self.WordleView.Update()
	
	@QtCore.pyqtSlot(float)
	def on_doubleSpinBox_thetaMult_valueChanged(self,val):
		self.WordleView.SetthetaMult(val)
		self.table.Modified()
		self.WordleView.Update()
	
	@QtCore.pyqtSlot(float)
	def on_doubleSpinBox_thetaPow_valueChanged(self,val):
		self.WordleView.SetthetaPow(val)
		self.table.Modified()
		self.WordleView.Update()
	
	@QtCore.pyqtSlot(float)
	def on_doubleSpinBox_rMult_valueChanged(self,val):
		self.WordleView.SetrMult(val)
		self.table.Modified()
		self.WordleView.Update()
	
	@QtCore.pyqtSlot(float)
	def on_doubleSpinBox_rPow_valueChanged(self,val):
		self.WordleView.SetrPow(val)
		self.table.Modified()
		self.WordleView.Update()
	
	@QtCore.pyqtSlot(float)
	def on_doubleSpinBox_dMult_valueChanged(self,val):
		self.WordleView.SetdMult(val)
		self.table.Modified()
		self.WordleView.Update()
	
	@QtCore.pyqtSlot(float)
	def on_doubleSpinBox_dPow_valueChanged(self,val):
		self.WordleView.SetdPow(val)
		self.table.Modified()
		self.WordleView.Update()
	
	@QtCore.pyqtSlot(float)
	def on_doubleSpinBox_wordSizePower_valueChanged(self,val):
		self.WordleView.SetWordSizePower(val)
		self.table.Modified()
		self.WordleView.Update()
	
	@QtCore.pyqtSlot(int)
	def on_comboBox_pathShape_currentIndexChanged(self,val):
		self.WordleView.SetLayoutPathShape(val)
		# self.table.Modified()
		self.WordleView.Update()
	
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
			scene = self.WordleView.GetScene()
			rectf = QtCore.QRectF(scene.sceneRect())
			width = rectf.width()
			height = rectf.height()
			if (width > height):
				diff = (width-height)/2.0
				rectf.adjust(0,-diff,0,diff)
			else:
				diff = (height-width)/2.0
				rectf.adjust(-diff,0,diff,0)
			image = QtGui.QImage(256,256,QtGui.QImage.Format_ARGB32)
			image.fill(QtGui.QColor(255,255,255,255).rgba())
			painter = QtGui.QPainter(image)
			painter.setRenderHint(QtGui.QPainter.Antialiasing)
			scene.render(painter, QtCore.QRectF(image.rect()), rectf)
			painter.end()
			image.save("out.png")
		
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
			scene = self.WordleView.GetScene()
			printer = QtGui.QPrinter()
			printer.setOutputFormat(QtGui.QPrinter.PdfFormat)
			printer.setOutputFileName("out.pdf")
			pdfPainter = QtGui.QPainter(printer)
			scene.render(pdfPainter)
			pdfPainter.end()
		
		# Write SVG (s)
		if event.key() == QtCore.Qt.Key_S:
			self.WordleView.SaveSVG("out.svg")
# 			scene = self.WordleView.GetScene()
# 			svggen = QtSvg.QSvgGenerator()
# 			svggen.setFileName("out.svg")
# 			svggen.setSize(QtCore.QSize(600, 600))
# 			svggen.setViewBox(QtCore.QRect(0, 0, 600, 600))
# 			svggen.setTitle("SVG Generator Example Drawing")
# 			svggen.setDescription("An SVG drawing created by the SVG Generator")
# 			svgPainter = QtGui.QPainter(svggen)
# 			scene.render(svgPainter)
# 			svgPainter.end()
	
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

		# Usually would use the qApp global variable qApp.quit(), but wasn't working...
		QtGui.QApplication.instance().quit()

if __name__ == "__main__":

	app = QApplication(sys.argv)
	window = SimpleView()
	window.show()
	sys.exit(app.exec_())
