/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtWordleDebugView.cxx

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

#include "vtkQtWordleDebugView.h"

#include <QCoreApplication>
#include <QFont>
#include <QFontDatabase>
// #include <QGraphicsScene>
#include <QGraphicsView>
#include <QImage>
#include <QList>
#include <QPolygonF>
#include <QPrinter>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QtSvg/QSvgGenerator>
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
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkQImageToImageSource.h"
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


//------------------------
// Need to translate rect_item into proper QRectF before passing to this routine
void vtkQtWordleDebugQuadCIF::AddRectItemMin(QGraphicsRectItem *rect_item, int index)
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
		ItemsList.append(vtkQtWordleDebugIndexedRectItem(index, rect_item));
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
				if(PRINT) printf("Creating new vtkQtWordleDebugQuadCIF UL quad\n");
				UL = new vtkQtWordleDebugQuadCIF(QRectF(frame.x(), ymiddle, frame.width()/2.0, frame.height()/2.0));
				}
			if(PRINT) printf("Calling AddRectItem UL quad\n");
			UL->AddRectItemMin(rect_item, index);
			}
		// LL
		if (ay2 < ymiddle && ax2 < xmiddle)
			{
			if (!LL)
				{
				if(PRINT) printf("Creating new vtkQtWordleDebugQuadCIF LL quad\n");
				LL = new vtkQtWordleDebugQuadCIF(QRectF(frame.x(), frame.y(), frame.width()/2.0, frame.height()/2.0));
				}
			if(PRINT) printf("Calling AddRectItem LL quad\n");
			LL->AddRectItemMin(rect_item, index);
			}
		// UR
		if (ay1 > ymiddle && ax1 > xmiddle)
			{
			if (!UR)
				{
				if(PRINT) printf("Creating new vtkQtWordleDebugQuadCIF UR quad\n");
				UR = new vtkQtWordleDebugQuadCIF(QRectF(xmiddle, ymiddle, frame.width()/2.0, frame.height()/2.0));
				}
			if(PRINT) printf("Calling AddRectItem UR quad\n");
			UR->AddRectItemMin(rect_item, index);
			}
		// LR
		if (ay2 < ymiddle && ax1 > xmiddle)
			{
			if (!LR)
				{
				if(PRINT) printf("Creating new vtkQtWordleDebugQuadCIF LR quad\n");
				LR = new vtkQtWordleDebugQuadCIF(QRectF(xmiddle, frame.y(), frame.width()/2.0, frame.height()/2.0));
				}
			if(PRINT) printf("Calling AddRectItem LR quad\n");
			LR->AddRectItemMin(rect_item, index);
			}
		}
}

//------------------------
// Need to translate rect_item into proper QRectF before passing to this routine
// DEBUG version which draws rectangles in the scene
void vtkQtWordleDebugQuadCIF::AddRectItemMin(QGraphicsRectItem *rect_item, int index, QGraphicsScene* scene)
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
		ItemsList.append(vtkQtWordleDebugIndexedRectItem(index, rect_item));
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
				if(PRINT) printf("Creating new vtkQtWordleDebugQuadCIF UL quad\n");
				UL = new vtkQtWordleDebugQuadCIF(QRectF(frame.x(), ymiddle, frame.width()/2.0, frame.height()/2.0), scene);
				}
			if(PRINT) printf("Calling AddRectItem UL quad\n");
			UL->AddRectItemMin(rect_item, index, scene);
			}
		// LL
		if (ay2 < ymiddle && ax2 < xmiddle)
			{
			if (!LL)
				{
				if(PRINT) printf("Creating new vtkQtWordleDebugQuadCIF LL quad\n");
				LL = new vtkQtWordleDebugQuadCIF(QRectF(frame.x(), frame.y(), frame.width()/2.0, frame.height()/2.0), scene);
				}
			if(PRINT) printf("Calling AddRectItem LL quad\n");
			LL->AddRectItemMin(rect_item, index, scene);
			}
		// UR
		if (ay1 > ymiddle && ax1 > xmiddle)
			{
			if (!UR)
				{
				if(PRINT) printf("Creating new vtkQtWordleDebugQuadCIF UR quad\n");
				UR = new vtkQtWordleDebugQuadCIF(QRectF(xmiddle, ymiddle, frame.width()/2.0, frame.height()/2.0), scene);
				}
			if(PRINT) printf("Calling AddRectItem UR quad\n");
			UR->AddRectItemMin(rect_item, index, scene);
			}
		// LR
		if (ay2 < ymiddle && ax1 > xmiddle)
			{
			if (!LR)
				{
				if(PRINT) printf("Creating new vtkQtWordleDebugQuadCIF LR quad\n");
				LR = new vtkQtWordleDebugQuadCIF(QRectF(xmiddle, frame.y(), frame.width()/2.0, frame.height()/2.0), scene);
				}
			if(PRINT) printf("Calling AddRectItem LR quad\n");
			LR->AddRectItemMin(rect_item, index, scene);
			}
		}
}

//============================================================================
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkQtWordleDebugView);

//----------------------------------------------------------------------------
vtkQtWordleDebugView::vtkQtWordleDebugView()
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
  
  OutputImageDataDimensions[0] = 256;
  OutputImageDataDimensions[1] = 256;
  this->QImageToImage = vtkSmartPointer<vtkQImageToImageSource>::New();

  this->bigFontSize = 100;
  this->MaxNumberOfWords = 150;
  this->orientation = vtkQtWordleDebugView::HORIZONTAL;
  this->LayoutPathShape = vtkQtWordleDebugView::CIRCULAR_PATH;
  this->WordSizePower = 1.0;
  
	this->xbuffer = 1.5;
	this->ybuffer = 1.5;
	this->randSpread = 0.1;
	
	// Archimedean spiral
	this->thetaMult = 0.75;
	this->thetaPow = 0.66667;
	this->rMult = 2.0;
	this->rPow = 0.5;
	// Square spiral
	this->dMult = 2.5;
	this->dPow = 0.80;
	
	// DEBUG
	this->WatchLayout = false;
	this->WatchCollision = false;
	this->WatchQuadTree = false;
	this->WatchDelay = 0;
	
  this->boundingRect = new QRectF(0.0, 0.0, 0.0, 0.0);
  this->font = new QFont("Adobe Caslon Pro", 12);
  this->font->setStyle(QFont::StyleNormal);
  this->font->setWeight(QFont::Bold);
  
	this->FontDatabase = new QFontDatabase();
  
  this->FieldType = vtkQtWordleDebugView::ROW_DATA;
  this->LastInputMTime = 0;
  this->LastColorMTime = 0;
  this->LastMTime = 0;
  
  srand( time(NULL) );
}

//----------------------------------------------------------------------------
vtkQtWordleDebugView::~vtkQtWordleDebugView()
{
  if(this->View)
    {
    delete this->View;
    }
}

//----------------------------------------------------------------------------
QWidget* vtkQtWordleDebugView::GetWidget()
{
  return this->View;
}

//----------------------------------------------------------------------------
void vtkQtWordleDebugView::SetFieldType(int type)
{
  this->DataObjectToTable->SetFieldType(type);
  if(this->FieldType != type)
    {
    this->FieldType = type;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkQtWordleDebugView::AddRepresentationInternal(vtkDataRepresentation* rep)
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

void vtkQtWordleDebugView::RemoveRepresentationInternal(vtkDataRepresentation* rep)
{   
  vtkAlgorithmOutput *annConn, *conn;
  conn = rep->GetInputConnection();
  annConn = rep->GetInternalAnnotationOutputPort();

  this->DataObjectToTable->RemoveInputConnection(0, conn);
  this->ApplyColors->RemoveInputConnection(1, annConn);
}

//----------------------------------------------------------------------------
void vtkQtWordleDebugView::SetFontFamily(const char* name)
{
	if (strcmp(this->font->family().toStdString().c_str(), name) != 0)
		{
		QStringList families = this->FontDatabase->families();
		if (families.contains(QString(name)))
			{
			this->font->setFamily(QString(name));
			this->GetRepresentation()->GetInputConnection()->GetProducer()->GetOutputDataObject(0)->Modified();
			}
		else
			{
			vtkDebugMacro(<< "Font family does not match a known entry in Qt database.");
			}
		}
}

//----------------------------------------------------------------------------
const char* vtkQtWordleDebugView::GetFontFamily()
{
  return this->font->family().toStdString().c_str();
}

//----------------------------------------------------------------------------
void vtkQtWordleDebugView::GetAllFontFamilies(vtkStringArray* famlies)
{
	famlies->SetName("FontFamilies");
	famlies->SetNumberOfComponents(1);
	famlies->SetNumberOfValues(this->FontDatabase->families().size());
	
	vtkIdType ii = 0;
	foreach (QString family, this->FontDatabase->families()) {
		famlies->SetValue(ii, family.toStdString());
		ii++;
		
		// foreach (QString style, this->FontDatabase->styles(family)) {
		// 	 cout << "\t" << style.toStdString() << endl;
		// }
		// cout << endl;
	}
}

//----------------------------------------------------------------------------
bool vtkQtWordleDebugView::FontFamilyExists(const char* name)
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
void vtkQtWordleDebugView::SetFontStyle(int style)
{
	if (this->font->style() != style)
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
			this->GetRepresentation()->GetInputConnection()->GetProducer()->GetOutputDataObject(0)->Modified();
			}
		}
}

//----------------------------------------------------------------------------
int vtkQtWordleDebugView::GetFontStyle()
{
  return this->font->style();
}

//----------------------------------------------------------------------------
void vtkQtWordleDebugView::SetFontWeight(int weight)
{
	if (this->font->weight() != weight)
		{
		if (weight <= 99 && weight >= 0)
			{
			this->font->setWeight(weight);
			this->GetRepresentation()->GetInputConnection()->GetProducer()->GetOutputDataObject(0)->Modified();
			}
		else
			{
			vtkDebugMacro(<< "Font weight not in correct range.");
			}
		}
}

//----------------------------------------------------------------------------
int vtkQtWordleDebugView::GetFontWeight()
{
  return this->font->weight();
}

//----------------------------------------------------------------------------
void vtkQtWordleDebugView::SetOrientation(int orientation)
{
	if (this->orientation != orientation)
		{
		if (orientation <= 4 && orientation >= 0)
			{
			this->orientation = orientation;
			this->GetRepresentation()->GetInputConnection()->GetProducer()->GetOutputDataObject(0)->Modified();
			}
		else
			{
			vtkDebugMacro(<< "Orientation not in correct range.");
			}
		}
}

//----------------------------------------------------------------------------
int vtkQtWordleDebugView::GetOrientation()
{
  return this->orientation;
}

//----------------------------------------------------------------------------
void vtkQtWordleDebugView::SetColorByArray(bool b)
{
  this->ApplyColors->SetUsePointLookupTable(b);
}

//----------------------------------------------------------------------------
bool vtkQtWordleDebugView::GetColorByArray()
{
  return this->ApplyColors->GetUsePointLookupTable();
}

//----------------------------------------------------------------------------
void vtkQtWordleDebugView::SetColorArrayName(const char* name)
{
	this->SetColorArrayNameInternal(name);
	this->ApplyColors->SetInputArrayToProcess(0, 0, 0,
		vtkDataObject::FIELD_ASSOCIATION_ROWS, name);
}

//----------------------------------------------------------------------------
const char* vtkQtWordleDebugView::GetColorArrayName()
{
  return this->GetColorArrayNameInternal();
}

//----------------------------------------------------------------------------
void vtkQtWordleDebugView::ApplyViewTheme(vtkViewTheme* theme)
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
  
  double bg[3];
  theme->GetBackgroundColor(bg);
  QColor bgColor;
  bgColor.setRgbF(bg[0],bg[1],bg[2]);
  this->scene->setBackgroundBrush(QBrush(bgColor));
}

//----------------------------------------------------------------------------
void vtkQtWordleDebugView::SetTermsArrayName(const char* name)
{
  if (QString(name) != QString(this->TermsArrayNameInternal))
  	{
  	this->SetTermsArrayNameInternal(name);
		this->GetRepresentation()->GetInputConnection()->GetProducer()->GetOutputDataObject(0)->Modified();
  	}
}

//----------------------------------------------------------------------------
const char* vtkQtWordleDebugView::GetTermsArrayName()
{
  return this->GetTermsArrayNameInternal();
}

//----------------------------------------------------------------------------
void vtkQtWordleDebugView::SetSizeArrayName(const char* name)
{
  if (QString(name) != QString(this->SizeArrayNameInternal))
  	{
  	this->SetSizeArrayNameInternal(name);
		this->GetRepresentation()->GetInputConnection()->GetProducer()->GetOutputDataObject(0)->Modified();
  	}
}

//----------------------------------------------------------------------------
const char* vtkQtWordleDebugView::GetSizeArrayName()
{
  return this->GetSizeArrayNameInternal();
}

//----------------------------------------------------------------------------
void vtkQtWordleDebugView::ClearGraphicsView()
{
	QList<QGraphicsItem*> sceneItems = this->scene->items();
	for (int ii=0; ii < sceneItems.length(); ++ii)
		{
		this->scene->removeItem(sceneItems[ii]);
		}
	this->scene->clear();
}

//----------------------------------------------------------------------------
void vtkQtWordleDebugView::ZoomToBounds()
{
	if (!this->WatchQuadTree)
		{
		this->View->fitInView(this->scene->sceneRect(), Qt::KeepAspectRatio);
		}
}

//----------------------------------------------------------------------------
QGraphicsScene* vtkQtWordleDebugView::GetScene()
{
	return this->scene;
}

//----------------------------------------------------------------------------
vtkVector2f vtkQtWordleDebugView::CartesianToPolar(vtkVector2f posArr)
{
	double r = sqrt(posArr.X()*posArr.X() + posArr.Y()*posArr.Y());
	double phi = atan2(posArr.Y(), posArr.X());
	vtkVector2f r_phi(r, phi);
	
	return r_phi;
}
	
//----------------------------------------------------------------------------
vtkVector2f vtkQtWordleDebugView::PolarToCartesian(vtkVector2f posArr)
{
	float x = posArr.X()*cos(posArr.Y());
	float y = posArr.X()*sin(posArr.Y());
	vtkVector2f result(x, y);
	
	return result;
}

//----------------------------------------------------------------------------
vtkVector2f vtkQtWordleDebugView::MakeInitialPosition()
{
	double x = (double)(this->scene->sceneRect().width())*this->randSpread*((double)rand()/(double)RAND_MAX);
	double y = (double)(this->scene->sceneRect().height())*this->randSpread*((double)rand()/(double)RAND_MAX);
	vtkVector2f result((float)x, (float)y);
	
	return result;
}

//----------------------------------------------------------------------------
void vtkQtWordleDebugView::UpdateArchPositionSpirals(vtkQtWordleDebugWordObject* word)
{
	// Updating in place
	double const Pi = 4.0 * atan(1);
	double ri = word->R0 + (word->rdelta * word->theta);
	word->theta += (word->delta * Pi) / ri;
	double r = word->R0 + word->rdelta * word->theta;
	vtkVector2f dPos = this->PolarToCartesian(vtkVector2f(r, word->theta));
	
	word->pos.SetX(dPos.X() + word->initial_pos.X());
	word->pos.SetY(dPos.Y() + word->initial_pos.Y());
}

//----------------------------------------------------------------------------
void vtkQtWordleDebugView::UpdateSquarePositionSpirals(vtkQtWordleDebugWordObject* word)
{
	// Updating in place
	if (word->flag)
		{
		float x = word->pos.GetX();
		x += word->sign * word->dist;
		word->pos.SetX(x);
		word->count += 1;
		if (word->count == word->target_count)
			{
			word->flag = false;
			word->count = 0;
			}
		}
	else
		{
		float y = word->pos.GetY();
		y += word->sign * word->dist;
		word->pos.SetY(y);
		word->count += 1;
		if (word->count == word->target_count)
			{
			word->count = 0;
			word->sign *= -1;
			word->target_count += 1;
			word->flag = true;
			}
		}
}

namespace
{
	// Compare the two word objects for vector sort
	// Want to sort in DESCENDING order, so return "greater than" rather than <
	bool compvtkQtWordleDebugWordObject(const vtkQtWordleDebugWordObject& v1, const vtkQtWordleDebugWordObject& v2)
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
vtkImageData* vtkQtWordleDebugView::GetImageData()
{
	this->Update();
	
	// Create a new QImage and fill it with the background color
	QImage* qimage = new QImage(OutputImageDataDimensions[0],
	                            OutputImageDataDimensions[1],
	                            QImage::Format_ARGB32);
	qimage->fill(this->scene->backgroundBrush().color().rgba());
	
	QPainter* painter = new QPainter(qimage);
	painter->setRenderHint(QPainter::Antialiasing);

	QRectF r_source(this->scene->sceneRect());
	QRectF r_target(qimage->rect());
	double ws = r_source.width();
	double hs = r_source.height();
	double wt = r_target.width();
	double ht = r_target.height();
	double d;
	
	// Make sure the wordle ends up in the center of the image
	// rather than the top (default if source & target rect aspects don't match)
	if (wt/ht < ws/hs)
		{
		// hs adjust
		d = (ht*(ws/wt) - hs)/2.0;
		r_source.adjust(0, -d, 0, d);
		}
	else
		{
		// ws adjust
		d = (wt*(hs/ht) - ws)/2.0;
		r_source.adjust(-d, 0, d, 0);
		}
	
	this->scene->render(painter, r_target, r_source, Qt::KeepAspectRatio);
	painter->end();
	
	this->QImageToImage->SetQImage(qimage);
	this->QImageToImage->Update();
	
	return vtkImageData::SafeDownCast(this->QImageToImage->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
// void vtkQtWordleDebugView::SaveSVG(char* filename)
// {
// 	this->Update();
// 	
// 	QSvgGenerator* svggen = new QSvgGenerator();
// 	svggen->setFileName(filename);
// 	svggen->setSize(QSize(600, 600));
// 	svggen->setViewBox(QRect(0, 0, 600, 600));
// 	svggen->setTitle("SVG Wordle");
// 	svggen->setDescription("An SVG drawing created by the vtkQtWordleDebugView");
// 	QPainter* svgPainter = new QPainter(svggen);
// 	this->scene->render(svgPainter);
// 	svgPainter->end();
// }

//----------------------------------------------------------------------------
void vtkQtWordleDebugView::SavePDF(char* filename)
{
	this->Update();
	
	// Output size with same aspect ratio as scene rect
	// limited to 7" wide or 9.5" high.
	QRectF sr = this->scene->sceneRect();
	double ph, pw;
	
	if (sr.width()/sr.height() >= (7.0/9.5))
		{
		pw = 7.0;
		ph = pw * (sr.height()/sr.width());
		}
	else
		{
		ph = 9.5;
		pw = ph * (sr.width()/sr.height());
		}
	QSizeF paper_size(pw, ph);
	
	QPrinter* printer = new QPrinter();
	printer->setOutputFormat(QPrinter::PdfFormat);
	printer->setOutputFileName(filename);
	printer->setPaperSize(paper_size, QPrinter::Inch);
	
	QPainter* pdfPainter = new QPainter(printer);
	this->scene->render(pdfPainter);
	pdfPainter->end();
}

//----------------------------------------------------------------------------
void vtkQtWordleDebugView::SaveImage(char* filename, const char* format)
{
	this->Update();
	
	// Create a new QImage and fill it with the background color
	QImage* qimage = new QImage(OutputImageDataDimensions[0],
	                            OutputImageDataDimensions[1],
	                            QImage::Format_ARGB32);
	qimage->fill(this->scene->backgroundBrush().color().rgba());
	
	QPainter* painter = new QPainter(qimage);
	painter->setRenderHint(QPainter::Antialiasing);

	QRectF r_source(this->scene->sceneRect());
	QRectF r_target(qimage->rect());
	double ws = r_source.width();
	double hs = r_source.height();
	double wt = r_target.width();
	double ht = r_target.height();
	double d;
	
	// Make sure the wordle ends up in the center of the image
	// rather than the top (default if source & target rect aspects don't match)
	if (wt/ht < ws/hs)
		{
		// hs adjust
		d = (ht*(ws/wt) - hs)/2.0;
		r_source.adjust(0, -d, 0, d);
		}
	else
		{
		// ws adjust
		d = (wt*(hs/ht) - ws)/2.0;
		r_source.adjust(-d, 0, d, 0);
		}
	
	this->scene->render(painter, r_target, r_source, Qt::KeepAspectRatio);
	painter->end();
	
	qimage->save(filename, format);
	
}

//----------------------------------------------------------------------------
void vtkQtWordleDebugView::BuildvtkQtWordleDebugWordObjectsList()
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
  
	this->sortedvtkQtWordleDebugWordObjectList.clear();
	
	double sizeRange[2];
	sizes->GetRange(sizeRange);
	double maxSize = std::max(fabs(sizeRange[0]), fabs(sizeRange[1]));
	
  unsigned char cc[4];
  
  // Doing a two-stage load for the data so that the more expensive
  // Qt path related work is only done for the needed number of words
  // but for sorting it's necessary to load all of the external data first
  for (vtkIdType ii=0; ii < terms->GetNumberOfValues(); ++ii)
    {
  	vtkQtWordleDebugWordObject word;
    word.text = terms->GetValue(ii);
    word.original_index = ii;
    word.size = fabs(sizes->GetValue(ii));
    colors->GetTupleValue(ii, cc);
    word.color = new QColor(cc[0],cc[1],cc[2],cc[3]);
		word.font_size = (int)((float)this->bigFontSize*pow((float)word.size/(float)maxSize,this->WordSizePower));
		if (word.font_size < 1)
			{
			word.font_size = 1;
			}
			
    (this->sortedvtkQtWordleDebugWordObjectList).push_back(word);
    }
  
  // Sort words according to size
  std::sort((this->sortedvtkQtWordleDebugWordObjectList).begin(), (this->sortedvtkQtWordleDebugWordObjectList).end(), compvtkQtWordleDebugWordObject);

	// Second stage for more expensive operations
	int word_count = std::min((int)this->sortedvtkQtWordleDebugWordObjectList.size(), this->MaxNumberOfWords);
  for (int ii=0; ii < word_count; ++ii)
    {
		this->sortedvtkQtWordleDebugWordObjectList[ii].pos = this->MakeInitialPosition();
		this->sortedvtkQtWordleDebugWordObjectList[ii].initial_pos = this->sortedvtkQtWordleDebugWordObjectList[ii].pos;
		// Archimedean spiral
		this->sortedvtkQtWordleDebugWordObjectList[ii].delta = this->thetaMult*pow((float)this->sortedvtkQtWordleDebugWordObjectList[ii].font_size, this->thetaPow);
		this->sortedvtkQtWordleDebugWordObjectList[ii].rdelta = this->rMult*pow((float)this->sortedvtkQtWordleDebugWordObjectList[ii].font_size, this->rPow);
		// Square spiral
		this->sortedvtkQtWordleDebugWordObjectList[ii].dist = this->dMult*pow((float)this->sortedvtkQtWordleDebugWordObjectList[ii].font_size, this->dPow);
		
		this->font->setPointSize(this->sortedvtkQtWordleDebugWordObjectList[ii].font_size);
		QPainterPath pathOrig;
		pathOrig.addText(0.0f, 0.0f, *this->font, QString(this->sortedvtkQtWordleDebugWordObjectList[ii].text.c_str()));
		QTransform trans;
		// Orientation values can take on [0,4] inclusive
		int flip = rand() % 4;
		if (flip <= (this->orientation-1))
			{
			trans.rotate(90);
			}
		this->sortedvtkQtWordleDebugWordObjectList[ii].painter_path = trans.map(pathOrig);
	
		QGraphicsPathItem* pathItem = new QGraphicsPathItem(this->sortedvtkQtWordleDebugWordObjectList[ii].painter_path);
		pathItem->setPen(QPen(Qt::NoPen));
		pathItem->setBrush(*this->sortedvtkQtWordleDebugWordObjectList[ii].color);
		this->sortedvtkQtWordleDebugWordObjectList[ii].path_item = pathItem;
		
		// Manually build two-deep tree right here for now...
		QGraphicsRectItem* rect = new QGraphicsRectItem(pathItem->boundingRect().adjusted(-this->xbuffer, -this->ybuffer, this->xbuffer, this->ybuffer));
		rect->setPen(QPen(Qt::NoPen));
		QList<QPolygonF> shapes = this->sortedvtkQtWordleDebugWordObjectList[ii].painter_path.toSubpathPolygons();
		for (int jj=0; jj < shapes.size(); ++jj)
			{
			QGraphicsRectItem* subRect = new QGraphicsRectItem(shapes.at(jj).boundingRect().adjusted(-this->xbuffer,-this->ybuffer,this->xbuffer,this->ybuffer));
			subRect->setParentItem(rect);
			subRect->setPen(QPen(Qt::NoPen));
			}
		this->sortedvtkQtWordleDebugWordObjectList[ii].rect_item = rect;

		this->sortedvtkQtWordleDebugWordObjectList[ii].rect_item->setPos(this->sortedvtkQtWordleDebugWordObjectList[ii].pos.X(),this->sortedvtkQtWordleDebugWordObjectList[ii].pos.Y());
		this->sortedvtkQtWordleDebugWordObjectList[ii].path_item->setPos(this->sortedvtkQtWordleDebugWordObjectList[ii].pos.X(),this->sortedvtkQtWordleDebugWordObjectList[ii].pos.Y());
    }
}

//----------------------------------------------------------------------------
void vtkQtWordleDebugView::ResetOnlyvtkQtWordleDebugWordObjectsPositions()
{
  if (this->sortedvtkQtWordleDebugWordObjectList.size() == 0)
    {
    printf("Tried to reset word objects list but EMPTY.\n");
    return;
    }

	int word_count = std::min((int)this->sortedvtkQtWordleDebugWordObjectList.size(), this->MaxNumberOfWords);
  for (int ii=0; ii < word_count; ++ii)
    {
		this->sortedvtkQtWordleDebugWordObjectList[ii].pos = this->MakeInitialPosition();
		this->sortedvtkQtWordleDebugWordObjectList[ii].initial_pos = this->sortedvtkQtWordleDebugWordObjectList[ii].pos;
		// Archimedean spiral
		this->sortedvtkQtWordleDebugWordObjectList[ii].theta = 0.0;
		// Square spiral
		this->sortedvtkQtWordleDebugWordObjectList[ii].flag = true;
		this->sortedvtkQtWordleDebugWordObjectList[ii].sign = 1;
		this->sortedvtkQtWordleDebugWordObjectList[ii].count = 0;
		this->sortedvtkQtWordleDebugWordObjectList[ii].target_count = 1;
		
		// Resetting only positions
		this->sortedvtkQtWordleDebugWordObjectList[ii].rect_item->setPos(this->sortedvtkQtWordleDebugWordObjectList[ii].pos.X(),this->sortedvtkQtWordleDebugWordObjectList[ii].pos.Y());
		this->sortedvtkQtWordleDebugWordObjectList[ii].path_item->setPos(this->sortedvtkQtWordleDebugWordObjectList[ii].pos.X(),this->sortedvtkQtWordleDebugWordObjectList[ii].pos.Y());	
    }
}

//----------------------------------------------------------------------------
void vtkQtWordleDebugView::ResetOnlyvtkQtWordleDebugWordObjectsColors()
{
  if (this->sortedvtkQtWordleDebugWordObjectList.size() == 0)
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
  int orig_idx;
  
	int word_count = std::min((int)this->sortedvtkQtWordleDebugWordObjectList.size(), this->MaxNumberOfWords);
  for (int ii=0; ii < word_count; ++ii)
    {
    orig_idx = this->sortedvtkQtWordleDebugWordObjectList[ii].original_index;
    colors->GetTupleValue(orig_idx, cc);
    delete this->sortedvtkQtWordleDebugWordObjectList[ii].color;
    this->sortedvtkQtWordleDebugWordObjectList[ii].color = new QColor(cc[0],cc[1],cc[2],cc[3]);
    this->sortedvtkQtWordleDebugWordObjectList[ii].path_item->setBrush(*this->sortedvtkQtWordleDebugWordObjectList[ii].color);
    }
}

//----------------------------------------------------------------------------
bool vtkQtWordleDebugView::HierarchicalRectCollision_B(QGraphicsRectItem* rectA, QGraphicsRectItem* rectB)
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
	 current word rect_item so don't have to translate the position every time 
	 
	QRectF current_rect = current_rect_item->rect();
	current_rect.translate(current_rect_item->pos());
	*/
int vtkQtWordleDebugView::AllIntersectionsMin(vtkQtWordleDebugQuadCIF* Tree, 
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
			this->sortedvtkQtWordleDebugWordObjectList[Tree->ItemsList[ii].index].path_item->setPen(QPen(QBrush(QColor(0,0,0)), 4.0));
			QCoreApplication::instance()->processEvents();
			usleep(this->WatchDelay);
			}
		itemCollided = this->HierarchicalRectCollision_B(rect_item, Tree->ItemsList[ii].rect_item);
		if (this->WatchCollision && this->WatchLayout)
			{
			this->sortedvtkQtWordleDebugWordObjectList[Tree->ItemsList[ii].index].path_item->setPen(QPen(Qt::NoPen));
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
/* Only test outer bounds, no children */
bool vtkQtWordleDebugView::IsBoundsIntersecting(QRectF frame, QRectF current_rect)
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
void vtkQtWordleDebugView::DoHybridLayout()
{
	int TEST_ALL = 0;
	int TEST_QUAD = 1;
	int mode = TEST_ALL;
	int quad_fsize_cutoff = 50;
	int quad_minnum_cutoff = 8;
	int quad_maxnum_cutoff = (int)((float)this->MaxNumberOfWords * 0.5);
	bool quadtree_loaded = false;
	double quad_inc_factor = 0.25;
	
	this->scene->setSceneRect(-300, -400, 900, 800);
	
	vtkQtWordleDebugQuadCIF *root_node;

	QRectF tmpRect = this->sortedvtkQtWordleDebugWordObjectList[0].path_item->boundingRect();
	int word_count = std::min((int)this->sortedvtkQtWordleDebugWordObjectList.size(), this->MaxNumberOfWords);
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
	  		((this->sortedvtkQtWordleDebugWordObjectList[ii].font_size < quad_fsize_cutoff && ii > quad_minnum_cutoff)
	  		|| ii > quad_maxnum_cutoff))
	  	{
	  	double xAd = tmpRect.width() * quad_inc_factor;
	  	double yAd = tmpRect.height() * quad_inc_factor;
	  	QRectF quad_bounds = tmpRect.adjusted(-xAd, -yAd, xAd, yAd);
			if (this->WatchQuadTree)
				{
	  		root_node = new vtkQtWordleDebugQuadCIF(quad_bounds, this->scene);
				}
			else
				{
	  		root_node = new vtkQtWordleDebugQuadCIF(quad_bounds);
				}
	  	for (int jj=0; jj < ii; ++jj)
	  		{
	  		if (this->WatchQuadTree)
	  			{
	  			root_node->AddRectItemMin(this->sortedvtkQtWordleDebugWordObjectList[jj].rect_item, jj, this->scene);
	  			}
	  		else
	  			{
	  			root_node->AddRectItemMin(this->sortedvtkQtWordleDebugWordObjectList[jj].rect_item, jj);
	  			}
	  		}
	  	mode = TEST_QUAD;
	  	quadtree_loaded = true;
	  	}
	  
		if (this->WatchLayout)
			{
			this->scene->addItem(this->sortedvtkQtWordleDebugWordObjectList[ii].path_item);
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
				this->sortedvtkQtWordleDebugWordObjectList[lastRectIndex].path_item->setPen(QPen(QBrush(QColor(0,0,0)), 4.0));
				QCoreApplication::instance()->processEvents();
				usleep(this->WatchDelay);
				}
			itemCollided = this->HierarchicalRectCollision_B(this->sortedvtkQtWordleDebugWordObjectList[ii].rect_item, this->sortedvtkQtWordleDebugWordObjectList[lastRectIndex].rect_item);
			if (this->WatchCollision && this->WatchLayout)
				{
				this->sortedvtkQtWordleDebugWordObjectList[lastRectIndex].path_item->setPen(QPen(Qt::NoPen));
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
					// Using QuadCIF tree for intersection tests
					QRectF current_rect = this->sortedvtkQtWordleDebugWordObjectList[ii].rect_item->rect();
					current_rect.translate(this->sortedvtkQtWordleDebugWordObjectList[ii].rect_item->pos());
					idxCollided = this->AllIntersectionsMin(root_node, this->sortedvtkQtWordleDebugWordObjectList[ii].rect_item, current_rect, lastRectIndex);
					if (idxCollided >= 0)
						{
						overlap = true;
						lastRectIndex = idxCollided;
						}
					}
				if (mode == TEST_ALL)
					{
					// Checking all words that have already been placed
					for (int jj=0; jj < ii; ++jj)
						{
						if (jj == lastRectIndex)
							continue;
						if (this->WatchCollision && this->WatchLayout)
							{
							this->sortedvtkQtWordleDebugWordObjectList[jj].path_item->setPen(QPen(QBrush(QColor(0,0,0)), 4.0));
							QCoreApplication::instance()->processEvents();
							usleep(this->WatchDelay);
							}
						itemCollided = this->HierarchicalRectCollision_B(this->sortedvtkQtWordleDebugWordObjectList[ii].rect_item, this->sortedvtkQtWordleDebugWordObjectList[jj].rect_item);
						if (this->WatchCollision && this->WatchLayout)
							{
							this->sortedvtkQtWordleDebugWordObjectList[jj].path_item->setPen(QPen(Qt::NoPen));
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
				// Update word position in place
				if (this->LayoutPathShape == vtkQtWordleDebugView::CIRCULAR_PATH)
					{
					this->UpdateArchPositionSpirals(&this->sortedvtkQtWordleDebugWordObjectList[ii]);
					}
				else
					{
					this->UpdateSquarePositionSpirals(&this->sortedvtkQtWordleDebugWordObjectList[ii]);
					}
				this->sortedvtkQtWordleDebugWordObjectList[ii].rect_item->setPos(this->sortedvtkQtWordleDebugWordObjectList[ii].pos.X(),this->sortedvtkQtWordleDebugWordObjectList[ii].pos.Y());
				if (this->WatchLayout)
					{
					this->sortedvtkQtWordleDebugWordObjectList[ii].path_item->setPos(this->sortedvtkQtWordleDebugWordObjectList[ii].pos.X(),this->sortedvtkQtWordleDebugWordObjectList[ii].pos.Y());
					QCoreApplication::instance()->processEvents();
					usleep(this->WatchDelay);
					}
				}
			}
			
		this->sortedvtkQtWordleDebugWordObjectList[ii].rect_item->setPos(this->sortedvtkQtWordleDebugWordObjectList[ii].pos.X(),this->sortedvtkQtWordleDebugWordObjectList[ii].pos.Y());
		this->sortedvtkQtWordleDebugWordObjectList[ii].path_item->setPos(this->sortedvtkQtWordleDebugWordObjectList[ii].pos.X(),this->sortedvtkQtWordleDebugWordObjectList[ii].pos.Y());	
		if (!this->WatchLayout)
			{
			this->scene->addItem(this->sortedvtkQtWordleDebugWordObjectList[ii].path_item);
			}
		
		if (mode == TEST_QUAD)
			{
			// Increase vtkQtWordleDebugQuadCIF size if placement will take new word out of bounds
			if (tmpRect.x() < root_node->frame.x() ||
					tmpRect.y() < root_node->frame.y() ||
					tmpRect.x() + tmpRect.width() > root_node->frame.x() + root_node->frame.width() ||
					tmpRect.y() + tmpRect.height() > root_node->frame.y() + root_node->frame.height())
				{
				// printf("*** Had to increase QuadCIF size!!! ***\n");
				double xAd = tmpRect.width() * quad_inc_factor;
				double yAd = tmpRect.height() * quad_inc_factor;
				QRectF quad_bounds = tmpRect.adjusted(-xAd, -yAd, xAd, yAd);
				if (this->WatchQuadTree)
					{
					root_node = new vtkQtWordleDebugQuadCIF(quad_bounds, this->scene);
					}
				else
					{
					root_node = new vtkQtWordleDebugQuadCIF(quad_bounds);
					}
				for (int jj=0; jj < ii; ++jj)
					{
					if (this->WatchQuadTree)
						{
						root_node->AddRectItemMin(this->sortedvtkQtWordleDebugWordObjectList[jj].rect_item, jj, this->scene);
						}
					else
						{
						root_node->AddRectItemMin(this->sortedvtkQtWordleDebugWordObjectList[jj].rect_item, jj);
						}
					}
				}
			// Add current item to the vtkQtWordleDebugQuadCIF tree
			if (this->WatchQuadTree)
				{
				root_node->AddRectItemMin(this->sortedvtkQtWordleDebugWordObjectList[ii].rect_item, ii, this->scene);
				}
			else
				{
				root_node->AddRectItemMin(this->sortedvtkQtWordleDebugWordObjectList[ii].rect_item, ii);
				}
			}
		tmpRect = tmpRect.united(this->sortedvtkQtWordleDebugWordObjectList[ii].path_item->mapRectToScene(this->sortedvtkQtWordleDebugWordObjectList[ii].path_item->boundingRect()));
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
	
}

//----------------------------------------------------------------------------
void vtkQtWordleDebugView::DoCheckAllLayout()
{
	this->scene->setSceneRect(-300, -400, 900, 800);
	QRectF tmpRect = this->sortedvtkQtWordleDebugWordObjectList[0].path_item->boundingRect();
	int word_count = std::min((int)this->sortedvtkQtWordleDebugWordObjectList.size(), this->MaxNumberOfWords);
	bool overlap, itemCollided;
	int idxCollided;
	
	int lastRectIndex = 0;
	
	// MAIN LOOP
	for (int ii=0; ii < word_count; ++ii)
	  {
		if (ii == 0)
			overlap = false;
		else
			overlap = true;

		while (overlap)
			{
			// Assume no overlap and collision detection turns to true if there is overlap
			overlap = false;
			// First test for overlap with last one intersected
			itemCollided = this->HierarchicalRectCollision_B(this->sortedvtkQtWordleDebugWordObjectList[ii].rect_item, this->sortedvtkQtWordleDebugWordObjectList[lastRectIndex].rect_item);
			if (itemCollided)
				{
				overlap = true;
				}
			else
				{
				// Checking all words that have already been placed
				for (int jj=0; jj < ii; ++jj)
					{
					if (jj == lastRectIndex)
						continue;
					itemCollided = this->HierarchicalRectCollision_B(this->sortedvtkQtWordleDebugWordObjectList[ii].rect_item, this->sortedvtkQtWordleDebugWordObjectList[jj].rect_item);
					if (itemCollided)
						{
						overlap = true;
						lastRectIndex = jj;
						break;
						}
					}
				}

			if (overlap)
				{
				// Update word position in place
				if (this->LayoutPathShape == vtkQtWordleDebugView::CIRCULAR_PATH)
					{
					this->UpdateArchPositionSpirals(&this->sortedvtkQtWordleDebugWordObjectList[ii]);
					}
				else
					{
					this->UpdateSquarePositionSpirals(&this->sortedvtkQtWordleDebugWordObjectList[ii]);
					}
				this->sortedvtkQtWordleDebugWordObjectList[ii].rect_item->setPos(this->sortedvtkQtWordleDebugWordObjectList[ii].pos.X(),this->sortedvtkQtWordleDebugWordObjectList[ii].pos.Y());
				}
			}
			
		this->sortedvtkQtWordleDebugWordObjectList[ii].rect_item->setPos(this->sortedvtkQtWordleDebugWordObjectList[ii].pos.X(),this->sortedvtkQtWordleDebugWordObjectList[ii].pos.Y());
		this->sortedvtkQtWordleDebugWordObjectList[ii].path_item->setPos(this->sortedvtkQtWordleDebugWordObjectList[ii].pos.X(),this->sortedvtkQtWordleDebugWordObjectList[ii].pos.Y());	
		this->scene->addItem(this->sortedvtkQtWordleDebugWordObjectList[ii].path_item);
		
		tmpRect = tmpRect.united(this->sortedvtkQtWordleDebugWordObjectList[ii].path_item->mapRectToScene(this->sortedvtkQtWordleDebugWordObjectList[ii].path_item->boundingRect()));
		}

	// Rescale to fit in view
	double adjX = (tmpRect.width()*0.05)/2.0;
	double adjY = (tmpRect.height()*0.05)/2.0;
	QRectF boundingRect = tmpRect.adjusted(-adjX, -adjY, adjX, adjY);
	
	this->scene->setSceneRect(boundingRect);
	this->View->fitInView(boundingRect, Qt::KeepAspectRatio);
	
}

//----------------------------------------------------------------------------
void vtkQtWordleDebugView::RedrawWithSameLayout()
{
	this->scene->setSceneRect(-300, -400, 900, 800);
	QRectF tmpRect = this->sortedvtkQtWordleDebugWordObjectList[0].path_item->boundingRect();
	int word_count = std::min((int)this->sortedvtkQtWordleDebugWordObjectList.size(), this->MaxNumberOfWords);

	// MAIN LOOP
	for (int ii=0; ii < word_count; ++ii)
	  {
		this->scene->addItem(this->sortedvtkQtWordleDebugWordObjectList[ii].path_item);
		tmpRect = tmpRect.united(this->sortedvtkQtWordleDebugWordObjectList[ii].path_item->mapRectToScene(this->sortedvtkQtWordleDebugWordObjectList[ii].path_item->boundingRect()));
		}

	// Rescale to fit in view
	double adjX = (tmpRect.width()*0.05)/2.0;
	double adjY = (tmpRect.height()*0.05)/2.0;
	QRectF boundingRect = tmpRect.adjusted(-adjX, -adjY, adjX, adjY);
	
	this->scene->setSceneRect(boundingRect);
	this->View->fitInView(boundingRect, Qt::KeepAspectRatio);
}

//----------------------------------------------------------------------------
void vtkQtWordleDebugView::Update()
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
		this->BuildvtkQtWordleDebugWordObjectsList();
		this->DoHybridLayout();
		// this->DoCheckAllLayout();

    this->LastInputMTime = d->GetMTime();
    this->LastColorMTime = this->ApplyColors->GetMTime();
    this->LastMTime = this->GetMTime();
    }
  
  // If only Colors Modified, don't even reset positions
  if (this->ApplyColors->GetMTime() > this->LastColorMTime)
    {
		this->ClearGraphicsView();
		this->ResetOnlyvtkQtWordleDebugWordObjectsColors();
		this->RedrawWithSameLayout();

    this->LastColorMTime = this->ApplyColors->GetMTime();
    this->LastMTime = this->GetMTime();
    }
    
  // If only this->Modified, only reset positions (not orientation or color)
  if (this->GetMTime() > this->LastMTime)
    {
		this->ClearGraphicsView();
		this->ResetOnlyvtkQtWordleDebugWordObjectsPositions();
		this->DoHybridLayout();
		// this->DoCheckAllLayout();

    this->LastMTime = this->GetMTime();
    }
    
	// DEBUG
	cout << timer.elapsed() << endl;

  this->View->update();
}

//----------------------------------------------------------------------------
void vtkQtWordleDebugView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

