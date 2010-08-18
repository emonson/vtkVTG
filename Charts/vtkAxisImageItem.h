/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAxisImageItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkAxisImageItem - Factory class for drawing XY charts
//
// .SECTION Description
// This class implements an XY chart.

#ifndef __vtkAxisImageItem_h
#define __vtkAxisImageItem_h

#include "vtkContextItem.h"
#include "vtkSmartPointer.h"
#include <math.h>

class vtkChartXY;
class vtkPlot;
class vtkAxis;
class vtkPlotGrid;
class vtkTable;
class vtkChartLegend;
class vtkTooltipImageItem;
class vtkContextMouseEvent;
class vtkDataArray;
class vtkAnnotationLink;
class vtkImageData;
class vtkMatrix4x4;
class vtkImageReslice;
class vtkLookupTable;
class vtkImageMapToColors;
class vtkAxisImagePrivate;
class vtkAxisImageItemPrivate; // Private class to keep my STL vector in...

class VTK_CHARTS_EXPORT vtkAxisImageItem : public vtkContextItem
{
public:
  vtkTypeMacro(vtkAxisImageItem, vtkContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a 2D Chart object.
  static vtkAxisImageItem *New();

  // Description:
  // Paint event for the chart, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);

  // Description
  // ImageData associated with plot, which the tooltip in the chartXY will get
  // a slice of to display when hovering over points (needs to be just 2d)
  virtual void SetAxisImageStack(vtkImageData*);
  virtual void SetCenterImage(vtkImageData*);
  virtual int GetNumberOfImages();
  virtual vtkImageData* GetImageAtIndex(int imageId);
  virtual void SetAxisImagesVertical();
  virtual void SetAxisImagesHorizontal();
  
  // Description:
  // Get/Set the DataColumnsLink for the chart.
  // Link used to externally control which columns are plotted against
  // each other. If being used, should contain a selection node with
  // a selection list of two indices which will be the X and Y axis columns.
  virtual void SetDataColumnsLink(vtkAnnotationLink *link);
  vtkGetObjectMacro(DataColumnsLink, vtkAnnotationLink);

  // Description
  // Alternatively to setting an Annotation Link to control any XY chart,
  // you can just register the vtkChartXY with the vtkAxisImageItem and
  // it will look for _ids table columns to avoid and can control the 
  // chart plotting directly
  virtual void SetChartXY(vtkChartXY *chart);

  // Description:
  // Return true if the supplied x, y coordinate is inside the item.
  virtual bool Hit(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse enter event.
  virtual bool MouseEnterEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse move event.
  virtual bool MouseMoveEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse leave event.
  virtual bool MouseLeaveEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse button down event
  virtual bool MouseButtonPressEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse button release event.
  virtual bool MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse wheel event, positive delta indicates forward movement of the wheel.
  virtual bool MouseWheelEvent(const vtkContextMouseEvent &mouse, int delta);

  // Description:
  // Enum containing whether axis images are 
  enum {
    VERTICAL = 0,
    HORIZONTAL
  };

  // Description:
  // Set the vtkContextScene for the item, always set for an item in a scene.
  virtual void SetScene(vtkContextScene *scene);

  // Description:
  // Set/get the width and the height of the chart.
  vtkSetVector2Macro(Geometry, int);
  vtkGetVector2Macro(Geometry, int);

  // Description:
  // Set/get the first point in the chart (the bottom left).
  vtkSetVector2Macro(Point1, int);
  vtkGetVector2Macro(Point1, int);

  // Description:
  // Set/get the second point in the chart (the top right).
  vtkSetVector2Macro(Point2, int);
  vtkGetVector2Macro(Point2, int);

  // Description:
  // Set/get the borders of the chart (space in pixels around the chart).
  void SetBottomBorder(int border);
  void SetTopBorder(int border);
  void SetLeftBorder(int border);
  void SetRightBorder(int border);

  // Description:
  // Set/get the borders of the chart (space in pixels around the chart).
  void SetBorders(int left, int bottom, int right, int top);

protected:
  vtkAxisImageItem();
  ~vtkAxisImageItem();

  // Description
  // ImageData associated with the axes. This should be in a z-stack.
  vtkImageData* AxisImageStack;
  int NumImages;
  vtkImageData* CenterImage;
  
 	// Description
 	// Used internally to check out chart table names and only use indices
 	// of columns whose names don't end in _ids
 	void SetColumnIndices();

private:
  vtkAxisImageItem(const vtkAxisImageItem &); // Not implemented.
  void operator=(const vtkAxisImageItem &);   // Not implemented.

  vtkAxisImageItemPrivate *AIPrivate; // Private class where I hide my STL containers

  // Description:
  // The width and the height of the chart.
  int Geometry[2];

  // Description:
  // The position of the lower left corner of the chart.
  int Point1[2];

  // Description:
  // The position of the upper right corner of the chart.
  int Point2[2];

  // Description
  // Link used to externally control which columns are plotted against
  // each other. If being used, should contain a selection node with
  // a selection list of two indices which will be the X and Y axis columns.
  vtkAnnotationLink *DataColumnsLink;
  
  // Description
  // Alternatively to setting an Annotation Link to control any XY chart,
  // you can just register the vtkChartXY with the vtkAxisImageItem and
  // it will look for _ids table columns to avoid and can control the 
  // chart plotting directly
  vtkChartXY *ChartXY;

  // Pipeline for displaying axis images
  vtkSmartPointer<vtkMatrix4x4> resliceAxes;
  vtkSmartPointer<vtkImageReslice> reslice;
  vtkSmartPointer<vtkLookupTable> lut;
  vtkSmartPointer<vtkImageMapToColors> color;
  // Pipeline for displaying center image
  vtkSmartPointer<vtkLookupTable> lutBW;
  vtkSmartPointer<vtkImageMapToColors> colorBW;
  
};

#endif //__vtkAxisImageItem_h
