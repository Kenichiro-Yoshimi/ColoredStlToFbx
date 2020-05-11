#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPolyDataNormals.h>
#include <vtkSmartPointer.h>
#include <vtkSTLReader.h>
#include <vtkXMLPolyDataWriter.h>

#include "Common.h"
#include <fbxsdk.h>

#include <map>

// Function prototypes.
bool CreateScene(vtkPolyData *pd, FbxScene* pScene);
FbxNode* CreateMeshWtihMaterials(vtkPolyData *pd, FbxScene* pScene, const char* pName = "FBXObject");
void CreateMaterials(FbxScene* pScene, FbxMesh* pMesh, int numLabels);

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    std::cout << "Required parameters: input(*.stl) output(*.fbl)" << std::endl;
    return EXIT_FAILURE;
  }

  std::string inputFileName = argv[1];
  std::string outputFileName = argv[2];

  auto reader = vtkSmartPointer<vtkSTLReader>::New();
  reader->SetFileName(inputFileName.c_str());
  reader->ScalarTagsOn();
  reader->Update();

  vtkPolyData* pd = reader->GetOutput();

  FbxManager* lSdkManager = nullptr;
  FbxScene* lScene = nullptr;
  bool lResult;

  // Prepare the FBX SDK.
  InitializeSdkObjects(lSdkManager, lScene);

  // Create the scene.
  lResult = CreateScene(pd, lScene);

  if (lResult == false)
  {
    FBXSDK_printf("\n\nAn error occurred while creating the scene...\n");
    DestroySdkObjects(lSdkManager, lResult);
    return EXIT_FAILURE;
  }

  // Save the scene.
  int lFormat = lSdkManager->GetIOPluginRegistry()->FindWriterIDByDescription("FBX binary (*.fbx)");
  lResult = SaveScene(lSdkManager, lScene, outputFileName.c_str(), lFormat);

  if (lResult == false)
  {
    FBXSDK_printf("\n\nAn error occured while saving the scene...\n");
    DestroySdkObjects(lSdkManager, lResult);
    return EXIT_FAILURE;
  }

  // Destroy all objects created by the FBX SDK.
  DestroySdkObjects(lSdkManager, lResult);

  return EXIT_SUCCESS;
}

bool CreateScene(vtkPolyData *pd, FbxScene* pScene)
{
  FbxNode* lNode = CreateMeshWtihMaterials(pd, pScene);

  // Build the node tree.
  FbxNode* lRootNode = pScene->GetRootNode();
  lRootNode->AddChild(lNode);

  return true;
}

// Create mesh with materials.
FbxNode* CreateMeshWtihMaterials(vtkPolyData *pd, FbxScene* pScene, const char* pName)
{
  FbxMesh* lMesh = FbxMesh::Create(pScene, pName);

  vtkIdType numPnts = pd->GetNumberOfPoints();
  vtkIdType numCells = pd->GetNumberOfCells();
  std::cout << "number of points in input surface: " << numPnts << std::endl;
  std::cout << "number of cells in input surface: " << numCells << std::endl;

  lMesh->InitControlPoints(numPnts);
  FbxVector4* controlPoints = lMesh->GetControlPoints();
  for (vtkIdType ptId = 0; ptId < numPnts; ++ptId)
  {
    double point[3];
    pd->GetPoint(ptId, point);
    controlPoints[ptId] = FbxVector4(point[0], point[1], point[2]);
  }

  // Check normals
  vtkFloatArray* normals = normals = dynamic_cast<vtkFloatArray*>(pd->GetPointData()->GetArray("Normals"));
  if (!normals)
  {
    // Calculate normal vectors
    auto normalFilter = vtkSmartPointer<vtkPolyDataNormals>::New();
    normalFilter->SetInputData(pd);
    normalFilter->SplittingOff();
    normalFilter->ConsistencyOn();
		normalFilter->ComputePointNormalsOn();
		normalFilter->ComputeCellNormalsOff();
		normalFilter->Update();
    pd->ShallowCopy(normalFilter->GetOutput());
  }
  normals = dynamic_cast<vtkFloatArray*>(pd->GetPointData()->GetArray("Normals"));

  // Specify normals per control point.
  FbxGeometryElementNormal* lNormalElement = lMesh->CreateElementNormal();
  lNormalElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
  lNormalElement->SetReferenceMode(FbxGeometryElement::eDirect);
  for (vtkIdType ptId = 0; ptId < numPnts; ++ptId)
  {
    double *normal = normals->GetTuple(ptId);
    lNormalElement->GetDirectArray().Add(FbxVector4(normal[0], normal[1], normal[2]));
  }

  // Set material mapping.
  FbxGeometryElementMaterial* lMaterialElement = lMesh->CreateElementMaterial();
  lMaterialElement->SetMappingMode(FbxGeometryElement::eByPolygon);
  lMaterialElement->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

  // Check labels in vtkPolyData for material
  vtkFloatArray* labels = dynamic_cast<vtkFloatArray*>(pd->GetCellData()->GetArray("STLSolidLabeling"));
  std::map<int, int> labelToIndex;
  int index = 0;
  for (vtkIdType cId = 0; cId < numCells; ++cId)
  {
    int label = labels->GetTuple1(cId);

    if (labelToIndex.find(label) == labelToIndex.end())
    {
      labelToIndex[label] = index++;
    }
  }

  vtkIdList *ptIds = vtkIdList::New();
  // Create polygons. Assign material indices.
  for (vtkIdType cId = 0; cId < numCells; ++cId)
  {
    int label = labels->GetTuple1(cId);
    lMesh->BeginPolygon(labelToIndex[label]);

    pd->GetCellPoints(cId, ptIds);
    for (vtkIdType i = 0; i < ptIds->GetNumberOfIds(); ++i)
    {
      lMesh->AddPolygon(ptIds->GetId(i));
    }
    lMesh->EndPolygon();
  }
  ptIds->Delete();

  FbxNode* lNode = FbxNode::Create(pScene, pName);

  lNode->SetNodeAttribute(lMesh);

  CreateMaterials(pScene, lMesh, labelToIndex.size());

  return lNode;
}

// Create materials according to the number of labels in vtkPolyData.
void CreateMaterials(FbxScene* pScene, FbxMesh* pMesh, int numLabels)
{
  vtkMath::RandomSeed(8775070);
  for (int i = 0; i < numLabels; ++i)
  {
    FbxString lMaterialName = "material";
    FbxString lShadingName = "Phong";
    lMaterialName += i;
    FbxDouble3 lBlack(0.0, 0.0, 0.0);
    FbxDouble3 lRed(1.0, 0.0, 0.0);
    FbxDouble3 lColor = {
        vtkMath::Random(64, 255) / 255.0f,
        vtkMath::Random(64, 255) / 255.0f,
        vtkMath::Random(64, 255) / 255.0f};
    FbxSurfacePhong *lMaterial = FbxSurfacePhong::Create(pScene, lMaterialName.Buffer());

    // Generate primary and secondary colors.
    lMaterial->Emissive.Set(lBlack);
    lMaterial->Ambient.Set(lRed);
    lMaterial->Diffuse.Set(lColor);
    lMaterial->TransparencyFactor.Set(0.0);
    lMaterial->ShadingModel.Set(lShadingName);
    lMaterial->Shininess.Set(0.5);

    // Get the node of mesh, add material for it.
    FbxNode* lNode = pMesh->GetNode();
    if (lNode)
    {
      lNode->AddMaterial(lMaterial);
    }
  }
}
