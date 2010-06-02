/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConvertIds.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkConvertIds.h"

#include "vtkAbstractArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

vtkStandardNewMacro(vtkConvertIds);

vtkConvertIds::vtkConvertIds()
{
  this->OutputArrayName = 0;
  this->SetOutputArrayName("ConvertedIds");

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0,0,0, vtkDataObject::FIELD_ASSOCIATION_POINTS,
    vtkDataSetAttributes::SCALARS);

}

vtkConvertIds::~vtkConvertIds()
{
  this->SetOutputArrayName(0);
}

int vtkConvertIds::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject *output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  
  // Copy input to output
  output->ShallowCopy(input);

  // Determine the type of output data
  vtkGraph* graph = vtkGraph::SafeDownCast(output);
  vtkDataSet* dataSet = vtkDataSet::SafeDownCast(output);
  vtkTable* table = vtkTable::SafeDownCast(output);

  // Abstract array can also contain vtkVariantArray and vtkStringArray
  vtkAbstractArray *inputAbstractArray = this->GetInputAbstractArrayToProcess(0, inputVector);
  vtkIdType numComps = inputAbstractArray->GetNumberOfComponents();
  vtkDataArray *inputArray = 0;
  
  // Figure out information about the input array
  vtkInformation* arrayInfo = this->GetInputArrayInformation(0);
  int fieldAssoc = arrayInfo->Get(vtkDataObject::FIELD_ASSOCIATION());
  
  // Some reasons to reject processing and just return
  if (!inputAbstractArray)
    {
    vtkDebugMacro("Input array not specified");
    return 1;
    }
  if (fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_NONE ||
      fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS)
    {
    vtkDebugMacro("Field association NONE or POINTS_THEN_CELLS");
    return 1;
    }
  if (numComps != 1)
    {
    vtkDebugMacro("Input array number of components != 1");
    return 1;
    }

  // Test if array is vtkDataArray subclass, and assign if so
  if (inputAbstractArray->IsA("vtkDataArray"))
    {
    inputArray = vtkDataArray::SafeDownCast(inputAbstractArray);
    }

  // Already an IdTypeArray, so just set as ActiveGlobalIds and return
  // Should put an option for pedigree ids here, also, as well as below
  if (vtkIdTypeArray::SafeDownCast(inputArray))
    {
		if (fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_POINTS)
			{
			dataSet->GetPointData()->SetActiveGlobalIds(inputArray->GetName());
			}
		else if (fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_CELLS)
			{
			dataSet->GetCellData()->SetActiveGlobalIds(inputArray->GetName());
			}
		else if (fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_VERTICES)
			{
			graph->GetVertexData()->SetActiveGlobalIds(inputArray->GetName());
			}
		else if (fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_EDGES)
			{
			graph->GetEdgeData()->SetActiveGlobalIds(inputArray->GetName());
			}
		else if (fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_ROWS)
			{
			// Not sure about this next one...
			table->GetRowData()->SetActiveGlobalIds(inputArray->GetName());
			}
		else
			{
			vtkDebugMacro("Unsupported field associate, so didn't assign global ids");
			}
    return 1;
    }
  
  // New array is always vtkIdTypeArray
  vtkSmartPointer<vtkIdTypeArray> outputArray = vtkSmartPointer<vtkIdTypeArray>::New();
  outputArray->SetName(this->OutputArrayName);

  // Relying here on good implementation of DeepCopy...
  if(inputAbstractArray->IsA("vtkDataArray"))
    {
    outputArray->DeepCopy(inputArray);
    }
  else
    {
    // Otherwise should maybe be rejecting float or double arrays
    //   so cast to idtype will give expected results
    //   and comparing number
    //   of bytes (GetDataTypeSize()) to make sure not overflowing outputArray
    vtkIdType numTuples = inputAbstractArray->GetNumberOfTuples();
    outputArray->SetNumberOfComponents(numComps);
    outputArray->SetNumberOfTuples(numTuples);
    for (vtkIdType i = 0; i < numTuples; ++i)
      {
      outputArray->SetValue(i, static_cast<vtkIdType>
                                  (inputAbstractArray->GetVariantValue(i).ToLong()));
      }
    }

  // Now need to add this new array in the proper way to the proper data type
  // NOTE: Should put in an option for pedigree ids

  if (fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
    dataSet->GetPointData()->AddArray(outputArray);
    dataSet->GetPointData()->SetActiveGlobalIds(this->OutputArrayName);
    }
  else if (fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_CELLS)
    {
    dataSet->GetCellData()->AddArray(outputArray);
    dataSet->GetCellData()->SetActiveGlobalIds(this->OutputArrayName);
    }
  else if (fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_VERTICES)
    {
    graph->GetVertexData()->AddArray(outputArray);
    graph->GetVertexData()->SetActiveGlobalIds(this->OutputArrayName);
    }
  else if (fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_EDGES)
    {
    graph->GetEdgeData()->AddArray(outputArray);
    graph->GetEdgeData()->SetActiveGlobalIds(this->OutputArrayName);
    }
  else if (fieldAssoc == vtkDataObject::FIELD_ASSOCIATION_ROWS)
    {
    table->AddColumn(outputArray);
    // Not sure about this next one...
    table->GetRowData()->SetActiveGlobalIds(this->OutputArrayName);
    }
  else
    {
    vtkDebugMacro("Didn't assign new IDs array to any data structure");
    }

  return 1;
}

int vtkConvertIds::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  // This algorithm may accept a vtkPointSet or vtkGraph.
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;  
}

void vtkConvertIds::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Output array name: ";
  if (this->OutputArrayName)
    {
    os << this->OutputArrayName << endl;
    }
  else
    {
    os << "(none)" << endl;
    }
}
