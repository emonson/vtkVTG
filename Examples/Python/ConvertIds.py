#!/usr/bin/env python

# Demonstrate how to use the vtkConvertIds class works with point & cell data

import vtk
import sys
sys.path.append("/Users/emonson/Programming/VTK_git/vtkVTG/build/bin")
import libvtkvtgRenderingPython as vtgR

rt = vtk.vtkRTAnalyticSource()
rt.SetWholeExtent(-3,3,-3,3,-3,3)
ids = vtk.vtkIdFilter()
# Ids filter destroys RTData attributes!!! :(
ids.SetInputConnection(rt.GetOutputPort(0))
ids.SetIdsArrayName('filter_ids')
ids.Update()

data = ids.GetOutputDataObject(0)
numPts = data.GetPointData().GetArray('filter_ids').GetNumberOfTuples()

att = vtk.vtkIntArray()
# att = vtk.vtkIdTypeArray()
att.SetName('orig_id_values')
att.SetNumberOfComponents(1)
att.SetNumberOfValues(numPts)
for ii in range(numPts):
	att.SetValue(ii,ii)

data.GetPointData().AddArray(att)
# data.GetPointData().SetActiveScalars('orig_id_values')

conv = vtgR.vtkConvertIds()
# conv.SetInputArrayToProcess(0,0,0,0,'orig_id_values')
# conv.SetInputArrayToProcess(0,0,0,0,'filter_ids')		# point data
conv.SetInputArrayToProcess(0,0,0,1,'filter_ids')			# cell data
conv.SetOutputArrayName('global_ids')
conv.SetInput(data)
conv.Update()

print conv.GetOutputDataObject(0)