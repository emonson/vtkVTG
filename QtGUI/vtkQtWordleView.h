/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtWordleView.h

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
// .NAME vtkQtWordleView - A VTK view that displays the annotations
//    on its annotation link.
//
// .SECTION Description
// vtkQtWordleView is a VTK view using an underlying QTableView. 
//
// .SECTION Thanks

#ifndef __vtkQtWordleView_h
#define __vtkQtWordleView_h

#include "vtkvtgQVTKWin32Header.h"
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

#include <vector>
#include <iostream>	// for cout debug

class vtkApplyColors;
class vtkDataObjectToTable;
class QFontDatabase;
class QGraphicsScene;
class QGraphicsView;
class QRectF;
class QFont;

// Storage class for word objects
// Making this a separate public class because had trouble with
// vector sort routine when it was private...
//----------------------------------------------
class WordObject
{
public:
  WordObject()
    {
    this->text = "";
    this->size = 0.0;
    this->font_size = 0;
    this->theda = 0.0;
    this->R0 = 20.0;
    this->delta = 0.0;
    this->rdelta = 0.0;
    }
  ~WordObject()
    {
    }
  
	const double getsize() const { return size; }
	
	friend std::ostream& operator <<(std::ostream &os, const WordObject &obj)
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
  double theda;
  double R0;
  double delta;
  double rdelta;
  
  QColor* color;
  QPainterPath painter_path;
  QGraphicsPathItem* path_item;
  QGraphicsRectItem* rect_item;
};

//
// Utility classes for QuadCIF tree
//----------------------------------------------
class IndexedRectItem
{
public:
  IndexedRectItem()
    {
    rect_item = 0;
    index = 0;
    }
  IndexedRectItem(int idx, QGraphicsRectItem* ri)
    {
    index = idx;
    rect_item = ri;
    }
  ~IndexedRectItem()
    {
    }
    
	int index;
	QGraphicsRectItem* rect_item;
};

//----------------------------------------------
class Btree
{
public:
  Btree()
    {
    left = 0;
    right = 0;
    ex_min = 0;
    ex_max = 0;
    ex_middle = 0;
    }
  Btree(double min, double max)
    {
    left = 0;
    right = 0;
    ex_min = min;
    ex_max = max;
    ex_middle = (min + max) / 2.0;
    }
  ~Btree()
    {
    if (left) delete left;
    if (right) delete right;
    }
  
  void AddRectItem(QGraphicsRectItem *rect_item, double min, double max, int index);
    
	QList<IndexedRectItem> ItemsList; // Any items which exist at this node
	
	double ex_min;	// spatial extents of this node (can be x or y)
	double ex_max;
	double ex_middle;
	
	Btree* left;
	Btree* right;
};

//----------------------------------------------
class QuadCIF
{
public:
  QuadCIF()
    {
    UL = 0;
    LL = 0;
    UR = 0;
    LR = 0;
    xline = new Btree;
    yline = new Btree;
    }

  QuadCIF(QRectF rect)
    {
    UL = 0;
    LL = 0;
    UR = 0;
    LR = 0;
    frame = rect;
    xline = new Btree(rect.x(), rect.x() + rect.width());
    xmiddle = rect.x() + (rect.width()/2.0);
    yline = new Btree(rect.y(), rect.y() + rect.height());
    ymiddle = rect.y() + (rect.height()/2.0);
    }

  ~QuadCIF()
    {
    if (UL) delete UL;
    if (LL) delete LL;
    if (UR) delete UR;
    if (LR) delete LR;
    if (xline) delete xline;
    if (yline) delete yline;
    }
    
	void AddRectItem(QGraphicsRectItem *rect_item, int index);
    
	QRectF frame;
	double xmiddle;
	double ymiddle;

	Btree* xline;
	Btree* yline;
	
	QuadCIF* UL;
	QuadCIF* LL;
	QuadCIF* UR;
	QuadCIF* LR;
};

//----------------------------------------------
// This version tests QuadCIF without Btree
class QuadCIFmin
{
public:
  QuadCIFmin()
    {
    UL = 0;
    LL = 0;
    UR = 0;
    LR = 0;
    }

  QuadCIFmin(QRectF rect)
    {
    UL = 0;
    LL = 0;
    UR = 0;
    LR = 0;
    frame = rect;
    xmiddle = rect.x() + (rect.width()/2.0);
    ymiddle = rect.y() + (rect.height()/2.0);
    }

  ~QuadCIFmin()
    {
    if (UL) delete UL;
    if (LL) delete LL;
    if (UR) delete UR;
    if (LR) delete LR;
    }
    
	QList<IndexedRectItem> ItemsList; // Any items which exist at this node

	void AddRectItemMin(QGraphicsRectItem *rect_item, int index);
    
	QRectF frame;
	double xmiddle;
	double ymiddle;

	QuadCIFmin* UL;
	QuadCIFmin* LL;
	QuadCIFmin* UR;
	QuadCIFmin* LR;
};

// ============================================================

class VTK_VTG_QVTK_EXPORT vtkQtWordleView : public vtkQtView
{
Q_OBJECT

public:
  static vtkQtWordleView *New();
  vtkTypeMacro(vtkQtWordleView, vtkQtView);
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
  // Enum containing predefined values layout orientations
  enum 
  	{
    HORIZONTAL = 0,
    MOSTLY_HORIZONTAL = 1,
    HALF_AND_HALF = 2,
    MOSTLY_VERTICAL = 3,
    VERTICAL = 4
		};

  // The field type to copy into the output table.
  // Should be one of FIELD_DATA, POINT_DATA, CELL_DATA, VERTEX_DATA, EDGE_DATA.
  vtkGetMacro(FieldType, int);
  void SetFieldType(int);
  
  // ==============================================
  // TODO: Need to change this so it sets a flag so Update knows
  //   to rebuild word objects vector when any important properties
  //   are updated!!!

  // Description:
  // The array to use for coloring items in view.  Default is "color".
  void SetColorArrayName(const char* name);
  const char* GetColorArrayName();
  
  // Description:
  // The array to use for terms that will be place in Wordle
  void SetTermsArrayName(const char* name);
  const char* GetTermsArrayName();
  
  // Description:
  // The array to use for size of the terms that will be place in Wordle
  void SetSizeArrayName(const char* name);
  const char* GetSizeArrayName();
  
  // Description:
  // Whether to color vertices.  Default is off.
  void SetColorByArray(bool vis);
  bool GetColorByArray();
  vtkBooleanMacro(ColorByArray, bool);

  // Description:
  // Set/Get the font family name to be used in the Wordle. FontFamilyExists
  // is used to probe whether the Qt font database includes a given font family.
  // Examples include "Adobe Caslon Pro", "Palatino", "Rockwell"
  void SetFontFamily(const char* name);
  const char* GetFontFamily();
  bool FontFamilyExists(const char* name);

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
  // Set the (max) number of words to include in the wordle
  vtkGetMacro(MaxNumberOfWords, int);
  vtkSetMacro(MaxNumberOfWords, int);

  // Description:
  // Set watch layout to true for debugging. Slows it all way
  // down and lets you watch the path each word is taking.
  vtkSetMacro(WatchLayout, bool);

  // Description:
  // Set watch layout to true for debugging. Slows it all way
  // down and lets you watch the path each word is taking.
  vtkSetMacro(WatchCollision, bool);

  // Description:
  // Extra delay in µs to add to stepping when watching layout
  vtkSetMacro(WatchDelay, int);

  // TODO: Should also add a routine where the colors are
  //   changed and same positions redrawn if lookup table
  //   or color array changes...
  virtual void ApplyViewTheme(vtkViewTheme* theme);
  
  // Description
  // Routines for dealing with searching QuadCIF tree
  int AllIntersectionsMin(QuadCIFmin* Tree, QGraphicsRectItem* rect_item, QRectF current_rect, int last_index);
  QList<int> AllIntersections(QuadCIF* Tree, QRectF current_rect);
	QList<int> IntersectLine(Btree *node, QRectF current_rect, double rect_min, double rect_max);
	bool IsBoundsIntersecting(QRectF frame, QRectF current_rect);
	bool IsFullIntersecting(QGraphicsRectItem* target_item, QRectF current_rect);


  // ==============================================

  void ClearGraphicsView();
  
  vtkVector2f CartesianToPolar(vtkVector2f posArr);
	vtkVector2f PolarToCartesian(vtkVector2f posArr);	
	vtkVector2f MakeInitialPosition();
	
	void DoLayout();
  
  void ZoomToBounds();
  QGraphicsScene* GetScene();

  // Description:
  // Updates the view.
  virtual void Update();

protected:
  vtkQtWordleView();
  ~vtkQtWordleView();

  virtual void AddRepresentationInternal(vtkDataRepresentation* rep);
  virtual void RemoveRepresentationInternal(vtkDataRepresentation* rep);

	void BuildWordObjectsList();
	void ResetOnlyWordObjectsPositions();
	bool HierarchicalRectCollision_B(QGraphicsRectItem* rectA, QGraphicsRectItem* rectB);

private:
  unsigned long LastInputMTime;
  unsigned long LastMTime;
  
  std::vector<WordObject> sortedWordObjectList;
	void UpdatePositionSpirals(WordObject* word);
  
  QPointer<QGraphicsView> View;
  QPointer<QGraphicsScene> scene;
  
  QRectF* boundingRect;
  QFont* font;
  QFontDatabase* FontDatabase;
  
  int bigFontSize;
  int MaxNumberOfWords;
  int FieldType;
  int orientation;
  
  bool WatchLayout;
  bool WatchCollision;
  int WatchDelay;
  
	float xbuffer;
	float ybuffer;
	float randSpread;
	float thedaMult;
	float thedaPow;
	float rMult;
	float rPow;

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

  vtkQtWordleView(const vtkQtWordleView&);  // Not implemented.
  void operator=(const vtkQtWordleView&);  // Not implemented.
  
};

#endif
