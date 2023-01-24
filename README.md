# Hulk-engine

Small 3d rendered i am building to teach myself 3d programming using d3d12. The code is influenced by the book "introduction to 3D programming using directX 12" of Frank d Luna.


It currently loads two scenes:

## Shapes scene :
- supports phong lighting,stencil reflection and planar shadows (for the skull object only)

![image](https://user-images.githubusercontent.com/56231582/214307328-475bf0b7-2092-4ea4-acde-b498e160cfeb.png)

## Wave scene:
- supports wave simulation on cpu using a vertex buffer on the upload heap.
- also implements blending for the water, alpha testing and Billboards using geometry shader.

![image](https://user-images.githubusercontent.com/56231582/214309902-1aafdaa7-2aac-4e94-913a-36e3cc16705d.png)



