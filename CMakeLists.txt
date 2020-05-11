# General project setup
cmake_minimum_required (VERSION 3.3)
project (ColoredStlToFbx)
cmake_policy(SET CMP0074 NEW)

# Find
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
find_package(FBX REQUIRED)
add_definitions(-DFBX_VERSION_MAJOR=${FBX_VERSION_MAJOR})

find_package(VTK COMPONENTS 
  vtkCommonCore
  vtkIOGeometry
  vtkIOImage
  vtkIOLegacy
  vtkIOXML
  vtkInteractionStyle
  vtkRenderingCore
  vtkRenderingFreeType
  vtkRenderingOpenGL2 QUIET)
if (NOT VTK_FOUND)
  message("Skipping VisualizeVTP: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif()
message (STATUS "VTK_VERSION: ${VTK_VERSION}")
if (VTK_VERSION VERSION_LESS "8.90.0")
  # old system
  include(${VTK_USE_FILE})
  add_executable(ColoredStlToFbx MACOSX_BUNDLE
  ColoredStlToFbx.cxx
  Common.cxx )
  target_link_libraries(ColoredStlToFbx PRIVATE ${VTK_LIBRARIES} ${FBX_LIBRARIES})
else ()
  # include all components
  add_executable(ColoredStlToFbx MACOSX_BUNDLE
  ColoredStlToFbx.cxx
  Common.cxx )
  target_link_libraries(ColoredStlToFbx PRIVATE ${VTK_LIBRARIES} ${FBX_LIBRARIES})
  # vtk_module_autoinit is needed
  vtk_module_autoinit(
    TARGETS ColoredStlToFbx
    MODULES ${VTK_LIBRARIES}
    )
endif () 
target_include_directories(ColoredStlToFbx PUBLIC SYSTEM ${FBX_INCLUDE_DIRS})
