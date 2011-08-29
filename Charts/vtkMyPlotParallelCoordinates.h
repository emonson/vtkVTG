/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMyPlotParallelCoordinates.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkMyPlotParallelCoordinates - Class for drawing an XY plot given two columns from a
// vtkTable.
//
// .SECTION Description
//

#ifndef __vtkMyPlotParallelCoordinates_h
#define __vtkMyPlotParallelCoordinates_h

#include "vtkPlot.h"
#include "vtkScalarsToColors.h" // For VTK_COLOR_MODE_DEFAULT and _MAP_SCALARS

class vtkMyChartParallelCoordinates;
class vtkTable;
class vtkPoints2D;
class vtkStdString;
class vtkImageData;
class vtkScalarsToColors;
class vtkUnsignedCharArray;

class VTK_CHARTS_EXPORT vtkMyPlotParallelCoordinates : public vtkPlot
{
public:
  vtkTypeMacro(vtkMyPlotParallelCoordinates, vtkPlot);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a parallel coordinates chart
  static vtkMyPlotParallelCoordinates* New();

  // Description:
  // Perform any updates to the item that may be necessary before rendering.
  // The scene should take care of calling this on all items before their
  // Paint function is invoked.
  virtual void Update();

  // Description:
  // Paint event for the XY plot, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Paint legend event for the XY plot, called whenever the legend needs the
  // plot items symbol/mark/line drawn. A rect is supplied with the lower left
  // corner of the rect (elements 0 and 1) and with width x height (elements 2
  // and 3). The plot can choose how to fill the space supplied.
  virtual bool PaintLegend(vtkContext2D *painter, float rect[4]);

  // Description:
  // Get the bounds for this mapper as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
  virtual void GetBounds(double bounds[4]);

//BTX
  // Description:
  // Function to query a plot for the nearest point to the specified coordinate.
  virtual vtkIdType GetNearestPoint(const vtkVector2f& point,
                               const vtkVector2f& tolerance,
                               vtkVector2f* location);
//ETX

  // Description;
  // Set the parent, required to query the axes etc.
  virtual void SetParent(vtkMyChartParallelCoordinates* parent);

  // Description:
  // Set the selection criteria on the given axis in normalized space (0.0 - 1.0).
  bool SetSelectionRange(int Axis, float low, float high);

  // Description:
  // Reset the selection criteria for the chart.
  bool ResetSelectionRange();

  // Description:
  // Set the list of ids for highlighting (back from outside plot).
  virtual void SetHighlightSelection(vtkIdTypeArray *id);
  vtkGetObjectMacro(HighlightSelection, vtkIdTypeArray);

  // Description:
  // This is a convenience function to set the input table.
  virtual void SetInput(vtkTable *table);
  virtual void SetInput(vtkTable *table, const char*, const char*)
  {
    this->SetInput(table);
  }

  // Description:
  // Specify a lookup table for the mapper to use.
  void SetLookupTable(vtkScalarsToColors *lut);
  vtkScalarsToColors *GetLookupTable();

  // Description:
  // Create default lookup table. Generally used to create one when none
  // is available with the scalar data.
  virtual void CreateDefaultLookupTable();

  // Description:
  // Turn on/off flag to control whether scalar data is used to color objects.
  vtkSetMacro(ScalarVisibility,int);
  vtkGetMacro(ScalarVisibility,int);
  vtkBooleanMacro(ScalarVisibility,int);

  // Description:
  // When ScalarMode is set to UsePointFieldData or UseCellFieldData,
  // you can specify which array to use for coloring using these methods.
  // The lookup table will decide how to convert vectors to colors.
  void SelectColorArray(vtkIdType arrayNum);
  void SelectColorArray(const char* arrayName);

//BTX
  // Description:
  // Enum containing various marker styles that can be used in a plot.
  enum {
    NONE = 0,
    CROSS,
    PLUS,
    SQUARE,
    CIRCLE,
    DIAMOND
  };
//ETX

  // Description:
  // Get/set the marker style that should be used. The default is none, the enum
  // in this class is used as a parameter.
  vtkGetMacro(MarkerStyle, int);
  vtkSetMacro(MarkerStyle, int);

//BTX
protected:
  vtkMyPlotParallelCoordinates();
  ~vtkMyPlotParallelCoordinates();

  // Description:
  // Generate the requested symbol for the plot
  void GeneraterMarker(int width, bool highlight = false);

  // Description:
  // The marker style that should be used
  int MarkerStyle;
  vtkImageData* Marker;
  vtkImageData* HighlightMarker;

  // Description:
  // Update the table cache.
  bool UpdateTableCache(vtkTable *table);

  // Description:
  // Store a well packed set of XY coordinates for this data series.
  class Private;
  Private* Storage;
  vtkPoints2D* Points;

  vtkMyChartParallelCoordinates* Parent;

  // Description:
  // The point cache is marked dirty until it has been initialized.
  vtkTimeStamp BuildTime;

  // Description:
  // Selected indices coming back from outside the chart this plot is associated with.
  vtkIdTypeArray *HighlightSelection;

  // Description:
  // Lookup Table for coloring points by scalar value
  vtkScalarsToColors *LookupTable;
  vtkUnsignedCharArray *Colors;
  int ScalarVisibility;
  char ColorArrayName[256];

private:
  vtkMyPlotParallelCoordinates(const vtkMyPlotParallelCoordinates &); // Not implemented.
  void operator=(const vtkMyPlotParallelCoordinates &); // Not implemented.

//ETX
};

#endif //__vtkMyPlotParallelCoordinates_h
