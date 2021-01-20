#include "tests/graphics/graphics_config.hpp"
#include <catch2/catch_all.hpp>



TEST_CASE_METHOD(GraphicsFixture, "Build Bottomlevel acceleration structure") {
  /*
  We need info of all possible geometry going into bottomlevel

  Mesh/vertice clumb == one bottomlevel acceleration structure

  basically:
    GeometryInstance:
      D3D12_GPU_VIRTUAL_ADDRESS Transform3x4;
      DXGI_FORMAT IndexFormat;
      DXGI_FORMAT VertexFormat;
      UINT IndexCount;
      UINT VertexCount;
      D3D12_GPU_VIRTUAL_ADDRESS IndexBuffer;
      D3D12_GPU_VIRTUAL_ADDRESS_AND_STRIDE VertexBuffer;

      which can be sort of summarized to
      indices = BufferIBV and vertices = BufferSRV
      Transform... is harder, meshes might have default transform so this is???, maybe forget for now =D
    
      Then have to tell flag also
      None, or is it opaque or No_duplicate_anyhit_invocation (?)

      Type defaulted to triangles for now.

    Then we need to specify how the AccelerationStructure is built... waait, this is runtime information and affects the size of things... ugh
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE,
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE,
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION,
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD,
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY,
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE
    honestly sea of choices which all matter a lot
      Question, When do we want to rebuild a bottomlevel structure?
        Reusing the memory?
        Oh crap, static vs dynamic ...? What is dynamic here, vertices don't change right?
    Is it a Top or Bottomlevel
      - this actually forces my implementation to be
        - BottomLevelAccelerationStructure
        - TopLevelAccelerationStructure

    How many instances of Geometries with NumDescs in d3d12, So we can have many GeometryInstance descriptions added to this.

    And thats it

    backend:
      Query size from driver -> allocate a buffer that will be big enough
    
    runtime:
      actually building the bottomlevel, we will need scratch buffer to work with which is same or greater than the acceleration structure itself.

    BottomLevelAccelerationStructure blas = device.createBottomLevelAccelerationStructure(desc::BottomLevelAccelerationStructures()
      .setGeometries()
      .set)


  */
}

TEST_CASE_METHOD(GraphicsFixture, "Build Bottomlevel+toplevel acceleration structure") {
}
TEST_CASE_METHOD(GraphicsFixture, "Rebuild toplevel only") {
}
TEST_CASE_METHOD(GraphicsFixture, "Raytrace?") {
}