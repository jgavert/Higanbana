#include "tests/graphics/graphics_config.hpp"

#include <catch2/catch.hpp>


SHADER_STRUCT(TestConsts,
  float4x4 matr;
  float4 row1;
  float4 row2;
  float4 row3;
  float4 row4;
);

TEST_CASE_METHOD(GraphicsFixture, "create readback in graph") {

  higanbana::ShaderInputDescriptor intrface = ShaderInputDescriptor()
    .constants<TestConsts>()
    .readWrite(ShaderResourceType::Buffer, "float", "diff");

  auto comp = gpu().createComputePipeline(ComputePipelineDescriptor()
  .setLayout(intrface)
  .setShader("verify_matrix_upload")
  .setThreadGroups(uint3(1, 1, 1)));

  auto buffer = gpu().createBuffer(ResourceDescriptor()
    .setFormat(FormatType::Float32)
    .setUsage(ResourceUsage::GpuRW)
    .setCount(8*4)); // 1:1 copy of input, 1x 4x4 float matrix and 4x float4 vectors

  auto bufferUAV = gpu().createBufferUAV(buffer);

  auto refMat = float4x4({16, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15});

  auto graph = gpu().createGraph();

  auto node = graph.createPass("computeCopy");
  {
    auto binding = node.bind(comp);
    binding.bind("diff", bufferUAV);
    TestConsts consts{};
    consts.matr = refMat;
    consts.row1 = refMat.row(0);
    consts.row2 = refMat.row(1);
    consts.row3 = refMat.row(2);
    consts.row4 = refMat.row(3);
    binding.constants(consts);
    node.dispatch(binding, uint3(1, 1, 1));
  }
  auto asyncReadback = node.readback(buffer);
  graph.addPass(std::move(node));
  gpu().submit(graph);
  gpu().waitGpuIdle();

  REQUIRE(asyncReadback.ready()); // result should be ready at some point after gpu idle

  auto rb = asyncReadback.get();
  auto rbdata = rb.view<float>();

  REQUIRE(rbdata[0] == 16.f);
  REQUIRE(rbdata[1] == 1.f);
  REQUIRE(rbdata[2] == 2.f);
  REQUIRE(rbdata[3] == 3.f);
  REQUIRE(rbdata[4] == 4.f);
  REQUIRE(rbdata[5] == 5.f);
  REQUIRE(rbdata[6] == 6.f);
  REQUIRE(rbdata[7] == 7.f);
  REQUIRE(rbdata[8] == 8.f);
  REQUIRE(rbdata[9] == 9.f);
  REQUIRE(rbdata[10] == 10.f);
  REQUIRE(rbdata[11] == 11.f);
  REQUIRE(rbdata[12] == 12.f);
  REQUIRE(rbdata[13] == 13.f);
  REQUIRE(rbdata[14] == 14.f);
  REQUIRE(rbdata[15] == 15.f);
  REQUIRE(rbdata[16 + 0] == 16.f);
  REQUIRE(rbdata[16 + 1] == 1.f);
  REQUIRE(rbdata[16 + 2] == 2.f);
  REQUIRE(rbdata[16 + 3] == 3.f);
  REQUIRE(rbdata[16 + 4] == 4.f);
  REQUIRE(rbdata[16 + 5] == 5.f);
  REQUIRE(rbdata[16 + 6] == 6.f);
  REQUIRE(rbdata[16 + 7] == 7.f);
  REQUIRE(rbdata[16 + 8] == 8.f);
  REQUIRE(rbdata[16 + 9] == 9.f);
  REQUIRE(rbdata[16 + 10] == 10.f);
  REQUIRE(rbdata[16 + 11] == 11.f);
  REQUIRE(rbdata[16 + 12] == 12.f);
  REQUIRE(rbdata[16 + 13] == 13.f);
  REQUIRE(rbdata[16 + 14] == 14.f);
  REQUIRE(rbdata[16 + 15] == 15.f);
}