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
  
	const int getsize() const { return size; }
	
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

  void ClearGraphicsView();
  
  vtkVector2f CartesianToPolar(vtkVector2f posArr);
	vtkVector2f PolarToCartesian(vtkVector2f posArr);	
	vtkVector2f MakeInitialPosition();
	
	void DoLayout();

  virtual void ApplyViewTheme(vtkViewTheme* theme);
  
  void ZoomToBounds();

  // Description:
  // Updates the view.
  virtual void Update();

protected:
  vtkQtWordleView();
  ~vtkQtWordleView();

  virtual void AddRepresentationInternal(vtkDataRepresentation* rep);
  virtual void RemoveRepresentationInternal(vtkDataRepresentation* rep);

	void BuildWordObjectsList();
	void ResetWordObjectsPositions();
	void ResetWordObjectsColors();
	bool HierarchicalRectCollision_B();

private:
  unsigned long LastInputMTime;
  unsigned long LastMTime;
  
  std::vector<WordObject> sortedWordObjectList;
	void UpdatePositionSpirals(WordObject* word);
  
  QPointer<QGraphicsView> View;
  QPointer<QGraphicsScene> scene;
  
  QGraphicsRectItem* rectA;
  QGraphicsRectItem* rectB;
  QGraphicsRectItem* lastRect;
  QRectF* boundingRect;
  QFont* font;
  QFontDatabase* FontDatabase;
  
  int bigFontSize;
  int MaxNumberOfWords;
  int FieldType;
  int orientation;
  
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
