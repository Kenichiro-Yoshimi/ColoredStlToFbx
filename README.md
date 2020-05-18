# Convert Colored STL to FBX format with materials based on the color labels in STL file

- This tool is convenient in the use of [Quad Remesher](https://exoside.com/quadremesher/) for Blender. This allows you to assign the mesh edges along the boundaries of Materials according to the colored labels in STL file.

![quad_remesher_in_blender](https://user-images.githubusercontent.com/5258664/81551147-50fb4280-93bc-11ea-8749-98344331e158.png)

- It is also possible to set vtp file format as the input for the converter.

## Usage

Usage:

    ColoredStlToFbx.exe input.stl output.fbx

In loading .vtp file format as the input, labelName should be required to identify material array name.

    ColoredStlToFbx.exe input.vtp output.fbx RegionId
