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
#include <QFontDatabase>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QList>
#include <QPolygonF>
#include <QRectF>
#include <QString>
#include <QStringList>
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
	this->scene->setItemIndexMethod(QGraphicsScene::NoIndex);
			
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
  this->MaxNumberOfWords = 150;
  this->orientation = vtkQtWordleView::HORIZONTAL;
  
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
  
	this->FontDatabase = new QFontDatabase();
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
void vtkQtWordleView::SetFontFamily(const char* name)
{
  QStringList families = this->FontDatabase->families();
  if (families.contains(QString(name)))
		{
		this->font->setFamily(QString(name));
		}
	else
		{
		vtkDebugMacro(<< "Font family does not match a known entry in Qt database.");
		}
}

//----------------------------------------------------------------------------
const char* vtkQtWordleView::GetFontFamily()
{
  return this->font->family().toStdString().c_str();
}

//----------------------------------------------------------------------------
bool vtkQtWordleView::FontFamilyExists(const char* name)
{
  QStringList families = this->FontDatabase->families();
  if (families.contains(QString(name)))
		{
		return true;
		}
	else
		{
		return false;
		}
}

//----------------------------------------------------------------------------
void vtkQtWordleView::SetFontStyle(int style)
{
  // Convert from int to QFont::Style
  QFont::Style val;
  switch (style)
    {
    case 0:
    	val = QFont::StyleNormal;
    	break;
    case 1:
    	val = QFont::StyleItalic;
    	break;
    case 2:
    	val = QFont::StyleOblique;
    	break;
    default:
    	vtkDebugMacro(<< "Font style not in correct range.");
    }
	if (style <= 2 && style >= 0)
		{
		this->font->setStyle(val);
		}
}

//----------------------------------------------------------------------------
int vtkQtWordleView::GetFontStyle()
{
  return this->font->style();
}

//----------------------------------------------------------------------------
void vtkQtWordleView::SetFontWeight(int weight)
{
  if (weight <= 99 && weight >= 0)
		{
		this->font->setWeight(weight);
		}
	else
		{
		vtkDebugMacro(<< "Font weight not in correct range.");
		}
}

//----------------------------------------------------------------------------
int vtkQtWordleView::GetFontWeight()
{
  return this->font->weight();
}

//----------------------------------------------------------------------------
void vtkQtWordleView::SetOrientation(int orientation)
{
  if (orientation <= 4 && orientation >= 0)
		{
		this->orientation = orientation;
		}
	else
		{
		vtkDebugMacro(<< "Orientation not in correct range.");
		}
}

//----------------------------------------------------------------------------
int vtkQtWordleView::GetOrientation()
{
  return this->orientation;
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
	QList<QGraphicsItem*> sceneItems = this->scene->items();
	for (int ii=0; ii < sceneItems.length(); ++ii)
		{
		this->scene->removeItem(sceneItems[ii]);
		}
	this->scene->clear();
}

//----------------------------------------------------------------------------
void vtkQtWordleView::ZoomToBounds()
{
		this->View->fitInView(this->scene->sceneRect(), Qt::KeepAspectRatio);
}

//----------------------------------------------------------------------------
QGraphicsScene* vtkQtWordleView::GetScene()
{
		return this->scene;
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
	double x = (double)(this->scene->sceneRect().width())*this->randSpread*((double)rand()/(double)RAND_MAX);
	double y = (double)(this->scene->sceneRect().height())*this->randSpread*((double)rand()/(double)RAND_MAX);
	vtkVector2f result((float)x, (float)y);
	
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
	double maxSize = sizeRange[1];

  WordObject word;
  unsigned char cc[4];
  for (vtkIdType ii=0; ii < terms->GetNumberOfValues(); ++ii)
    {
    word.text = terms->GetValue(ii);
    word.original_index = ii;
    word.size = fabs(sizes->GetValue(ii));
    colors->GetTupleValue(ii, cc);
    word.color = new QColor(cc[0],cc[1],cc[2],cc[3]);
		word.font_size = (int)((float)this->bigFontSize*(float)word.size/(float)maxSize);
		if (word.font_size < 1)
			{
			word.font_size = 1;
			}
		word.pos = this->MakeInitialPosition();
		word.initial_pos = word.pos;
		word.delta = this->thedaMult*pow((float)word.font_size, this->thedaPow);
		word.rdelta = this->rMult*pow((float)word.font_size, this->rPow);
		
		this->font->setPointSize(word.font_size);
		QPainterPath pathOrig;
		pathOrig.addText(0.0f, 0.0f, *this->font, QString(word.text.c_str()));
		QTransform trans;
		// Orientation values can take on [0,4] inclusive
		int flip = rand() % 4;
		if (flip <= (this->orientation-1))
			{
			trans.rotate(90);
			}
		word.painter_path = trans.map(pathOrig);
 	
		QGraphicsPathItem* pathItem = new QGraphicsPathItem(word.painter_path);
		pathItem->setPen(QPen(Qt::NoPen));
		pathItem->setBrush(*word.color);
		word.path_item = pathItem;
		
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

		word.rect_item->setPos(word.pos.X(),word.pos.Y());
		word.path_item->setPos(word.pos.X(),word.pos.Y());	

    (this->sortedWordObjectList).push_back(word);
    }
  
  std::sort((this->sortedWordObjectList).begin(), (this->sortedWordObjectList).end(), compWordObject);
}

// TODO: Crashes when try to do this and then redo layout
//----------------------------------------------------------------------------
// void vtkQtWordleView::ResetWordObjectsPositions()
// {
//   if (this->sortedWordObjectList.size() == 0)
//     {
//     printf("Tried to reset word objects list but EMPTY.\n");
//     return;
//     }
// 
//   for (int ii=0; ii < this->sortedWordObjectList.size(); ++ii)
//     {
//     WordObject word = this->sortedWordObjectList[ii];
// 		word.pos = this->MakeInitialPosition();
// 		word.initial_pos = word.pos;
// 		
// 		// Resetting all the rest of this also since want random orientations to change
// 		this->font->setPointSize(word.font_size);
// 		QPainterPath pathOrig;
// 		pathOrig.addText(0.0f, 0.0f, *this->font, QString(word.text.c_str()));
// 		QTransform trans;
// 		// Orientation values can take on [0,4] inclusive
// 		int flip = rand() % 4;
// 		if (flip <= (this->orientation-1))
// 			{
// 			trans.rotate(90);
// 			}
// 		word.painter_path = trans.map(pathOrig);
//  	
// 		QGraphicsPathItem* pathItem = new QGraphicsPathItem(word.painter_path);
// 		pathItem->setPen(QPen(Qt::NoPen));
// 		pathItem->setBrush(*word.color);
// 		delete word.path_item;
// 		word.path_item = pathItem;
// 		
// 		// Manually build two-deep tree right here for now...
// 		QGraphicsRectItem* rect = new QGraphicsRectItem(pathItem->boundingRect().adjusted(-this->xbuffer, -this->ybuffer, this->xbuffer, this->ybuffer));
// 		rect->setPen(QPen(Qt::NoPen));
// 		QList<QPolygonF> shapes = word.painter_path.toSubpathPolygons();
// 		for (int jj=0; jj < shapes.size(); ++jj)
// 		  {
// 			QGraphicsRectItem* subRect = new QGraphicsRectItem(shapes.at(jj).boundingRect().adjusted(-this->xbuffer,-this->ybuffer,this->xbuffer,this->ybuffer));
// 			subRect->setParentItem(rect);
// 			subRect->setPen(QPen(Qt::NoPen));
// 			}
// 		QList<QGraphicsItem *> rBchildren = word.rect_item->childItems();
// 		foreach (QGraphicsItem* item, rBchildren)
// 		  {
// 		  delete item;
// 		  }
// 		delete word.rect_item;
// 		word.rect_item = rect;
// 
// 		word.rect_item->setPos(word.pos.X(),word.pos.Y());
// 		word.path_item->setPos(word.pos.X(),word.pos.Y());	
//     }
// }

// TODO: Basically works when you redo this and then re-layout, but some
// overlaps occur that don't seem to be there on the first pass...
//----------------------------------------------------------------------------
void vtkQtWordleView::ResetWordObjectsPositions()
{
  if (this->sortedWordObjectList.size() == 0)
    {
    printf("Tried to reset word objects list but EMPTY.\n");
    return;
    }

  for (int ii=0; ii < this->sortedWordObjectList.size(); ++ii)
    {
    WordObject word = this->sortedWordObjectList[ii];
		word.pos = this->MakeInitialPosition();
		word.initial_pos = word.pos;
		
		// Resetting only positions
		word.rect_item->setPos(word.pos.X(),word.pos.Y());
		word.path_item->setPos(word.pos.X(),word.pos.Y());	
    }
}

// TODO: Need a routine that just redraws the scene with new colors
// and doesn't redo positions
//----------------------------------------------------------------------------
void vtkQtWordleView::ResetWordObjectsColors()
{
  if (this->sortedWordObjectList.size() == 0)
    {
    printf("Tried to reset word objects list but EMPTY.\n");
    return;
    }
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
  vtkUnsignedCharArray* colors = vtkUnsignedCharArray::SafeDownCast(table->GetColumnByName("vtkApplyColors color"));
  if (!colors)
    {
    printf("Colors array not vtkUnsignedCharArray\n");
    return;
    }

  unsigned char cc[4];
  for (int ii=0; ii < this->sortedWordObjectList.size(); ++ii)
    {
    WordObject word = this->sortedWordObjectList[ii];
    colors->GetTupleValue(word.original_index, cc);
    delete word.color;
    word.color = new QColor(cc[0],cc[1],cc[2],cc[3]);
    }
}

//----------------------------------------------------------------------------
bool vtkQtWordleView::HierarchicalRectCollision_B()
{
	QRectF rA = this->rectA->rect();
	QRectF rB = this->rectB->rect();
	rA.translate(this->rectA->pos());
	rB.translate(this->rectB->pos());
	double ax1 = rA.x();
	double ay1 = rA.y();
	double ax2 = ax1 + rA.width();
	double ay2 = ay1 + rA.height();
	double bx1 = rB.x();
	double by1 = rB.y();
	double bx2 = bx1 + rB.width();
	double by2 = by1 + rB.height();

	// This sequence only true for non-overlap
	if ((ax2<bx1) || (ax1>bx2) || (ay2<by1) || (ay1>by2))
	  {
		// short-circuit if not overlapping with outer rect
		return false;
		}
	else
	  {
	  // but if overlap with outer rect, check to make sure overlap with a sub-rect
		QList<QGraphicsItem *> rBchildren = this->rectB->childItems();
		const QGraphicsRectItem* gitem;
		foreach (const QGraphicsItem* item, rBchildren)
		  {
		  gitem = static_cast<const QGraphicsRectItem*>(item);
			rB = gitem->rect();
			rB.translate(this->rectB->pos());
			bx1 = rB.x();
			by1 = rB.y();
			bx2 = bx1 + rB.width();
			by2 = by1 + rB.height();
			if (!((ax2<bx1) or (ax1>bx2) or (ay2<by1) or (ay1>by2)))
				{
				// short-circuit if find overlap with any sub-rect
				return true;
				}
			}
		// return no overlaps if didn't hit any sub-rects
		return false;
		}
}

//----------------------------------------------------------------------------
void vtkQtWordleView::DoLayout()
{
	this->lastRect = this->sortedWordObjectList[0].rect_item;
	this->scene->setSceneRect(-300, -400, 900, 800);

	QRectF tmpRect = this->sortedWordObjectList[0].path_item->boundingRect();
	int word_count = std::min((int)this->sortedWordObjectList.size(), this->MaxNumberOfWords);
	bool overlap;
	for (int ii=0; ii < word_count; ++ii)
	  {
	  WordObject word = this->sortedWordObjectList[ii];
		// printf("%d\t%s\t%d\n", ii, word.text.c_str(), word.font_size);
		if (ii == 0)
			overlap = false;
		else
			overlap = true;

		while (overlap)
			{
			// Assume no overlap and collision detection turns to true if there is overlap
			overlap = false;
			// First test for overlap with last one intersected
			this->rectA = word.rect_item;
			this->rectB = this->lastRect;
			if (this->HierarchicalRectCollision_B())
				{
				overlap = true;
				}
			else
				{
				// Found that "collidingItems" was taking most of the time, so just checking all..
				for (int jj=0; jj < ii; ++jj)
					{
					this->rectB = this->sortedWordObjectList[jj].rect_item;
					if (this->HierarchicalRectCollision_B())
						{
						overlap = true;
						this->lastRect = this->rectB;
						break;
						}
					}
				}

			if (overlap)
				{
				this->UpdatePositionSpirals(&word);
				word.rect_item->setPos(word.pos.X(),word.pos.Y());
				}
			}
			
		word.rect_item->setPos(word.pos.X(),word.pos.Y());
		word.path_item->setPos(word.pos.X(),word.pos.Y());	
		this->scene->addItem(word.path_item);
		tmpRect = tmpRect.united(word.path_item->mapRectToScene(word.path_item->boundingRect()));
		// Can't get the view to update after each word is added...
		// this->ui.graphicsView.repaint()
		// this->scene.update(this->scene.sceneRect())
		}

		// Rescale to fit in view
		double adjX = (tmpRect.width()*0.05)/2.0;
		double adjY = (tmpRect.height()*0.05)/2.0;
		QRectF boundingRect = tmpRect.adjusted(-adjX, -adjY, adjX, adjY);
		this->scene->setSceneRect(boundingRect);
		this->View->fitInView(boundingRect, Qt::KeepAspectRatio);
}

//----------------------------------------------------------------------------
void vtkQtWordleView::Update()
{
  vtkDataRepresentation* rep = this->GetRepresentation();
  if (!rep)
    {
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

		this->BuildWordObjectsList();
		this->ClearGraphicsView();
		this->DoLayout();

    this->LastInputMTime = d->GetMTime();
    this->LastMTime = this->GetMTime();
    }
    
// TODO: Basically works when you redo this and then re-layout, but some
// overlaps occur that don't seem to be there on the first pass...
//   if (this->GetMTime() > this->LastMTime)
//     {
//     printf("This time changed\n");
// 		this->ClearGraphicsView();
// 		printf("Cleared scene\n");
// 		this->ResetWordObjectsPositions();
// 		printf("Reset positions\n");
// 		this->DoLayout();
// 		printf("Finished Layout\n");
// 
//     this->LastMTime = this->GetMTime();
//     }
    
// TODO: Need a routine that just redraws the scene with new colors
// and doesn't redo positions (keep track of ApplyColors Modified time)...

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

