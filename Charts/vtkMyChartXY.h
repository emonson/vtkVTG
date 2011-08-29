/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMyChartXY.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkMyChartXY - Factory class for drawing XY charts
//
// .SECTION Description
// This class implements an XY chart.

#ifndef __vtkMyChartXY_h
#define __vtkMyChartXY_h

#include "vtkChartXY.h"
#include "vtkSmartPointer.h"
// #include "vtkVector.h"
#include <math.h>
#include <vtkstd/vector>

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
class vtkMyChartXYPrivate; // Private class to keep my STL vector in...

class VTK_CHARTS_EXPORT vtkMyChartXY : public vtkChartXY
{
public:
  vtkTypeMacro(vtkMyChartXY, vtkChartXY);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a 2D Chart object.
  static vtkMyChartXY *New();

  // Description:
  // Perform any updates to the item that may be necessary before rendering.
  // The scene should take care of calling this on all items before their
  // Paint function is invoked.
  virtual void Update();

  // Description:
  // Paint event for the chart, called whenever the chart needs to be drawn
  // virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Get/Set the HighlightLink for the chart.
  virtual void SetHighlightLink(vtkAnnotationLink *link);
  vtkGetObjectMacro(HighlightLink, vtkAnnotationLink);

  // Description:
  // Get/Set the DataColumnsLink for the chart.
  // Link used to externally control which columns are plotted against
  // each other. If being used, should contain a selection node with
  // a selection list of two indices which will be the X and Y axis columns.
  virtual void SetDataColumnsLink(vtkAnnotationLink *link);
  vtkGetObjectMacro(DataColumnsLink, vtkAnnotationLink);

  // Description
  // Set whether tooltip will show image or text
  // Please set the image stack on the plot before this call so here
  // I can initialize tooltip with an example image.
  virtual void SetTooltipShowImage(bool ShowImage);

  // Description
  // Set a size scaling factor for tooltip image
  virtual void SetTooltipImageScalingFactor(float ScalingFactor);
  virtual void SetTooltipImageTargetSize(int pixels);

  // Description
  // ImageData associated with plot, which the tooltip in the chartXY will get
  // a slice of to display when hovering over points (needs to be just 2d)
  virtual void SetTooltipImageStack(vtkImageData*);

  // Description
  // Externally set which data columns will be the X and Y axis data
  // Will look for the first vtkMyPlotPoints and set that
  virtual void SetPlotColumnIndices(int xI, int yI);

protected:
  vtkMyChartXY();
  ~vtkMyChartXY();

  // Description:
  // The tooltip item for the chart - can be used to display extra information.
  // vtkTooltipImageItem *Tooltip;
  bool TooltipShowImage;
  
  // Description:
  // Set the information passed to the tooltip
  virtual void SetTooltipInfo(const vtkContextMouseEvent &,
                              const vtkVector2f &,
                              vtkIdType, vtkPlot*,
                              vtkIdType segmentIndex = -1);

private:
  vtkMyChartXY(const vtkMyChartXY &); // Not implemented.
  void operator=(const vtkMyChartXY &);   // Not implemented.

  // Description:
  // Link back into chart to highlight selections made in other plots
  vtkAnnotationLink *HighlightLink;
  
  // Description
  // Link used to externally control which columns are plotted against
  // each other. If being used, should contain a selection node with
  // a selection list of two indices which will be the X and Y axis columns.
  vtkAnnotationLink *DataColumnsLink;
  
  // Description:
  // Contains the map between indices (axis images) and "valid", non_ids data
  vtkstd::vector<vtkIdType> col_idxs;

};

#endif //__vtkMyChartXY_h
