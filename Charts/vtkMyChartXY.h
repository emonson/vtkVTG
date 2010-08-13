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
#include <math.h>

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
  // Set the vtkHighlightLink for the chart.
  virtual void SetHighlightLink(vtkAnnotationLink *link);

  // Description:
  // Get the vtkHighlightLink for the chart.
  vtkGetObjectMacro(HighlightLink, vtkAnnotationLink);

  // Description
  // Set whether tooltip will show image or text
  // Please set the image stack on the plot before this call so here
  // I can initialize tooltip with an example image.
  virtual void SetTooltipShowImage(bool ShowImage);

  // Description
  // Set a size scaling factor for tooltip image
  virtual void SetTooltipImageScalingFactor(float ScalingFactor);
  virtual void SetTooltipImageTargetSize(int pixels);


protected:
  vtkMyChartXY();
  ~vtkMyChartXY();

  // Description:
  // The tooltip item for the chart - can be used to display extra information.
  vtkTooltipImageItem *Tooltip;
  bool TooltipShowImage;


private:
  vtkMyChartXY(const vtkMyChartXY &); // Not implemented.
  void operator=(const vtkMyChartXY &);   // Not implemented.

  vtkMyChartXYPrivate *ChartPrivate; // Private class where I hide my STL containers

  // Description:
  // Private functions to render different parts of the chart
  void RenderPlotHighlights(vtkContext2D *painter);

  // Description:
  // Link back into chart to highlight selections made in other plots
  vtkAnnotationLink *HighlightLink;

  // Description:
  // Try to locate a point within the plots to display in a tooltip
  bool LocatePointInPlots(const vtkContextMouseEvent &mouse);


};

#endif //__vtkMyChartXY_h
