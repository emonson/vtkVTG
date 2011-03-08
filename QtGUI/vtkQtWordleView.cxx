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

#include <QCoreApplication>
#include <QFont>
#include <QFontDatabase>
// #include <QGraphicsScene>
#include <QGraphicsView>
#include <QList>
#include <QPolygonF>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QTime>
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
#include <time.h>
// Delay for debug watching layout
#include <unistd.h>
// #include <cmath>

void Btree::AddRectItem(QGraphicsRectItem *rect_item, double min, double max, int index)
{
	bool PRINT = false;
	
	if(PRINT) printf("Entering Btree AddRectItem\n");
	if (ex_middle >= min && ex_middle <= max)
		{
		if(PRINT) printf("Adding item to Btree list\n");
		ItemsList.append(IndexedRectItem(index, rect_item));
		}
	else
		{
		// TODO: Eventually test here whether this line is too small to be
		// subdivided further, and if so, add to current ItemsList
		
		// L
		if (max < ex_middle)
			{
			if (!left)
				{
				if(PRINT) printf("Creating new Btree left branch\n");
				left = new Btree(ex_min, ex_middle);
				}
			if(PRINT) printf("Calling Btree AddRectItem for left branch\n");
			left->AddRectItem(rect_item, min, max, index);
			}
		// R
		if (min > ex_middle)
			{
			if (!right)
				{
				if(PRINT) printf("Creating new Btree right branch\n");
				right = new Btree(ex_middle, ex_max);
				}
			if(PRINT) printf("Calling Btree AddRectItem for right branch\n");
			right->AddRectItem(rect_item, min, max, index);
			}
		}
}

//----------------------
void QuadCIF::AddRectItem(QGraphicsRectItem *rect_item, int index)
{
	bool PRINT = false;
	
	if(PRINT) printf("Entering QuadCIF AddRectItem\n");
	
	QRectF rA = rect_item->rect();
	rA.translate(rect_item->pos());
	double ax1 = rA.x();
	double ax2 = ax1 + rA.width();
	double ay1 = rA.y();
	double ay2 = ay1 + rA.height();
	if(PRINT) printf("ThisQuad: x1: %3.2f xMid: %3.2f x2: %3.2f  y1: %3.2f  yMid: %3.2f y2: %3.2f\n", frame.x(), xmiddle, frame.x()+frame.height(), frame.y(), ymiddle, frame.y()+frame.height());
	if(PRINT) printf("RI x1: %3.2f x2: %3.2f  y1: %3.2f  y2: %3.2f\n", ax1, ax2, ay1, ay2);
	// Intersects xline
	if (xmiddle >= ax1 && xmiddle <= ax2)
		{
		if(PRINT) printf("Calling AddRectItem on xline\n");
		xline->AddRectItem(rect_item, ax1, ax2, index);
		}
	else if (ymiddle >= ay1 && ymiddle <= ay2)
		{
		if(PRINT) printf("Calling AddRectItem on yline\n");
		yline->AddRectItem(rect_item, ay1, ay2, index);
		}
	else
		{
		// TODO: Eventually test here to see whether this node is too small
		// to subdivide, and if so, I guess add items directly to xline->ItemsList?
		
		// UL
		if (ay1 > ymiddle && ax2 < xmiddle)
			{
			if (!UL)
				{
				if(PRINT) printf("Creating new QuadCIF UL quad\n");
				UL = new QuadCIF(QRectF(frame.x(), ymiddle, frame.width()/2.0, frame.height()/2.0));
				}
			if(PRINT) printf("Calling AddRectItem UL quad\n");
			UL->AddRectItem(rect_item, index);
			}
		// LL
		if (ay2 < ymiddle && ax2 < xmiddle)
			{
			if (!LL)
				{
				if(PRINT) printf("Creating new QuadCIF LL quad\n");
				LL = new QuadCIF(QRectF(frame.x(), frame.y(), frame.width()/2.0, frame.height()/2.0));
				}
			if(PRINT) printf("Calling AddRectItem LL quad\n");
			LL->AddRectItem(rect_item, index);
			}
		// UR
		if (ay1 > ymiddle && ax1 > xmiddle)
			{
			if (!UR)
				{
				if(PRINT) printf("Creating new QuadCIF UR quad\n");
				UR = new QuadCIF(QRectF(xmiddle, ymiddle, frame.width()/2.0, frame.height()/2.0));
				}
			if(PRINT) printf("Calling AddRectItem UR quad\n");
			UR->AddRectItem(rect_item, index);
			}
		// LR
		if (ay2 < ymiddle && ax1 > xmiddle)
			{
			if (!LR)
				{
				if(PRINT) printf("Creating new QuadCIF LR quad\n");
				LR = new QuadCIF(QRectF(xmiddle, frame.y(), frame.width()/2.0, frame.height()/2.0));
				}
			if(PRINT) printf("Calling AddRectItem LR quad\n");
			LR->AddRectItem(rect_item, index);
			}
		}
}

//------------------------
// Need to translate rect_item into proper QRectF before passing to this routine
void QuadCIFmin::AddRectItemMin(QGraphicsRectItem *rect_item, int index, QGraphicsScene* scene)
{
	bool PRINT = false;
	
	if(PRINT) printf("Entering QuadCIF AddRectItem\n");
	
	QRectF rA = rect_item->rect();
	rA.translate(rect_item->pos());
	double ax1 = rA.x();
	double ax2 = ax1 + rA.width();
	double ay1 = rA.y();
	double ay2 = ay1 + rA.height();
	if(PRINT) printf("ThisQuad: x1: %3.2f xMid: %3.2f x2: %3.2f  y1: %3.2f  yMid: %3.2f y2: %3.2f\n", frame.x(), xmiddle, frame.x()+frame.height(), frame.y(), ymiddle, frame.y()+frame.height());
	if(PRINT) printf("RI x1: %3.2f x2: %3.2f  y1: %3.2f  y2: %3.2f\n", ax1, ax2, ay1, ay2);
	// Intersects xline
	if ((xmiddle >= ax1 && xmiddle <= ax2) || (ymiddle >= ay1 && ymiddle <= ay2))
		{
		if(PRINT) printf("Calling AddRectItem to ItemsList\n");
		ItemsList.append(IndexedRectItem(index, rect_item));
		}
	else
		{
		// TODO: Eventually test here to see whether this node is too small
		// to subdivide, and if so, I guess add items directly to xline->ItemsList?
		
		// UL
		if (ay1 > ymiddle && ax2 < xmiddle)
			{
			if (!UL)
				{
				if(PRINT) printf("Creating new QuadCIFmin UL quad\n");
				UL = new QuadCIFmin(QRectF(frame.x(), ymiddle, frame.width()/2.0, frame.height()/2.0), scene);
				}
			if(PRINT) printf("Calling AddRectItem UL quad\n");
			UL->AddRectItemMin(rect_item, index, scene);
			}
		// LL
		if (ay2 < ymiddle && ax2 < xmiddle)
			{
			if (!LL)
				{
				if(PRINT) printf("Creating new QuadCIFmin LL quad\n");
				LL = new QuadCIFmin(QRectF(frame.x(), frame.y(), frame.width()/2.0, frame.height()/2.0), scene);
				}
			if(PRINT) printf("Calling AddRectItem LL quad\n");
			LL->AddRectItemMin(rect_item, index, scene);
			}
		// UR
		if (ay1 > ymiddle && ax1 > xmiddle)
			{
			if (!UR)
				{
				if(PRINT) printf("Creating new QuadCIFmin UR quad\n");
				UR = new QuadCIFmin(QRectF(xmiddle, ymiddle, frame.width()/2.0, frame.height()/2.0), scene);
				}
			if(PRINT) printf("Calling AddRectItem UR quad\n");
			UR->AddRectItemMin(rect_item, index, scene);
			}
		// LR
		if (ay2 < ymiddle && ax1 > xmiddle)
			{
			if (!LR)
				{
				if(PRINT) printf("Creating new QuadCIFmin LR quad\n");
				LR = new QuadCIFmin(QRectF(xmiddle, frame.y(), frame.width()/2.0, frame.height()/2.0), scene);
				}
			if(PRINT) printf("Calling AddRectItem LR quad\n");
			LR->AddRectItemMin(rect_item, index, scene);
			}
		}
}

//============================================================================
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
	
	this->WatchLayout = false;
	this->WatchCollision = false;
	this->WatchDelay = 0;
	
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
		// this->View->fitInView(this->scene->sceneRect(), Qt::KeepAspectRatio);
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
	double maxSize = std::max(fabs(sizeRange[0]), fabs(sizeRange[1]));
	
  unsigned char cc[4];
  
  // Doing a two-stage load for the data so that the more expensive
  // Qt path related work is only done for the needed number of words
  // but for sorting it's necessary to load all of the external data first
  for (vtkIdType ii=0; ii < terms->GetNumberOfValues(); ++ii)
    {
  	WordObject word;
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
			
    (this->sortedWordObjectList).push_back(word);
    }
  
  // Sort words according to size
  std::sort((this->sortedWordObjectList).begin(), (this->sortedWordObjectList).end(), compWordObject);

	// Second stage for more expensive operations
	int word_count = std::min((int)this->sortedWordObjectList.size(), this->MaxNumberOfWords);
  for (int ii=0; ii < word_count; ++ii)
    {
		this->sortedWordObjectList[ii].pos = this->MakeInitialPosition();
		this->sortedWordObjectList[ii].initial_pos = this->sortedWordObjectList[ii].pos;
		this->sortedWordObjectList[ii].delta = this->thedaMult*pow((float)this->sortedWordObjectList[ii].font_size, this->thedaPow);
		this->sortedWordObjectList[ii].rdelta = this->rMult*pow((float)this->sortedWordObjectList[ii].font_size, this->rPow);
		
		this->font->setPointSize(this->sortedWordObjectList[ii].font_size);
		QPainterPath pathOrig;
		pathOrig.addText(0.0f, 0.0f, *this->font, QString(this->sortedWordObjectList[ii].text.c_str()));
		QTransform trans;
		// Orientation values can take on [0,4] inclusive
		int flip = rand() % 4;
		if (flip <= (this->orientation-1))
			{
			trans.rotate(90);
			}
		this->sortedWordObjectList[ii].painter_path = trans.map(pathOrig);
	
		QGraphicsPathItem* pathItem = new QGraphicsPathItem(this->sortedWordObjectList[ii].painter_path);
		pathItem->setPen(QPen(Qt::NoPen));
		pathItem->setBrush(*this->sortedWordObjectList[ii].color);
		this->sortedWordObjectList[ii].path_item = pathItem;
		
		// Manually build two-deep tree right here for now...
		QGraphicsRectItem* rect = new QGraphicsRectItem(pathItem->boundingRect().adjusted(-this->xbuffer, -this->ybuffer, this->xbuffer, this->ybuffer));
		rect->setPen(QPen(Qt::NoPen));
		QList<QPolygonF> shapes = this->sortedWordObjectList[ii].painter_path.toSubpathPolygons();
		for (int jj=0; jj < shapes.size(); ++jj)
			{
			QGraphicsRectItem* subRect = new QGraphicsRectItem(shapes.at(jj).boundingRect().adjusted(-this->xbuffer,-this->ybuffer,this->xbuffer,this->ybuffer));
			subRect->setParentItem(rect);
			subRect->setPen(QPen(Qt::NoPen));
			}
		this->sortedWordObjectList[ii].rect_item = rect;

		this->sortedWordObjectList[ii].rect_item->setPos(this->sortedWordObjectList[ii].pos.X(),this->sortedWordObjectList[ii].pos.Y());
		this->sortedWordObjectList[ii].path_item->setPos(this->sortedWordObjectList[ii].pos.X(),this->sortedWordObjectList[ii].pos.Y());
    }
}

//----------------------------------------------------------------------------
void vtkQtWordleView::ResetOnlyWordObjectsPositions()
{
  if (this->sortedWordObjectList.size() == 0)
    {
    printf("Tried to reset word objects list but EMPTY.\n");
    return;
    }

	int word_count = std::min((int)this->sortedWordObjectList.size(), this->MaxNumberOfWords);
  for (int ii=0; ii < word_count; ++ii)
    {
		this->sortedWordObjectList[ii].pos = this->MakeInitialPosition();
		this->sortedWordObjectList[ii].initial_pos = this->sortedWordObjectList[ii].pos;
		this->sortedWordObjectList[ii].theda = 0.0;
		
		// Resetting only positions
		this->sortedWordObjectList[ii].rect_item->setPos(this->sortedWordObjectList[ii].pos.X(),this->sortedWordObjectList[ii].pos.Y());
		this->sortedWordObjectList[ii].path_item->setPos(this->sortedWordObjectList[ii].pos.X(),this->sortedWordObjectList[ii].pos.Y());	
    }
}

//----------------------------------------------------------------------------
bool vtkQtWordleView::HierarchicalRectCollision_B(QGraphicsRectItem* rectA, QGraphicsRectItem* rectB)
{
	QRectF rA = rectA->rect();
	QRectF rB = rectB->rect();
	rA.translate(rectA->pos());
	rB.translate(rectB->pos());
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
		QList<QGraphicsItem *> rBchildren = rectB->childItems();
		const QGraphicsRectItem* gitem;
		foreach (const QGraphicsItem* item, rBchildren)
		  {
		  gitem = static_cast<const QGraphicsRectItem*>(item);
			rB = gitem->rect();
			rB.translate(rectB->pos());
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
/* A procedure for finding all the items that intersect a given current_rect
	 Need to supply this code with already translated outer rectangle for the
	 current word rect_item so don't have to translate every time 
	 
	QRectF current_rect = current_rect_item->rect();
	current_rect.translate(current_rect_item->pos());
	*/

//----------------------------------------------------------------------------
int vtkQtWordleView::AllIntersectionsMin(QuadCIFmin* Tree, 
																					QGraphicsRectItem *rect_item, 
																					QRectF current_rect,
																					int last_index) 
{
	bool PRINT = false;
	bool itemCollided;
	int idxCollided = -1;

	if(PRINT) printf("Checking collisions with current ItemsList\n");
	for (int ii=0; ii < Tree->ItemsList.length(); ++ii)
		{
		if (Tree->ItemsList[ii].index == last_index)
			{
			continue;
			}

		if (this->WatchCollision && this->WatchLayout)
			{
			this->sortedWordObjectList[Tree->ItemsList[ii].index].path_item->setPen(QPen(QBrush(QColor(0,0,0)), 4.0));
			QCoreApplication::instance()->processEvents();
			usleep(this->WatchDelay);
			}
		itemCollided = this->HierarchicalRectCollision_B(rect_item, Tree->ItemsList[ii].rect_item);
		if (this->WatchCollision && this->WatchLayout)
			{
			this->sortedWordObjectList[Tree->ItemsList[ii].index].path_item->setPen(QPen(Qt::NoPen));
			QCoreApplication::instance()->processEvents();
			}
		if (itemCollided)
			{
			// Short circuit on collision
			return Tree->ItemsList[ii].index;
			}
		}
	
	/* traverse the four children */
	if (Tree->UL && IsBoundsIntersecting(Tree->UL->frame, current_rect)) 
		{
		if(PRINT) printf("Calling AllIntersectionsMin on UL\n");
		idxCollided = this->AllIntersectionsMin(Tree->UL, rect_item, current_rect, last_index);
		if (idxCollided >= 0)
			{
			// Short circuit on collision
			return idxCollided;
			}
		}
	
	if (Tree->LL && IsBoundsIntersecting(Tree->LL->frame, current_rect)) 
		{
		if(PRINT) printf("Calling AllIntersectionsMin on LL\n");
		idxCollided = this->AllIntersectionsMin(Tree->LL, rect_item, current_rect, last_index);
		if (idxCollided >= 0)
			{
			// Short circuit on collision
			return idxCollided;
			}
		}
	
	if (Tree->UR && IsBoundsIntersecting(Tree->UR->frame, current_rect))
		{
		if(PRINT) printf("Calling AllIntersectionsMin on UR\n");
		idxCollided = this->AllIntersectionsMin(Tree->UR, rect_item, current_rect, last_index);
		if (idxCollided >= 0)
			{
			// Short circuit on collision
			return idxCollided;
			}
		}
	
	if (Tree->LR && IsBoundsIntersecting(Tree->LR->frame, current_rect)) 
		{
		if(PRINT) printf("Calling AllIntersectionsMin on LR\n");
		idxCollided = this->AllIntersectionsMin(Tree->LR, rect_item, current_rect, last_index);
		if (idxCollided >= 0)
			{
			// Short circuit on collision
			return idxCollided;
			}
		}
	
	if(PRINT) printf("Returning -1 from AllIntersectionsMin\n");
	return -1;
}

//----------------------------------------------------------------------------
QList<int> vtkQtWordleView::AllIntersections(QuadCIF* Tree, QRectF current_rect) 
{
	bool PRINT = false;

	double ax1 = current_rect.x();
	double ay1 = current_rect.y();
	double ax2 = ax1 + current_rect.width();
	double ay2 = ay1 + current_rect.height();
	QList<int> idxs_list;

	if(PRINT) printf("Calling IntersectLine on xline\n");
	idxs_list.append(this->IntersectLine(Tree->xline, current_rect, ax1, ax2)); 

	if(PRINT) printf("Calling IntersectLine on xline\n");
	idxs_list.append(this->IntersectLine(Tree->yline, current_rect, ay1, ay2));
	
	/* traverse the four children */
	if (Tree->UL && IsBoundsIntersecting(Tree->UL->frame, current_rect)) 
		{
		if(PRINT) printf("Calling AllIntersections on UL\n");
		idxs_list.append(this->AllIntersections(Tree->UL, current_rect));
		}
	
	if (Tree->LL && IsBoundsIntersecting(Tree->LL->frame, current_rect)) 
		{
		if(PRINT) printf("Calling AllIntersections on UL\n");
		idxs_list.append(this->AllIntersections(Tree->LL, current_rect));
		}
	
	if (Tree->UR && IsBoundsIntersecting(Tree->UR->frame, current_rect))
		{
		if(PRINT) printf("Calling AllIntersections on UL\n");
		idxs_list.append(this->AllIntersections(Tree->UR, current_rect));
		}
	
	if (Tree->LR && IsBoundsIntersecting(Tree->LR->frame, current_rect)) 
		{
		if(PRINT) printf("Calling AllIntersections on UL\n");
		idxs_list.append(this->AllIntersections(Tree->LR, current_rect));
		}
	
	if(PRINT) printf("Returning idxs_list from AllIntersections\n");
	return idxs_list;
}

//----------------------------------------------------------------------------
QList<int> vtkQtWordleView::IntersectLine(Btree *node, QRectF current_rect, double rect_min, double rect_max) 
{
	bool PRINT = false;
	if(PRINT) printf("Entering IntersectLine\n");
	
	IndexedRectItem item;
	double ax1 = current_rect.x();
	double ay1 = current_rect.y();
	double ax2 = ax1 + current_rect.width();
	double ay2 = ay1 + current_rect.height();
	QList<int> idxs_list;
	
	for (int ii = 0; ii < node->ItemsList.length(); ++ii) 
		{
		item = node->ItemsList[ii];
		
		// if (IsFullIntersecting(item.rect_item, current_rect)) 
		// 	{ 
			if(PRINT) printf("Appending index to list\n");
			idxs_list.append(item.index); 
		// 	}
		}

	if (node->left && node->ex_middle >= rect_min)
		{
		if(PRINT) printf("Calling IntersectLine on left node\n");
		idxs_list.append(this->IntersectLine(node->left, current_rect, rect_min, rect_max));
		}
		
	if (node->right && node->ex_middle <= rect_max)
		{
		if(PRINT) printf("Calling IntersectLine on right node\n");
		idxs_list.append(this->IntersectLine(node->right, current_rect, rect_min, rect_max));
		}
	
	return idxs_list;
}

//----------------------------------------------------------------------------
/* Only test outer bounds, no children */
bool vtkQtWordleView::IsBoundsIntersecting(QRectF frame, QRectF current_rect)
{
	double ax1 = current_rect.x();
	double ay1 = current_rect.y();
	double ax2 = ax1 + current_rect.width();
	double ay2 = ay1 + current_rect.height();
	
	double bx1 = frame.x();
	double by1 = frame.y();
	double bx2 = bx1 + frame.width();
	double by2 = by1 + frame.height();

	// This sequence only true for non-overlap
	if ((ax2<bx1) || (ax1>bx2) || (ay2<by1) || (ay1>by2))
	  {
		return false;
		}
	else
		{
		return true;
		}
}

//----------------------------------------------------------------------------
/* Full intersection test with children (one or both) */
bool vtkQtWordleView::IsFullIntersecting(QGraphicsRectItem* target_item, QRectF current_rect)
{
	double ax1 = current_rect.x();
	double ay1 = current_rect.y();
	double ax2 = ax1 + current_rect.width();
	double ay2 = ay1 + current_rect.height();

	QRectF rB = target_item->rect();
	rB.translate(target_item->pos());
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
		QList<QGraphicsItem *> rBchildren = target_item->childItems();
		const QGraphicsRectItem* gitem;
		foreach (const QGraphicsItem* item, rBchildren)
		  {
		  gitem = static_cast<const QGraphicsRectItem*>(item);
			rB = gitem->rect();
			rB.translate(target_item->pos());
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
	// cout << this->sortedWordObjectList.at(0);
	// cout << this->sortedWordObjectList.at(0).rect_item->rect().x() << endl;
	// cout << this->sortedWordObjectList.at(0).path_item->boundingRect().x() << endl;
	// return;
	
	int TEST_ALL = 0;
	int TEST_QUAD = 1;
	int mode = TEST_ALL;
	int quad_fsize_cutoff = 50;
	int quad_minnum_cutoff = 8;
	int quad_maxnum_cutoff = (int)((float)this->MaxNumberOfWords * 0.5);
	bool quadtree_loaded = false;
	
	this->scene->setSceneRect(-300, -400, 900, 800);
	
	QuadCIFmin *root_node;

	QRectF tmpRect = this->sortedWordObjectList[0].path_item->boundingRect();
	int word_count = std::min((int)this->sortedWordObjectList.size(), this->MaxNumberOfWords);
	bool overlap, itemCollided;
	int idxCollided;
	
	int lastRectIndex = 0;
	
	// MAIN LOOP
	for (int ii=0; ii < word_count; ++ii)
	  {
	  // Check whether font size (normalized to 100) has dropped below threshold
	  // or sufficient number of words have been positioned,
	  // and if so, initialize and load up QuadCIF based on layout so far
	  // and switch over to Quad mode collision detection
	  if (quadtree_loaded == false && 
	  		((this->sortedWordObjectList[ii].font_size < quad_fsize_cutoff && ii > quad_minnum_cutoff)
	  		|| ii > quad_maxnum_cutoff))
	  	{
	  	double xAd = tmpRect.width()/2.0;
	  	double yAd = tmpRect.height()/2.0;
	  	QRectF quad_bounds = tmpRect.adjusted(-xAd, -yAd, xAd, yAd);
	  	root_node = new QuadCIFmin(quad_bounds, this->scene);
	  	for (int jj=0; jj < ii; ++jj)
	  		{
	  		root_node->AddRectItemMin(this->sortedWordObjectList[jj].rect_item, jj, this->scene);
	  		}
	  	mode = TEST_QUAD;
	  	quadtree_loaded = true;
	  	}
	  
		// printf("%d\t%s\t%d\n", ii, this->sortedWordObjectList[ii].text.c_str(), this->sortedWordObjectList[ii].font_size);
		if (this->WatchLayout)
			{
			this->scene->addItem(this->sortedWordObjectList[ii].path_item);
			}
		if (ii == 0)
			overlap = false;
		else
			overlap = true;

		while (overlap)
			{
			// Assume no overlap and collision detection turns to true if there is overlap
			overlap = false;
			// First test for overlap with last one intersected
			if (this->WatchCollision && this->WatchLayout)
				{
				this->sortedWordObjectList[lastRectIndex].path_item->setPen(QPen(QBrush(QColor(0,0,0)), 4.0));
				QCoreApplication::instance()->processEvents();
				usleep(this->WatchDelay);
				}
			itemCollided = this->HierarchicalRectCollision_B(this->sortedWordObjectList[ii].rect_item, this->sortedWordObjectList[lastRectIndex].rect_item);
			if (this->WatchCollision && this->WatchLayout)
				{
				this->sortedWordObjectList[lastRectIndex].path_item->setPen(QPen(Qt::NoPen));
				QCoreApplication::instance()->processEvents();
				}
			if (itemCollided)
				{
				overlap = true;
				}
			else
				{
				if (mode == TEST_QUAD)
					{
					// New method using QuadCIF tree for intersection tests
					QRectF current_rect = this->sortedWordObjectList[ii].rect_item->rect();
					current_rect.translate(this->sortedWordObjectList[ii].rect_item->pos());
					idxCollided = this->AllIntersectionsMin(root_node, this->sortedWordObjectList[ii].rect_item, current_rect, lastRectIndex);
					if (idxCollided >= 0)
						{
						overlap = true;
						lastRectIndex = idxCollided;
						}
					}
				if (mode == TEST_ALL)
					{
					// Found that "collidingItems" was taking most of the time, so just checking all..
					for (int jj=0; jj < ii; ++jj)
						{
						if (jj == lastRectIndex)
							continue;
						if (this->WatchCollision && this->WatchLayout)
							{
							this->sortedWordObjectList[jj].path_item->setPen(QPen(QBrush(QColor(0,0,0)), 4.0));
							QCoreApplication::instance()->processEvents();
							usleep(this->WatchDelay);
							}
						itemCollided = this->HierarchicalRectCollision_B(this->sortedWordObjectList[ii].rect_item, this->sortedWordObjectList[jj].rect_item);
						if (this->WatchCollision && this->WatchLayout)
							{
							this->sortedWordObjectList[jj].path_item->setPen(QPen(Qt::NoPen));
							QCoreApplication::instance()->processEvents();
							}
						if (itemCollided)
							{
							overlap = true;
							lastRectIndex = jj;
							break;
							}
						}
					}
				}

			if (overlap)
				{
				// Update position in place
				this->UpdatePositionSpirals(&this->sortedWordObjectList[ii]);
				this->sortedWordObjectList[ii].rect_item->setPos(this->sortedWordObjectList[ii].pos.X(),this->sortedWordObjectList[ii].pos.Y());
				if (this->WatchLayout)
					{
					this->sortedWordObjectList[ii].path_item->setPos(this->sortedWordObjectList[ii].pos.X(),this->sortedWordObjectList[ii].pos.Y());
					QCoreApplication::instance()->processEvents();
					usleep(this->WatchDelay);
					}
				}
			}
			
		this->sortedWordObjectList[ii].rect_item->setPos(this->sortedWordObjectList[ii].pos.X(),this->sortedWordObjectList[ii].pos.Y());
		this->sortedWordObjectList[ii].path_item->setPos(this->sortedWordObjectList[ii].pos.X(),this->sortedWordObjectList[ii].pos.Y());	
		if (!this->WatchLayout)
			{
			this->scene->addItem(this->sortedWordObjectList[ii].path_item);
			}
		// printf("\n\n* * WORD: %s * *\n", this->sortedWordObjectList[ii].text.c_str());
// 		QRectF current_rect = this->sortedWordObjectList[ii].rect_item->rect();
// 		current_rect.translate(this->sortedWordObjectList[ii].rect_item->pos());
		
		if (mode == TEST_QUAD)
			{
			// Increase QuadCIFmin size if placement will take new word out of bounds
			if (tmpRect.x() < root_node->frame.x() ||
					tmpRect.y() < root_node->frame.y() ||
					tmpRect.x() + tmpRect.width() > root_node->frame.x() + root_node->frame.width() ||
					tmpRect.y() + tmpRect.height() > root_node->frame.y() + root_node->frame.height())
				{
				printf("*** Had to increase QuadCIF size!!! ***\n");
				double xAd = tmpRect.width()/2.0;
				double yAd = tmpRect.height()/2.0;
				QRectF quad_bounds = tmpRect.adjusted(-xAd, -yAd, xAd, yAd);
				root_node = new QuadCIFmin(quad_bounds, this->scene);
				for (int jj=0; jj < ii; ++jj)
					{
					root_node->AddRectItemMin(this->sortedWordObjectList[jj].rect_item, jj, this->scene);
					}
				}
			root_node->AddRectItemMin(this->sortedWordObjectList[ii].rect_item, ii, this->scene);
			}
		tmpRect = tmpRect.united(this->sortedWordObjectList[ii].path_item->mapRectToScene(this->sortedWordObjectList[ii].path_item->boundingRect()));
		// Can't get the view to update after each word is added...
		// this->ui.graphicsView.repaint()
		// this->scene.update(this->scene.sceneRect())
		if (this->WatchLayout)
			{
			QCoreApplication::instance()->processEvents();
			}
		}

	// Rescale to fit in view
	double adjX = (tmpRect.width()*0.05)/2.0;
	double adjY = (tmpRect.height()*0.05)/2.0;
	QRectF boundingRect = tmpRect.adjusted(-adjX, -adjY, adjX, adjY);
	
	this->scene->setSceneRect(boundingRect);
	this->View->fitInView(boundingRect, Qt::KeepAspectRatio);
	
	// DEBUG
// 	QRectF firstRect = this->sortedWordObjectList[0].path_item->boundingRect();
// 	cout << "first word x: " << firstRect.x() << " w: " << firstRect.width() << " y: " << firstRect.y() << " h: " << firstRect.height() << endl;
// 	cout << "bounding   x: " << boundingRect.x() << " w: " << boundingRect.width() << " y: " << boundingRect.y() << " h: " << boundingRect.height() << endl;
	
	// delete root_node;
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
  
  // DEBUG
  QTime timer;
  timer.start();

  vtkDataObject *d = conn->GetProducer()->GetOutputDataObject(0);
  
  // If input has changed, rebuild word objects list
  if (d->GetMTime() > this->LastInputMTime)
    {
    this->DataObjectToTable->Update();
    this->ApplyColors->Update();

		this->ClearGraphicsView();
		this->BuildWordObjectsList();
		this->DoLayout();

    this->LastInputMTime = d->GetMTime();
    this->LastMTime = this->GetMTime();
    }
  
  // If only this->Modified, only reset positions (not orientation or color)
  if (this->GetMTime() > this->LastMTime)
    {
		this->ClearGraphicsView();
		this->ResetOnlyWordObjectsPositions();
		this->DoLayout();

    this->LastMTime = this->GetMTime();
    }
    
// TODO: Need a routine that just redraws the scene with new colors
// and doesn't redo positions (keep track of ApplyColors Modified time)...

	// DEBUG
	cout << timer.elapsed() << endl;

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

