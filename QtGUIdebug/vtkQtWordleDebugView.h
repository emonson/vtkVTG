/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtWordleDebugView.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**********************************************************************
 This source file is part of the Titan Toolkit

 This source code is released under the New BSD License.
 **********************************************************************/
// .NAME vtkQtWordleDebugView - A vtkView that displays a tag cloud in the
// spirit of a Wordle (www.wordle.net)
//
// .SECTION Description
// vtkQtWordleDebugView is a vtkView for displaying a tag or word cloud
// in the spirit of a Wordle (www.wordle.net), but using its own C++
// implementation of the algorithms spelled out in Viegas, Wattenberg
// & Feinberg, "Participatory Visualization with Wordle" (2009).
// 
// The view takes in a table of terms (strings) and weights (doubles).
// The terms are then layed out in a compact fashion according to
// various options, with the size of the terms proportional to the
// weight values. Terms can also be colored by an array (table column)
// using a lookup table passed in through the PointLookupTable of
// a vtkViewTheme.
// 
// Qt is used for its font handling. Almost any font installed on your
// system can be used if you correctly specify the "family" name, style
// and weight. There is a utility method, GetAllFontFamilies, which you
// can pass a vtkStringArray and it will fill it with valid family
// names.
// 
// Besides displaying the view in a Qt-based application, you can
// just use vtkQtInitialization to create a QtApplication instance
// and then use this class to generate vtkImageData, PNG or PDF
// output.
//
// .SECTION Thanks
// Thanks to Jonathan Feinberg for developing the original Wordle.

#ifndef __vtkQtWordleDebugView_h
#define __vtkQtWordleDebugView_h

#include "vtkvtgQVTKdebugWin32Header.h"
#include "vtkVector.h"
#include "vtkQtAbstractModelAdapter.h"
#include "vtkQtView.h"
#include "vtkSmartPointer.h"

#include <QObject>
#include <QPointer>
#include <QSortFilterProxyModel>
#include <QPainterPath>
#include <QColor>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>

#include <vector>
#include <iostream>	// for cout debug

class vtkApplyColors;
class vtkDataObjectToTable;
class vtkImageData;
class vtkQImageToImageSource;
class vtkStringArray;
class QFontDatabase;
class QGraphicsView;
class QRectF;
class QFont;

// Storage class for word objects
// Making this a separate public class because had trouble with
// vector sort routine when it was private...
//----------------------------------------------
class vtkQtWordleDebugWordObject
{
public:
  vtkQtWordleDebugWordObject()
    {
    this->text = "";
    this->size = 0.0;
    this->font_size = 0;
    
    this->theta = 0.0;
    this->R0 = 20.0;
    this->delta = 0.0;
    this->rdelta = 0.0;
    
    this->flag = true;
    this->sign = 1;
    this->count = 0;
    this->target_count = 1;
    this->dist = 0.0;
    }
  ~vtkQtWordleDebugWordObject()
    {
    }
  
	const double getsize() const { return size; }
	
	friend std::ostream& operator <<(std::ostream &os, const vtkQtWordleDebugWordObject &obj)
		{
    os.precision(2);
    os << "Text: " << obj.text << "\t";
    os << "Size: " << std::fixed << obj.size << std::endl;
    return os;
		}
	
  std::string text;
  vtkIdType original_index;
  double size;
  int font_size;
  
  vtkVector2f initial_pos;
  vtkVector2f pos;
  
  // Archimedean Spiral layout
  double theta;
  double R0;
  double delta;
  double rdelta;
  
  // Square Spiral layout
  bool flag;
  int sign;
  int count;
  int target_count;
  double dist;
  
  QColor* color;
  QPainterPath painter_path;
  QGraphicsPathItem* path_item;
  QGraphicsRectItem* rect_item;
};

//
// Utility classes for QuadCIF tree
//----------------------------------------------
class vtkQtWordleDebugIndexedRectItem
{
public:
  vtkQtWordleDebugIndexedRectItem()
    {
    rect_item = 0;
    index = 0;
    }
  vtkQtWordleDebugIndexedRectItem(int idx, QGraphicsRectItem* ri)
    {
    index = idx;
    rect_item = ri;
    }
  ~vtkQtWordleDebugIndexedRectItem()
    {
    }
    
	int index;
	QGraphicsRectItem* rect_item;
};

//----------------------------------------------
// This version tests QuadCIF without Btree
class vtkQtWordleDebugQuadCIF
{
public:
  vtkQtWordleDebugQuadCIF()
    {
    UL = 0;
    LL = 0;
    UR = 0;
    LR = 0;
    }

  vtkQtWordleDebugQuadCIF(QRectF rect)
    {
    UL = 0;
    LL = 0;
    UR = 0;
    LR = 0;
    frame = rect;
    xmiddle = rect.x() + (rect.width()/2.0);
    ymiddle = rect.y() + (rect.height()/2.0);
    }

  vtkQtWordleDebugQuadCIF(QRectF rect, QGraphicsScene* scene)
    {
    UL = 0;
    LL = 0;
    UR = 0;
    LR = 0;
    frame = rect;
    xmiddle = rect.x() + (rect.width()/2.0);
    ymiddle = rect.y() + (rect.height()/2.0);
		scene->addRect(frame, QPen(QBrush(QColor(0,0,0)), 1.0));
    }

  ~vtkQtWordleDebugQuadCIF()
    {
    if (UL) delete UL;
    if (LL) delete LL;
    if (UR) delete UR;
    if (LR) delete LR;
    }
    
	QList<vtkQtWordleDebugIndexedRectItem> ItemsList; // Any items which exist at this node

	void AddRectItemMin(QGraphicsRectItem *rect_item, int index);
	void AddRectItemMin(QGraphicsRectItem *rect_item, int index, QGraphicsScene* scene);
    
	QRectF frame;
	double xmiddle;
	double ymiddle;

	vtkQtWordleDebugQuadCIF* UL;
	vtkQtWordleDebugQuadCIF* LL;
	vtkQtWordleDebugQuadCIF* UR;
	vtkQtWordleDebugQuadCIF* LR;
};

// ============================================================

class VTK_VTG_QVTK_DEBUG_EXPORT vtkQtWordleDebugView : public vtkQtView
{
Q_OBJECT

public:
  static vtkQtWordleDebugView *New();
  vtkTypeMacro(vtkQtWordleDebugView, vtkQtView);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get the main container of this view (a  QWidget).
  // The application typically places the view with a call
  // to GetWidget(): something like this
  // this->ui->box->layout()->addWidget(this->View->GetWidget());
  virtual QWidget* GetWidget();

  // Description:
  // Enum containing type of data (from vtkQtListView)
  enum
    {
    FIELD_DATA = 0,
    POINT_DATA = 1,
    CELL_DATA = 2,
    VERTEX_DATA = 3,
    EDGE_DATA = 4,
    ROW_DATA = 5
    };

  // Description:
  // Enum containing predefined values for font "weight" from QFont::Weight
  enum 
  	{
		Light	= 25,
		Normal = 50,
		DemiBold = 63,
		Bold = 75,
		Black = 87
		};

  // Description:
  // Enum containing predefined values for font "style" from QFont::Style
  enum 
  	{
		StyleNormal	= 0,
		StyleItalic = 1,
		StyleOblique = 2
		};

  // Description:
  // Enum containing predefined values for layout orientations
  enum 
  	{
    HORIZONTAL = 0,
    MOSTLY_HORIZONTAL = 1,
    HALF_AND_HALF = 2,
    MOSTLY_VERTICAL = 3,
    VERTICAL = 4
		};

  // Description:
  // Enum containing predefined values for layout path shapes
  enum 
  	{
    CIRCULAR_PATH = 0,
    SQUARE_PATH = 1
		};

  // The field type to copy into the output table.
  // Should be one of FIELD_DATA, POINT_DATA, CELL_DATA, VERTEX_DATA, EDGE_DATA.
  vtkGetMacro(FieldType, int);
  void SetFieldType(int);
  
  // Description:
  // The array to use for terms that will be place in Wordle
  void SetTermsArrayName(const char* name);
  const char* GetTermsArrayName();
  
  // Description:
  // The array to use for size of the terms that will be place in Wordle
  void SetSizeArrayName(const char* name);
  const char* GetSizeArrayName();
  
  // Description:
  // The array to use for coloring items in view.  Default is "color".
  void SetColorArrayName(const char* name);
  const char* GetColorArrayName();
  
  // Description:
  // Whether to color words according to a lookup table.
  // Default is off.
  void SetColorByArray(bool vis);
  bool GetColorByArray();
  vtkBooleanMacro(ColorByArray, bool);

  // Description:
  // View theme that will be used for coloring.
  // PointColor will be used for default term color.
  // PointLookupTable will be used for term color when
  // ColorByArray is True. BackgroundColor is used
  // for view background color.
  virtual void ApplyViewTheme(vtkViewTheme* theme);
  
  // Description:
  // Set/Get the font family name to be used in the Wordle. FontFamilyExists
  // is used to probe whether the Qt font database includes a given font family.
  // Or, a vtkStringArray can be passed to GetAllFontFamilies() to get back
  // a list of all possible valid values.
  // Examples include "Adobe Caslon Pro", "Palatino", "Rockwell".
  void SetFontFamily(const char* name);
  const char* GetFontFamily();
  bool FontFamilyExists(const char* name);
  void GetAllFontFamilies(vtkStringArray* famlies);

  // Description:
  // Set/Get the font style name to be used in the Wordle.
  // See enum from QFont::Style (0,1,2)
  void SetFontStyle(int style);
  int GetFontStyle();

  // Description:
  // Set/Get the font weight to be used in the Wordle.
  // See enum from QFont::Weight (0-99)
  void SetFontWeight(int weight);
  int GetFontWeight();

  // Description:
  // Set/Get the layout orientations
  // See enum (0,1,2,3,4)
  void SetOrientation(int orientation);
  int GetOrientation();

  // Description:
  // Specify the width,height pixel dimensions of vtkImageData
  // output from GetImageData(). Default is 256, 256.
  vtkSetVector2Macro(OutputImageDataDimensions, int);
  vtkGetVectorMacro(OutputImageDataDimensions, int, 2);
  vtkImageData* GetImageData();

  // Description:
  // Set the (max) number of words to include in the wordle
  vtkGetMacro(MaxNumberOfWords, int);
  vtkSetMacro(MaxNumberOfWords, int);

  // Description:
  // Set the power relationship between size array value
  // and font size. (size/max_size)^power
  // Traditional wordles use 1.0 (default here). Size value proportional to
  // area (for words of the same length) would be use 0.5
  // Some people like something in between.
  vtkGetMacro(WordSizePower, double);
  vtkSetMacro(WordSizePower, double);

  // Description:
  // Set the (max) number of words to include in the wordle
  vtkGetMacro(LayoutPathShape, int);
  vtkSetMacro(LayoutPathShape, int);
  
  // Description:
  // Output routines for PDF or Image given a file name.
  // The Image output uses the same OutputImageDimensions
  // as GetImageData(). SaveImage() uses QImage->save()
  // which will try to guess the format from the file name
  // extension, or you can pass a valid format string.
  // E.g. "PNG"
  void SavePDF(char* filename);
  void SaveImage(char* filename, const char* format=0);

  // Description:
  // DEBUG
  // Set watch layout to true for debugging. Slows it all way
  // down and lets you watch the path each word is taking.
  vtkSetMacro(WatchLayout, bool);

  // Description:
  // DEBUG
  // Set watch layout to true for debugging. Slows it all way
  // down and lets you watch the path each word is taking.
  vtkSetMacro(WatchCollision, bool);

  // Description:
  // DEBUG
  // Extra delay in µs to add to stepping when watching layout
  vtkSetMacro(WatchDelay, int);
  
  // Description:
  // DEBUG
  // View QuadCIF tree in scene for debugging
  vtkSetMacro(WatchQuadTree, bool);

  // Description:
  // DEBUG
  // Layout parameters
	vtkSetMacro( xbuffer, float );
	vtkSetMacro( ybuffer, float );
	vtkSetMacro( randSpread, float );
	vtkSetMacro( thetaMult, float );
	vtkSetMacro( thetaPow, float );
	vtkSetMacro( rMult, float );
	vtkSetMacro( rPow, float );
	vtkSetMacro( dMult, float );
	vtkSetMacro( dPow, float );
	vtkGetMacro( xbuffer, float );
	vtkGetMacro( ybuffer, float );
	vtkGetMacro( randSpread, float );
	vtkGetMacro( thetaMult, float );
	vtkGetMacro( thetaPow, float );
	vtkGetMacro( rMult, float );
	vtkGetMacro( rPow, float );
	vtkGetMacro( dMult, float );
	vtkGetMacro( dPow, float );


  // ==============================================

  // Description:
  // Zoom view to outer bounds of the Wordle.
  void ZoomToBounds();
  
  // Description:
  // Get the QGraphicsScene for the view.
  QGraphicsScene* GetScene();
  
  // Description:
  // Updates the view.
  virtual void Update();

protected:
  vtkQtWordleDebugView();
  ~vtkQtWordleDebugView();

  virtual void AddRepresentationInternal(vtkDataRepresentation* rep);
  virtual void RemoveRepresentationInternal(vtkDataRepresentation* rep);

  void ClearGraphicsView();
  
  vtkVector2f CartesianToPolar(vtkVector2f posArr);
	vtkVector2f PolarToCartesian(vtkVector2f posArr);	
	vtkVector2f MakeInitialPosition();
	
	void DoHybridLayout();
	void DoCheckAllLayout();
	void RedrawWithSameLayout();
  
	void BuildvtkQtWordleDebugWordObjectsList();
	void ResetOnlyvtkQtWordleDebugWordObjectsPositions();
	void ResetOnlyvtkQtWordleDebugWordObjectsColors();
	bool HierarchicalRectCollision_B(QGraphicsRectItem* rectA, QGraphicsRectItem* rectB);

  // Description:
  // Routines for dealing with searching QuadCIF tree
  int AllIntersectionsMin(vtkQtWordleDebugQuadCIF* Tree, QGraphicsRectItem* rect_item, QRectF current_rect, int last_index);
 	bool IsBoundsIntersecting(QRectF frame, QRectF current_rect);

private:
  // Description:
  // Calling Update on the view will regenerate all of the internal
  // data if the input or size array have changed. This takes the longest. 
  // If only the view itself is Modified, then positions will be reset, but
  // not orientation. This is faster than regenerating the whole word list.
  // Changing color settings will keep the same layout and only update the
  // colors, which is very fast.
  unsigned long LastInputMTime;
  unsigned long LastMTime;
  unsigned long LastColorMTime;
  
  int OutputImageDataDimensions[2];
  
  std::vector<vtkQtWordleDebugWordObject> sortedvtkQtWordleDebugWordObjectList;
	void UpdateArchPositionSpirals(vtkQtWordleDebugWordObject* word);
	void UpdateSquarePositionSpirals(vtkQtWordleDebugWordObject* word);
  
  QPointer<QGraphicsView> View;
  QPointer<QGraphicsScene> scene;
  
  QRectF* boundingRect;
  QFont* font;
  QFontDatabase* FontDatabase;
  
  int bigFontSize;
  int MaxNumberOfWords;
  int FieldType;
  int orientation;
  double WordSizePower;
  int LayoutPathShape;
  
  bool WatchLayout;
  bool WatchCollision;
  bool WatchQuadTree;
  int WatchDelay;
  
	float xbuffer;
	float ybuffer;
	float randSpread;
	// Archimedean spiral
	float thetaMult;
	float thetaPow;
	float rMult;
	float rPow;
	// Square spiral
	float dMult;
	float dPow;

  char* ColorArrayNameInternal;
  vtkSetStringMacro(ColorArrayNameInternal);
  vtkGetStringMacro(ColorArrayNameInternal);

  char* TermsArrayNameInternal;
  vtkSetStringMacro(TermsArrayNameInternal);
  vtkGetStringMacro(TermsArrayNameInternal);

  char* SizeArrayNameInternal;
  vtkSetStringMacro(SizeArrayNameInternal);
  vtkGetStringMacro(SizeArrayNameInternal);

  vtkSmartPointer<vtkDataObjectToTable> DataObjectToTable;
  vtkSmartPointer<vtkApplyColors> ApplyColors;
  vtkSmartPointer<vtkQImageToImageSource> QImageToImage;

  vtkQtWordleDebugView(const vtkQtWordleDebugView&);  // Not implemented.
  void operator=(const vtkQtWordleDebugView&);  // Not implemented.
  
};

#endif
