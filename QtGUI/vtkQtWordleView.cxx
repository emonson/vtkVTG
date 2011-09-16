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

#include "vtkAbstractArray.h"
#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkApplyColors.h"
#include "vtkDataObjectToTable.h"
#include "vtkDataRepresentation.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkImageGaussianSmooth.h"
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

//------------------------
// Need to translate rect_item into proper QRectF before passing to this routine
void vtkQtWordleQuadCIF::AddRectItemMin(QGraphicsRectItem *rect_item, int index)
{
	QRectF rA = rect_item->rect();
	rA.translate(rect_item->pos());
	double ax1 = rA.x();
	double ax2 = ax1 + rA.width();
	double ay1 = rA.y();
	double ay2 = ay1 + rA.height();

	// Intersects xline
	if ((xmiddle >= ax1 && xmiddle <= ax2) || (ymiddle >= ay1 && ymiddle <= ay2))
		{
		ItemsList.append(vtkQtWordleIndexedRectItem(index, rect_item));
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
				UL = new vtkQtWordleQuadCIF(QRectF(frame.x(), ymiddle, frame.width()/2.0, frame.height()/2.0));
				}
			UL->AddRectItemMin(rect_item, index);
			}
		// LL
		if (ay2 < ymiddle && ax2 < xmiddle)
			{
			if (!LL)
				{
				LL = new vtkQtWordleQuadCIF(QRectF(frame.x(), frame.y(), frame.width()/2.0, frame.height()/2.0));
				}
			LL->AddRectItemMin(rect_item, index);
			}
		// UR
		if (ay1 > ymiddle && ax1 > xmiddle)
			{
			if (!UR)
				{
				UR = new vtkQtWordleQuadCIF(QRectF(xmiddle, ymiddle, frame.width()/2.0, frame.height()/2.0));
				}
			UR->AddRectItemMin(rect_item, index);
			}
		// LR
		if (ay2 < ymiddle && ax1 > xmiddle)
			{
			if (!LR)
				{
				LR = new vtkQtWordleQuadCIF(QRectF(xmiddle, frame.y(), frame.width()/2.0, frame.height()/2.0));
				}
			LR->AddRectItemMin(rect_item, index);
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
  
  OutputImageDataDimensions[0] = 256;
  OutputImageDataDimensions[1] = 256;
  this->QImageToImage = vtkSmartPointer<vtkQImageToImageSource>::New();
  this->ImageGaussSmooth = vtkSmartPointer<vtkImageGaussianSmooth>::New();
  this->ImageGaussSmooth->SetInputConnection(this->QImageToImage->GetOutputPort(0));
  this->ImageGaussSmooth->SetDimensionality(2);
  this->ImageGaussSmooth->SetStandardDeviations(1.0, 1.0, 1.0);

  this->bigFontSize = 100;
  this->MaxNumberOfWords = 150;
  this->orientation = vtkQtWordleView::HORIZONTAL;
  this->LayoutPathShape = vtkQtWordleView::CIRCULAR_PATH;
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
	
  this->boundingRect = new QRectF(0.0, 0.0, 0.0, 0.0);
  this->font = new QFont("Adobe Caslon Pro", 12);
  this->font->setStyle(QFont::StyleNormal);
  this->font->setWeight(QFont::Bold);
  
	this->FontDatabase = new QFontDatabase();
// 	if (this->FontDatabase->supportsThreadedFontRendering())
// 		printf("Threaded font rendering supported!!!\n");
// 	else
// 		printf("Font rendering can't be done outside GUI thread! :(\n");
  
  this->FieldType = vtkQtWordleView::ROW_DATA;
  this->LastInputMTime = 0;
  this->LastColorMTime = 0;
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
void vtkQtWordleView::SetRandomSeed(unsigned int seed)
{
  srand(seed);
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
void vtkQtWordleView::SetFontFamily(const char* name)
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
const char* vtkQtWordleView::GetFontFamily()
{
  return this->font->family().toStdString().c_str();
}

//----------------------------------------------------------------------------
void vtkQtWordleView::GetAllFontFamilies(vtkStringArray* famlies)
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
int vtkQtWordleView::GetFontStyle()
{
  return this->font->style();
}

//----------------------------------------------------------------------------
void vtkQtWordleView::SetFontWeight(int weight)
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
int vtkQtWordleView::GetFontWeight()
{
  return this->font->weight();
}

//----------------------------------------------------------------------------
void vtkQtWordleView::SetOrientation(int orientation)
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
int vtkQtWordleView::GetOrientation()
{
  return this->orientation;
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
  
  double bg[3];
  theme->GetBackgroundColor(bg);
  QColor bgColor;
  bgColor.setRgbF(bg[0],bg[1],bg[2]);
  this->scene->setBackgroundBrush(QBrush(bgColor));
}

//----------------------------------------------------------------------------
void vtkQtWordleView::SetTermsArrayName(const char* name)
{
  if (QString(name) != QString(this->TermsArrayNameInternal))
  	{
  	this->SetTermsArrayNameInternal(name);
		this->GetRepresentation()->GetInputConnection()->GetProducer()->GetOutputDataObject(0)->Modified();
  	}
}

//----------------------------------------------------------------------------
const char* vtkQtWordleView::GetTermsArrayName()
{
  return this->GetTermsArrayNameInternal();
}

//----------------------------------------------------------------------------
void vtkQtWordleView::SetSizeArrayName(const char* name)
{
  if (QString(name) != QString(this->SizeArrayNameInternal))
  	{
  	this->SetSizeArrayNameInternal(name);
		this->GetRepresentation()->GetInputConnection()->GetProducer()->GetOutputDataObject(0)->Modified();
  	}
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
	double r = sqrt(posArr.X()*posArr.X() + posArr.Y()*posArr.Y());
	double phi = atan2(posArr.Y(), posArr.X());
	vtkVector2f r_phi(r, phi);
	
	return r_phi;
}
	
//----------------------------------------------------------------------------
vtkVector2f vtkQtWordleView::PolarToCartesian(vtkVector2f posArr)
{
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
void vtkQtWordleView::UpdateArchPositionSpirals(vtkQtWordleWordObject* word)
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
void vtkQtWordleView::UpdateSquarePositionSpirals(vtkQtWordleWordObject* word)
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
	bool compvtkQtWordleWordObject(const vtkQtWordleWordObject& v1, const vtkQtWordleWordObject& v2)
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
vtkImageData* vtkQtWordleView::GetImageData(bool antialias = false)
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
	this->ImageGaussSmooth->Update();
	
	if (antialias)
	{
		return vtkImageData::SafeDownCast(this->ImageGaussSmooth->GetOutputDataObject(0));
	}
	else
	{
		return vtkImageData::SafeDownCast(this->QImageToImage->GetOutputDataObject(0));
	}
}

//----------------------------------------------------------------------------
// void vtkQtWordleView::SaveSVG(char* filename)
// {
// 	this->Update();
// 	
// 	QSvgGenerator* svggen = new QSvgGenerator();
// 	svggen->setFileName(filename);
// 	svggen->setSize(QSize(600, 600));
// 	svggen->setViewBox(QRect(0, 0, 600, 600));
// 	svggen->setTitle("SVG Wordle");
// 	svggen->setDescription("An SVG drawing created by the vtkQtWordleView");
// 	QPainter* svgPainter = new QPainter(svggen);
// 	this->scene->render(svgPainter);
// 	svgPainter->end();
// }

//----------------------------------------------------------------------------
void vtkQtWordleView::SavePDF(char* filename)
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
void vtkQtWordleView::SaveImage(char* filename, const char* format)
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
void vtkQtWordleView::BuildvtkQtWordleWordObjectsList()
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
    vtkDebugMacro(<< "Terms array not vtkStringArray");
    return;
    }
  vtkUnsignedCharArray* colors = vtkUnsignedCharArray::SafeDownCast(table->GetColumnByName("vtkApplyColors color"));
  if (!colors)
    {
    vtkDebugMacro(<< "Colors array not vtkUnsignedCharArray");
    return;
    }
  vtkDoubleArray* sizes = vtkDoubleArray::SafeDownCast(table->GetColumnByName(this->SizeArrayNameInternal));
  if (!sizes)
    {
    vtkDebugMacro(<< "Size array not vtkDoubleArray");
    return;
    }
  
	this->sortedvtkQtWordleWordObjectList.clear();
	
	double sizeRange[2];
	sizes->GetRange(sizeRange);
	double maxSize = std::max(fabs(sizeRange[0]), fabs(sizeRange[1]));
	
  unsigned char cc[4];
  
  // Doing a two-stage load for the data so that the more expensive
  // Qt path related work is only done for the needed number of words
  // but for sorting it's necessary to load all of the external data first
  for (vtkIdType ii=0; ii < terms->GetNumberOfValues(); ++ii)
    {
  	vtkQtWordleWordObject word;
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
			
    (this->sortedvtkQtWordleWordObjectList).push_back(word);
    }
  
  // Sort words according to size
  std::sort((this->sortedvtkQtWordleWordObjectList).begin(), (this->sortedvtkQtWordleWordObjectList).end(), compvtkQtWordleWordObject);

	// Second stage for more expensive operations
	int word_count = std::min((int)this->sortedvtkQtWordleWordObjectList.size(), this->MaxNumberOfWords);
  for (int ii=0; ii < word_count; ++ii)
    {
		this->sortedvtkQtWordleWordObjectList[ii].pos = this->MakeInitialPosition();
		this->sortedvtkQtWordleWordObjectList[ii].initial_pos = this->sortedvtkQtWordleWordObjectList[ii].pos;
		// Archimedean spiral
		this->sortedvtkQtWordleWordObjectList[ii].delta = this->thetaMult*pow((float)this->sortedvtkQtWordleWordObjectList[ii].font_size, this->thetaPow);
		this->sortedvtkQtWordleWordObjectList[ii].rdelta = this->rMult*pow((float)this->sortedvtkQtWordleWordObjectList[ii].font_size, this->rPow);
		// Square spiral
		this->sortedvtkQtWordleWordObjectList[ii].dist = this->dMult*pow((float)this->sortedvtkQtWordleWordObjectList[ii].font_size, this->dPow);
		
		this->font->setPointSize(this->sortedvtkQtWordleWordObjectList[ii].font_size);
		QPainterPath pathOrig;
		pathOrig.addText(0.0f, 0.0f, *this->font, QString(this->sortedvtkQtWordleWordObjectList[ii].text.c_str()));
		QTransform trans;
		// Orientation values can take on [0,4] inclusive
		int flip = rand() % 4;
		if (flip <= (this->orientation-1))
			{
			trans.rotate(90);
			}
		this->sortedvtkQtWordleWordObjectList[ii].painter_path = trans.map(pathOrig);
	
		QGraphicsPathItem* pathItem = new QGraphicsPathItem(this->sortedvtkQtWordleWordObjectList[ii].painter_path);
		pathItem->setPen(QPen(Qt::NoPen));
		pathItem->setBrush(*this->sortedvtkQtWordleWordObjectList[ii].color);
		this->sortedvtkQtWordleWordObjectList[ii].path_item = pathItem;
		
		// Manually build two-deep tree right here for now...
		QGraphicsRectItem* rect = new QGraphicsRectItem(pathItem->boundingRect().adjusted(-this->xbuffer, -this->ybuffer, this->xbuffer, this->ybuffer));
		rect->setPen(QPen(Qt::NoPen));
		QList<QPolygonF> shapes = this->sortedvtkQtWordleWordObjectList[ii].painter_path.toSubpathPolygons();
		for (int jj=0; jj < shapes.size(); ++jj)
			{
			QGraphicsRectItem* subRect = new QGraphicsRectItem(shapes.at(jj).boundingRect().adjusted(-this->xbuffer,-this->ybuffer,this->xbuffer,this->ybuffer));
			subRect->setParentItem(rect);
			subRect->setPen(QPen(Qt::NoPen));
			}
		this->sortedvtkQtWordleWordObjectList[ii].rect_item = rect;

		this->sortedvtkQtWordleWordObjectList[ii].rect_item->setPos(this->sortedvtkQtWordleWordObjectList[ii].pos.X(),this->sortedvtkQtWordleWordObjectList[ii].pos.Y());
		this->sortedvtkQtWordleWordObjectList[ii].path_item->setPos(this->sortedvtkQtWordleWordObjectList[ii].pos.X(),this->sortedvtkQtWordleWordObjectList[ii].pos.Y());
    }
}

//----------------------------------------------------------------------------
void vtkQtWordleView::ResetOnlyvtkQtWordleWordObjectsPositions()
{
  if (this->sortedvtkQtWordleWordObjectList.size() == 0)
    {
    vtkDebugMacro(<< "Tried to reset word objects list but EMPTY.");
    return;
    }

	int word_count = std::min((int)this->sortedvtkQtWordleWordObjectList.size(), this->MaxNumberOfWords);
  for (int ii=0; ii < word_count; ++ii)
    {
		this->sortedvtkQtWordleWordObjectList[ii].pos = this->MakeInitialPosition();
		this->sortedvtkQtWordleWordObjectList[ii].initial_pos = this->sortedvtkQtWordleWordObjectList[ii].pos;
		// Archimedean spiral
		this->sortedvtkQtWordleWordObjectList[ii].theta = 0.0;
		// Square spiral
		this->sortedvtkQtWordleWordObjectList[ii].flag = true;
		this->sortedvtkQtWordleWordObjectList[ii].sign = 1;
		this->sortedvtkQtWordleWordObjectList[ii].count = 0;
		this->sortedvtkQtWordleWordObjectList[ii].target_count = 1;
		
		// Resetting only positions
		this->sortedvtkQtWordleWordObjectList[ii].rect_item->setPos(this->sortedvtkQtWordleWordObjectList[ii].pos.X(),this->sortedvtkQtWordleWordObjectList[ii].pos.Y());
		this->sortedvtkQtWordleWordObjectList[ii].path_item->setPos(this->sortedvtkQtWordleWordObjectList[ii].pos.X(),this->sortedvtkQtWordleWordObjectList[ii].pos.Y());	
    }
}

//----------------------------------------------------------------------------
void vtkQtWordleView::ResetOnlyvtkQtWordleWordObjectsColors()
{
  if (this->sortedvtkQtWordleWordObjectList.size() == 0)
    {
    vtkDebugMacro(<< "Tried to reset word objects list but EMPTY.");
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
    vtkDebugMacro(<< "Colors array not vtkUnsignedCharArray");
    return;
    }

  unsigned char cc[4];
  int orig_idx;
  
	int word_count = std::min((int)this->sortedvtkQtWordleWordObjectList.size(), this->MaxNumberOfWords);
  for (int ii=0; ii < word_count; ++ii)
    {
    orig_idx = this->sortedvtkQtWordleWordObjectList[ii].original_index;
    colors->GetTupleValue(orig_idx, cc);
    delete this->sortedvtkQtWordleWordObjectList[ii].color;
    this->sortedvtkQtWordleWordObjectList[ii].color = new QColor(cc[0],cc[1],cc[2],cc[3]);
    this->sortedvtkQtWordleWordObjectList[ii].path_item->setBrush(*this->sortedvtkQtWordleWordObjectList[ii].color);
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
	 current word rect_item so don't have to translate the position every time 
	 
	QRectF current_rect = current_rect_item->rect();
	current_rect.translate(current_rect_item->pos());
	*/
int vtkQtWordleView::AllIntersectionsMin(vtkQtWordleQuadCIF* Tree, 
																					QGraphicsRectItem *rect_item, 
																					QRectF current_rect,
																					int last_index) 
{
	bool itemCollided;
	int idxCollided = -1;

	for (int ii=0; ii < Tree->ItemsList.length(); ++ii)
		{
		if (Tree->ItemsList[ii].index == last_index)
			{
			continue;
			}

		itemCollided = this->HierarchicalRectCollision_B(rect_item, Tree->ItemsList[ii].rect_item);
		if (itemCollided)
			{
			// Short circuit on collision
			return Tree->ItemsList[ii].index;
			}
		}
	
	/* traverse the four children */
	if (Tree->UL && IsBoundsIntersecting(Tree->UL->frame, current_rect)) 
		{
		idxCollided = this->AllIntersectionsMin(Tree->UL, rect_item, current_rect, last_index);
		if (idxCollided >= 0)
			{
			// Short circuit on collision
			return idxCollided;
			}
		}
	
	if (Tree->LL && IsBoundsIntersecting(Tree->LL->frame, current_rect)) 
		{
		idxCollided = this->AllIntersectionsMin(Tree->LL, rect_item, current_rect, last_index);
		if (idxCollided >= 0)
			{
			// Short circuit on collision
			return idxCollided;
			}
		}
	
	if (Tree->UR && IsBoundsIntersecting(Tree->UR->frame, current_rect))
		{
		idxCollided = this->AllIntersectionsMin(Tree->UR, rect_item, current_rect, last_index);
		if (idxCollided >= 0)
			{
			// Short circuit on collision
			return idxCollided;
			}
		}
	
	if (Tree->LR && IsBoundsIntersecting(Tree->LR->frame, current_rect)) 
		{
		idxCollided = this->AllIntersectionsMin(Tree->LR, rect_item, current_rect, last_index);
		if (idxCollided >= 0)
			{
			// Short circuit on collision
			return idxCollided;
			}
		}
	
	return -1;
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
void vtkQtWordleView::DoHybridLayout()
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
	
	vtkQtWordleQuadCIF *root_node;

	QRectF tmpRect = this->sortedvtkQtWordleWordObjectList[0].path_item->boundingRect();
	int word_count = std::min((int)this->sortedvtkQtWordleWordObjectList.size(), this->MaxNumberOfWords);
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
	  		((this->sortedvtkQtWordleWordObjectList[ii].font_size < quad_fsize_cutoff && ii > quad_minnum_cutoff)
	  		|| ii > quad_maxnum_cutoff))
	  	{
	  	double xAd = tmpRect.width() * quad_inc_factor;
	  	double yAd = tmpRect.height() * quad_inc_factor;
	  	QRectF quad_bounds = tmpRect.adjusted(-xAd, -yAd, xAd, yAd);
	  	root_node = new vtkQtWordleQuadCIF(quad_bounds);
	  	for (int jj=0; jj < ii; ++jj)
	  		{
	  		root_node->AddRectItemMin(this->sortedvtkQtWordleWordObjectList[jj].rect_item, jj);
	  		}
	  	mode = TEST_QUAD;
	  	quadtree_loaded = true;
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
			itemCollided = this->HierarchicalRectCollision_B(this->sortedvtkQtWordleWordObjectList[ii].rect_item, this->sortedvtkQtWordleWordObjectList[lastRectIndex].rect_item);
			if (itemCollided)
				{
				overlap = true;
				}
			else
				{
				if (mode == TEST_QUAD)
					{
					// Using QuadCIF tree for intersection tests
					QRectF current_rect = this->sortedvtkQtWordleWordObjectList[ii].rect_item->rect();
					current_rect.translate(this->sortedvtkQtWordleWordObjectList[ii].rect_item->pos());
					idxCollided = this->AllIntersectionsMin(root_node, this->sortedvtkQtWordleWordObjectList[ii].rect_item, current_rect, lastRectIndex);
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
						itemCollided = this->HierarchicalRectCollision_B(this->sortedvtkQtWordleWordObjectList[ii].rect_item, this->sortedvtkQtWordleWordObjectList[jj].rect_item);
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
				if (this->LayoutPathShape == vtkQtWordleView::CIRCULAR_PATH)
					{
					this->UpdateArchPositionSpirals(&this->sortedvtkQtWordleWordObjectList[ii]);
					}
				else
					{
					this->UpdateSquarePositionSpirals(&this->sortedvtkQtWordleWordObjectList[ii]);
					}
				this->sortedvtkQtWordleWordObjectList[ii].rect_item->setPos(this->sortedvtkQtWordleWordObjectList[ii].pos.X(),this->sortedvtkQtWordleWordObjectList[ii].pos.Y());
				}
			}
			
		this->sortedvtkQtWordleWordObjectList[ii].rect_item->setPos(this->sortedvtkQtWordleWordObjectList[ii].pos.X(),this->sortedvtkQtWordleWordObjectList[ii].pos.Y());
		this->sortedvtkQtWordleWordObjectList[ii].path_item->setPos(this->sortedvtkQtWordleWordObjectList[ii].pos.X(),this->sortedvtkQtWordleWordObjectList[ii].pos.Y());	
		this->scene->addItem(this->sortedvtkQtWordleWordObjectList[ii].path_item);
		
		if (mode == TEST_QUAD)
			{
			// Increase vtkQtWordleQuadCIF size if placement will take new word out of bounds
			if (tmpRect.x() < root_node->frame.x() ||
					tmpRect.y() < root_node->frame.y() ||
					tmpRect.x() + tmpRect.width() > root_node->frame.x() + root_node->frame.width() ||
					tmpRect.y() + tmpRect.height() > root_node->frame.y() + root_node->frame.height())
				{
				double xAd = tmpRect.width() * quad_inc_factor;
				double yAd = tmpRect.height() * quad_inc_factor;
				QRectF quad_bounds = tmpRect.adjusted(-xAd, -yAd, xAd, yAd);
				root_node = new vtkQtWordleQuadCIF(quad_bounds);
				for (int jj=0; jj < ii; ++jj)
					{
					root_node->AddRectItemMin(this->sortedvtkQtWordleWordObjectList[jj].rect_item, jj);
					}
				}
			// Add current item to the vtkQtWordleQuadCIF tree
			root_node->AddRectItemMin(this->sortedvtkQtWordleWordObjectList[ii].rect_item, ii);
			}
		tmpRect = tmpRect.united(this->sortedvtkQtWordleWordObjectList[ii].path_item->mapRectToScene(this->sortedvtkQtWordleWordObjectList[ii].path_item->boundingRect()));
		}

	// Rescale to fit in view
	double adjX = (tmpRect.width()*0.05)/2.0;
	double adjY = (tmpRect.height()*0.05)/2.0;
	QRectF boundingRect = tmpRect.adjusted(-adjX, -adjY, adjX, adjY);
	
	this->scene->setSceneRect(boundingRect);
	this->View->fitInView(boundingRect, Qt::KeepAspectRatio);
}

//----------------------------------------------------------------------------
void vtkQtWordleView::RedrawWithSameLayout()
{
	this->scene->setSceneRect(-300, -400, 900, 800);
	QRectF tmpRect = this->sortedvtkQtWordleWordObjectList[0].path_item->boundingRect();
	int word_count = std::min((int)this->sortedvtkQtWordleWordObjectList.size(), this->MaxNumberOfWords);

	// MAIN LOOP
	for (int ii=0; ii < word_count; ++ii)
	  {
		this->scene->addItem(this->sortedvtkQtWordleWordObjectList[ii].path_item);
		tmpRect = tmpRect.united(this->sortedvtkQtWordleWordObjectList[ii].path_item->mapRectToScene(this->sortedvtkQtWordleWordObjectList[ii].path_item->boundingRect()));
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
  
  // If input has changed, rebuild word objects list
  if (d->GetMTime() > this->LastInputMTime)
    {
    this->DataObjectToTable->Update();
    this->ApplyColors->Update();

		this->ClearGraphicsView();
		this->BuildvtkQtWordleWordObjectsList();
		this->DoHybridLayout();

    this->LastInputMTime = d->GetMTime();
    this->LastColorMTime = this->ApplyColors->GetMTime();
    this->LastMTime = this->GetMTime();
    }
  
  // If only Colors Modified, don't even reset positions
  if (this->ApplyColors->GetMTime() > this->LastColorMTime)
    {
		this->ClearGraphicsView();
		this->ResetOnlyvtkQtWordleWordObjectsColors();
		this->RedrawWithSameLayout();

    this->LastColorMTime = this->ApplyColors->GetMTime();
    this->LastMTime = this->GetMTime();
    }
    
  // If only this->Modified, only reset positions (not orientation or color)
  if (this->GetMTime() > this->LastMTime)
    {
		this->ClearGraphicsView();
		this->ResetOnlyvtkQtWordleWordObjectsPositions();
		this->DoHybridLayout();

    this->LastMTime = this->GetMTime();
    }
    
  this->View->update();
}

//----------------------------------------------------------------------------
void vtkQtWordleView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

