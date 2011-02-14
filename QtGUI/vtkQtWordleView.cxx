/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtWordleView.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkQtWordleView.h"

#include <QFont>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QList>
#include <QPolygonF>
#include <QRectF>
#include <QString>
#include <QTransform>

// DEBUG
// #include <QFontDatabase>

#include "vtkAbstractArray.h"
#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkApplyColors.h"
#include "vtkDataObjectToTable.h"
#include "vtkDataRepresentation.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkUnsignedCharArray.h"
#include "vtkViewTheme.h"

#include <string>
#include <iostream>	// for cout debug
#include <time.h>
// #include <cmath>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkQtWordleView);

//----------------------------------------------------------------------------
vtkQtWordleView::vtkQtWordleView()
{  
	// Scene which will actually be viewed with text
  this->scene = new QGraphicsScene();
	this->scene->setSceneRect(-300, -400, 900, 800);
	this->scene->setItemIndexMethod(QGraphicsScene::BspTreeIndex);
			
	// Keep non-viewed scene for rectangle collision detection
  this->collision_scene = new QGraphicsScene();
	this->collision_scene->setSceneRect(this->scene->sceneRect());
	this->collision_scene->setItemIndexMethod(QGraphicsScene::BspTreeIndex);
	// this->collision_scene->setItemIndexMethod(QGraphicsScene::NoIndex);
	
  this->View = new QGraphicsView();
	this->View->setScene(this->scene);
	this->View->setRenderHint(QPainter::Antialiasing);
	this->View->setCacheMode(QGraphicsView::CacheBackground);
	this->View->setAlignment(Qt::AlignCenter);
	// this->View->setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
	this->View->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
	this->View->setDragMode(QGraphicsView::ScrollHandDrag);
		
  this->ApplyColors = vtkSmartPointer<vtkApplyColors>::New();
  double defCol[3] = {0.827,0.827,0.827};
  this->ApplyColors->SetDefaultPointColor(defCol);
  this->ApplyColors->SetUseCurrentAnnotationColor(true);
  this->ColorArrayNameInternal = 0;
  this->TermsArrayNameInternal = 0;
  this->SizeArrayNameInternal = 0;

  this->DataObjectToTable = vtkSmartPointer<vtkDataObjectToTable>::New();
  this->ApplyColors->SetInputConnection(0, this->DataObjectToTable->GetOutputPort(0));
  this->DataObjectToTable->SetFieldType(vtkDataObjectToTable::VERTEX_DATA);

  this->rectA = new QGraphicsRectItem();
  this->rectB = new QGraphicsRectItem();
  this->lastRect = new QGraphicsRectItem();
  
  this->bigFontSize = 100;
  this->num_words = 150;
  this->maxSize = 0;
  
	this->max_words = 150;
	this->xbuffer = 2.0;
	this->ybuffer = 1.5;
	this->randSpread = 0.2;
	this->thedaMult = 0.75;
	this->thedaPow = 0.66667;
	this->rMult = 2.0;
	this->rPow = 0.5;
	
  this->boundingRect = new QRectF(0.0, 0.0, 0.0, 0.0);
  this->font = new QFont("Adobe Caslon Pro", 12);
  this->font->setStyle(QFont::StyleNormal);
  this->font->setWeight(QFont::Bold);
  
//   QFontDatabase database;
// 	foreach (QString family, database.families()) {
// 		 cout << family.toStdString() << endl;
// 	
// 		 foreach (QString style, database.styles(family)) {
// 				 cout << "\t" << style.toStdString() << endl;
// 		 }
// 		 cout << endl;
// 	}
  
  this->FieldType = vtkQtWordleView::ROW_DATA;
  this->LastInputMTime = 0;
  this->LastMTime = 0;
  
  srand( time(NULL) );
}

//----------------------------------------------------------------------------
vtkQtWordleView::~vtkQtWordleView()
{
  if(this->View)
    {
    delete this->View;
    }
}

//----------------------------------------------------------------------------
QWidget* vtkQtWordleView::GetWidget()
{
  return this->View;
}

//----------------------------------------------------------------------------
void vtkQtWordleView::SetFieldType(int type)
{
  this->DataObjectToTable->SetFieldType(type);
  if(this->FieldType != type)
    {
    this->FieldType = type;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkQtWordleView::AddRepresentationInternal(vtkDataRepresentation* rep)
{    
  vtkAlgorithmOutput *annConn, *conn;
  conn = rep->GetInputConnection();
  annConn = rep->GetInternalAnnotationOutputPort();

  this->DataObjectToTable->SetInputConnection(0, conn);

  if(annConn)
    {
    this->ApplyColors->SetInputConnection(1, annConn);
    }
}

void vtkQtWordleView::RemoveRepresentationInternal(vtkDataRepresentation* rep)
{   
  vtkAlgorithmOutput *annConn, *conn;
  conn = rep->GetInputConnection();
  annConn = rep->GetInternalAnnotationOutputPort();

  this->DataObjectToTable->RemoveInputConnection(0, conn);
  this->ApplyColors->RemoveInputConnection(1, annConn);
}

//----------------------------------------------------------------------------
void vtkQtWordleView::SetColorByArray(bool b)
{
  this->ApplyColors->SetUsePointLookupTable(b);
}

//----------------------------------------------------------------------------
bool vtkQtWordleView::GetColorByArray()
{
  return this->ApplyColors->GetUsePointLookupTable();
}

//----------------------------------------------------------------------------
void vtkQtWordleView::SetColorArrayName(const char* name)
{
  this->SetColorArrayNameInternal(name);
  this->ApplyColors->SetInputArrayToProcess(0, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_ROWS, name);
}

//----------------------------------------------------------------------------
const char* vtkQtWordleView::GetColorArrayName()
{
  return this->GetColorArrayNameInternal();
}

//----------------------------------------------------------------------------
void vtkQtWordleView::SetTermsArrayName(const char* name)
{
  this->SetTermsArrayNameInternal(name);
}

//----------------------------------------------------------------------------
const char* vtkQtWordleView::GetTermsArrayName()
{
  return this->GetTermsArrayNameInternal();
}

//----------------------------------------------------------------------------
void vtkQtWordleView::SetSizeArrayName(const char* name)
{
  this->SetSizeArrayNameInternal(name);
}

//----------------------------------------------------------------------------
const char* vtkQtWordleView::GetSizeArrayName()
{
  return this->GetSizeArrayNameInternal();
}

//----------------------------------------------------------------------------
void vtkQtWordleView::ClearGraphicsView()
{
	this->scene->clear();
	this->collision_scene->clear();
}

//----------------------------------------------------------------------------
vtkVector2f vtkQtWordleView::CartesianToPolar(vtkVector2f posArr)
{
// 	r = N.sqrt((posArr*posArr).sum())
// 	phi = N.arctan2(posArr[1],posArr[0])
// 	return N.array([r, phi],dtype='float32')
	double r = sqrt(posArr.X()*posArr.X() + posArr.Y()*posArr.Y());
	double phi = atan2(posArr.Y(), posArr.X());
	vtkVector2f r_phi(r, phi);
	
	return r_phi;
}
	
//----------------------------------------------------------------------------
vtkVector2f vtkQtWordleView::PolarToCartesian(vtkVector2f posArr)
{
// 	x = posArr[0]*N.cos(posArr[1])
// 	y = posArr[0]*N.sin(posArr[1])
// 	return N.array([x,y],dtype='float32')
	float x = posArr.X()*cos(posArr.Y());
	float y = posArr.X()*sin(posArr.Y());
	vtkVector2f result(x, y);
	
	return result;
}

//----------------------------------------------------------------------------
vtkVector2f vtkQtWordleView::MakeInitialPosition()
{
	// return (self.scene.sceneRect().width()*randSpread)*N.random.randn(2)
	
	// TODO: Check that this range is reasonable since used to be randn call...
	float x = (float)(this->scene->sceneRect().width())*this->randSpread*((float)rand()/(float)RAND_MAX);
	float y = (float)(this->scene->sceneRect().height())*this->randSpread*((float)rand()/(float)RAND_MAX);
	vtkVector2f result(x, y);
	
	return result;
}

//----------------------------------------------------------------------------
void vtkQtWordleView::UpdatePositionSpirals(WordObject* word)
{
	// ri = R0[0] + rdelta*theda
	// theda += (delta*N.pi)/ri
	// r = R0[0] + rdelta*theda
	// return self.PolarToCartesian(N.array([r[0],theda[0]]))+initPosArr
	
	// Updating in place
	double const Pi = 4.0 * atan(1);
	double ri = word->R0 + (word->rdelta * word->theda);
	word->theda += (word->delta * Pi) / ri;
	double r = word->R0 + word->rdelta * word->theda;
	vtkVector2f dPos = this->PolarToCartesian(vtkVector2f(r, word->theda));
	
	word->pos.SetX(dPos.X() + word->initial_pos.X());
	word->pos.SetY(dPos.Y() + word->initial_pos.Y());
}

namespace
{
	// Compare the two word objects for vector sort
	// Want to sort in DESCENDING order, so return "greater than" rather than <
	bool compWordObject(const WordObject& v1, const WordObject& v2)
	{
		if (v1.getsize() > v2.getsize())
			{
			return true;
			}
		else
			{
			return false;
			}
	}
}

//----------------------------------------------------------------------------
void vtkQtWordleView::BuildWordObjectsList()
{
	vtkDataRepresentation* rep = this->GetRepresentation();
  if (!rep)
    {
    return;
    }
  this->ApplyColors->Update();
  vtkTable* table = vtkTable::SafeDownCast(this->ApplyColors->GetOutput());
  if (!table)
    {
    return;
    }
  vtkStringArray* terms = vtkStringArray::SafeDownCast(table->GetColumnByName(this->TermsArrayNameInternal));
  if (!terms)
    {
    printf("Terms array not vtkStringArray\n");
    return;
    }
  vtkUnsignedCharArray* colors = vtkUnsignedCharArray::SafeDownCast(table->GetColumnByName("vtkApplyColors color"));
  if (!colors)
    {
    printf("Colors array not vtkUnsignedCharArray\n");
    return;
    }
  vtkDoubleArray* sizes = vtkDoubleArray::SafeDownCast(table->GetColumnByName(this->SizeArrayNameInternal));
  if (!sizes)
    {
    printf("Size array not vtkDoubleArray\n");
    return;
    }
  
	this->sortedWordObjectList.clear();
	
	double sizeRange[2];
	sizes->GetRange(sizeRange);
	this->maxSize = sizeRange[1];

  WordObject word;
  unsigned char cc[4];
  for (vtkIdType ii=0; ii < terms->GetNumberOfValues(); ++ii)
    {
    word.text = terms->GetValue(ii);
    word.size = fabs(sizes->GetValue(ii));
    colors->GetTupleValue(ii, cc);
    word.color = new QColor(cc[0],cc[1],cc[2],cc[3]);
		word.font_size = (int)((float)this->bigFontSize*(float)word.size/(float)this->maxSize);
		word.pos = this->MakeInitialPosition();
		word.initial_pos = word.pos;
		word.delta = this->thedaMult*pow((float)word.font_size, this->thedaPow);
		word.rdelta = this->rMult*pow((float)word.font_size, this->rPow);
		
		this->font->setPointSize(word.font_size);
		QPainterPath pathOrig;
		pathOrig.addText(0.0f, 0.0f, *this->font, QString(word.text.c_str()));
		QTransform trans;
		// TODO: This is where the orientation needs to be set...
		// index values can take on [0,4] inclusive
		// orient = self.ui.comboBox_orientation.currentIndex()
		// if (QtCore.qrand() % 4) in (N.arange(orient+1)-1):
		//	trans.rotate(90)
		trans.rotate(0);
		word.painter_path = trans.map(pathOrig);
 	
		QGraphicsPathItem* pathItem = new QGraphicsPathItem(word.painter_path);
		pathItem->setPen(QPen(Qt::NoPen));
		pathItem->setBrush(*word.color);
		word.path_item = pathItem;
		
		// DEBUG
		this->scene->addItem(word.path_item);
		
		// Manually build two-deep tree right here for now...
		QGraphicsRectItem* rect = new QGraphicsRectItem(pathItem->boundingRect().adjusted(-this->xbuffer, -this->ybuffer, this->xbuffer, this->ybuffer));
		rect->setPen(QPen(Qt::NoPen));
		QList<QPolygonF> shapes = word.painter_path.toSubpathPolygons();
		for (int jj=0; jj < shapes.size(); ++jj)
		  {
			QGraphicsRectItem* subRect = new QGraphicsRectItem(shapes.at(jj).boundingRect().adjusted(-this->xbuffer,-this->ybuffer,this->xbuffer,this->ybuffer));
			subRect->setParentItem(rect);
			subRect->setPen(QPen(Qt::NoPen));
			}
		word.rect_item = rect;

    (this->sortedWordObjectList).push_back(word);
    }
  
  std::sort((this->sortedWordObjectList).begin(), (this->sortedWordObjectList).end(), compWordObject);
  
  // DEBUG: Dump the vector to check the result
//   for (std::vector<WordObject>::iterator citer = (this->sortedWordObjectList).begin();
//   		 citer != (this->sortedWordObjectList).end(); ++citer)
//   	{
//   	cout << "Text: " << (*citer).text << endl;
//   	cout << "Size: " << std::fixed << (*citer).size << endl;
//   	cout << endl;
//   	}
}

//----------------------------------------------------------------------------
bool vtkQtWordleView::HierarchicalRectCollision_B()
{
// 	def hierarchicalRectCollision_B(self):
// 		rA = self.rectA.mapRectToScene(self.rectA.rect())
// 		rB = self.rectB.mapRectToScene(self.rectB.rect())
// 		ax1,ay1 = rA.x(), rA.y()
// 		ax2,ay2 = ax1+rA.width(), ay1+rA.height()
// 		bx1,by1 = rB.x(), rB.y()
// 		bx2,by2 = bx1+rB.width(), by1+rB.height()
// 		
// 		if ((ax2<bx1) or (ax1>bx2) or (ay2<by1) or (ay1>by2)):
// 			# print 'first stage false'
// 			return False
// 		else:
// 			rBchildren = self.rectB.childItems()
// 			overList = [True]*len(rBchildren)
// 			for ii,item in enumerate(rBchildren):
// 				rB = self.rectB.mapRectToScene(item.rect())
// 				bx1,by1 = rB.x(), rB.y()
// 				bx2,by2 = bx1+rB.width(), by1+rB.height()
// 				if ((ax2<bx1) or (ax1>bx2) or (ay2<by1) or (ay1>by2)):
// 					overList[ii] = False
// 				else:
// 					overList[ii] = True
// 					break
// 			return any(overList)

	return false;
}

//----------------------------------------------------------------------------
void vtkQtWordleView::DoLayout()
{
// 	def doLayout(self):
// 		self.ui.progressBar.reset()
// 		self.ui.progressBar.setMinimum(0)
// 		self.ui.progressBar.setMaximum(self.num_words)
// 		self.lastRect = self.sortedWordObjectList[0].rect_item
// 		self.scene.setSceneRect(-300, -400, 900, 800)
// 		print "Placing words in image"
// 		
// 		self.num_words = self.ui.spinBox_numWords.value()
// 		tmpRect = self.sortedWordObjectList[0].path_item.boundingRect()
// 		for ii,word in enumerate(self.sortedWordObjectList[:self.num_words]):
// 			print ii, word.text, word.font_size
// 			self.collision_scene.addItem(word.rect_item)
// 			if ii == 0:
// 				overlap = False
// 			else:
// 				overlap = True
// 				
// 			while overlap:
// 				# First test for overlap with last one intersected
// 				# if word.rect_item.collidesWithItem(lastWord.rect_item):
// 				self.rectA = word.rect_item
// 				self.rectB = self.lastRect
// 				if self.hierarchicalRectCollision_B():
// 					overlap = True
// 					# print "Last word overlap"
// 				else:
// 					itemsList = word.rect_item.collidingItems()
// 					# print itemsList
// 					overlapList = [True]*len(itemsList)
// 					for jj,item in enumerate(itemsList):
// 						self.rectB = item
// 						if self.hierarchicalRectCollision_B():
// 							overlapList[jj] = True
// 							self.lastRect = item
// 							break
// 						else:
// 							overlapList[jj] = False
// 					overlap = any(overlapList)
// 					
// 				if overlap:
// 					word.pos = self.UpdatePositionSpirals(word.initial_pos,word.theda,word.R0,word.delta,word.rdelta)
// 					word.rect_item.setPos(word.pos[0],word.pos[1])
// 					# TEST
// 					# if ii in [1, 2, 10, 20, 40, 80, 100, 140]:
// 					#	isz = N.array([2,2])
// 					#	trackDraw.ellipse(N.concatenate((word.pos-isz, word.pos+isz)).tolist(),fill=(0,200,5*ii))
// 			
// 			word.rect_item.setPos(word.pos[0],word.pos[1])
// 			word.path_item.setPos(word.pos[0],word.pos[1])		
// 			self.scene.addItem(word.path_item)
// 			tmpRect = tmpRect.united(word.path_item.mapRectToScene(word.path_item.boundingRect()))
// 			self.ui.progressBar.setValue(ii)
// 			# Can't get the view to update after each word is added...
// 			# self.ui.graphicsView.repaint()
// 			# self.scene.update(self.scene.sceneRect())
// 		
// 		adjX = (tmpRect.width()*0.05)/2.0
// 		adjY = (tmpRect.height()*0.05)/2.0
// 		self.boundingRect = tmpRect.adjusted(-adjX,-adjY,adjX,adjY)
// 		self.scene.setSceneRect(self.boundingRect)
// 		self.ui.graphicsView.fitInView(self.boundingRect,QtCore.Qt.KeepAspectRatio)
// 		self.ui.progressBar.reset()
// 
// 			# TEST
// 			# isz = N.array([2,2])
// 			# trackDraw.ellipse(N.concatenate((newPos-isz, newPos+isz)).tolist(),fill=(0,5*ii,200))

}

//----------------------------------------------------------------------------
void vtkQtWordleView::Update()
{
  vtkDataRepresentation* rep = this->GetRepresentation();
  if (!rep)
    {
    // Remove VTK data from the adapter
    this->View->update();
    return;
    }

  // Make the data current
  vtkAlgorithmOutput *conn;
  conn = rep->GetInputConnection();
  conn->GetProducer()->Update();

  vtkDataObject *d = conn->GetProducer()->GetOutputDataObject(0);
  if (d->GetMTime() > this->LastInputMTime ||
      this->GetMTime() > this->LastMTime)
    {
    this->DataObjectToTable->Update();
    this->ApplyColors->Update();

    this->LastInputMTime = d->GetMTime();
    this->LastMTime = this->GetMTime();
    }
    
  // DEBUG
  this->BuildWordObjectsList();

  this->View->update();
}

//----------------------------------------------------------------------------
void vtkQtWordleView::ApplyViewTheme(vtkViewTheme* theme)
{
  this->Superclass::ApplyViewTheme(theme);

  this->ApplyColors->SetPointLookupTable(theme->GetPointLookupTable());

  this->ApplyColors->SetDefaultPointColor(theme->GetPointColor());
  this->ApplyColors->SetDefaultPointOpacity(theme->GetPointOpacity());
  this->ApplyColors->SetDefaultCellColor(theme->GetCellColor());
  this->ApplyColors->SetDefaultCellOpacity(theme->GetCellOpacity());
  this->ApplyColors->SetSelectedPointColor(theme->GetSelectedPointColor());
  this->ApplyColors->SetSelectedPointOpacity(theme->GetSelectedPointOpacity());
  this->ApplyColors->SetSelectedCellColor(theme->GetSelectedCellColor());
  this->ApplyColors->SetSelectedCellOpacity(theme->GetSelectedCellOpacity());
}

//----------------------------------------------------------------------------
void vtkQtWordleView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

