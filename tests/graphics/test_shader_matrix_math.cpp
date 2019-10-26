#include "tests/graphics/graphics_config.hpp"

#include <catch2/catch.hpp>


SHADER_STRUCT(TestConsts,
  float4x4 matr;
  float4 row1;
  float4 row2;
  float4 row3;
  float4 row4;
);

SHADER_STRUCT(TestConsts2,
  float4x4 matr1;
  float4x4 matr2;
);

TEST_CASE_METHOD(GraphicsFixture, "verify matrixes end up on gpu correctly") {

  higanbana::ShaderArgumentsLayout argsLayout = gpu().createShaderArgumentsLayout(ShaderArgumentsLayoutDescriptor()
    .readWrite(ShaderResourceType::Buffer, "float", "diff"));

  auto comp = gpu().createComputePipeline(ComputePipelineDescriptor()
  .setInterface(PipelineInterfaceDescriptor()
    .constants<TestConsts>()
    .shaderArguments(0, argsLayout))
  .setShader("verify_matrix_upload")
  .setThreadGroups(uint3(1, 1, 1)));

  auto buffer = gpu().createBuffer(ResourceDescriptor()
    .setFormat(FormatType::Float32)
    .setUsage(ResourceUsage::GpuRW)
    .setCount(8*4)); // 1:1 copy of input, 1x 4x4 float matrix and 4x float4 vectors

  auto bufferUAV = gpu().createBufferUAV(buffer);

  auto argsDesc = ShaderArgumentsDescriptor("computeCopyArgs", argsLayout);
  argsDesc.bind("diff", bufferUAV);

  auto args = gpu().createShaderArguments(argsDesc);

  auto refMat = float4x4({16, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15});

  auto graph = gpu().createGraph();

  auto node = graph.createPass("computeCopy");
  {
    auto binding = node.bind(comp);
    binding.arguments(0, args);
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

// Honestly just documents hlsl code for achieving correct results.
TEST_CASE_METHOD(GraphicsFixture, "test that gpu multiplication is same as cpu") {

  higanbana::ShaderArgumentsLayout argsLayout = gpu().createShaderArgumentsLayout(ShaderArgumentsLayoutDescriptor()
    .readWrite(ShaderResourceType::Buffer, "float", "result"));

  auto comp = gpu().createComputePipeline(ComputePipelineDescriptor()
    .setInterface(PipelineInterfaceDescriptor()
      .constants<TestConsts2>()
      .shaderArguments(0, argsLayout))
    .setShader("matrix_multiplication")
    .setThreadGroups(uint3(1, 1, 1)));

  auto buffer = gpu().createBuffer(ResourceDescriptor()
    .setFormat(FormatType::Float32)
    .setUsage(ResourceUsage::GpuRW)
    .setCount(16)); // 1:1 copy of input, 1x 4x4 float matrix and 4x float4 vectors

  auto bufferUAV = gpu().createBufferUAV(buffer);

  auto refMat = float4x4({16, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15});
  auto refMat2 = float4x4({116, 11, 12, 13, 14, 15, 16, 17, 18, 19, 110, 111, 112, 113, 114, 115});

  auto graph = gpu().createGraph();

  auto node = graph.createPass("computeCopy");
  {
    auto argsDesc = ShaderArgumentsDescriptor("computeCopyArgs", argsLayout);
    argsDesc.bind("result", bufferUAV);

    auto args = gpu().createShaderArguments(argsDesc);

    auto binding = node.bind(comp);
    binding.arguments(0, args);
    TestConsts2 consts{};
    consts.matr1 = refMat;
    consts.matr2 = refMat2;
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

  auto refResult = math::mul(refMat, refMat2);

  REQUIRE(refResult(0) == 2242.f);
  REQUIRE(refResult(1) == 568.f);
  REQUIRE(refResult(2) == 770.f);
  REQUIRE(refResult(3) == 792.f);
  REQUIRE(refResult(4) == 1426.f);
  REQUIRE(refResult(5) == 1024.f);
  REQUIRE(refResult(6) == 1586.f);
  REQUIRE(refResult(7) == 1608.f);
  REQUIRE(refResult(8) == 2466.f);
  REQUIRE(refResult(9) == 1656.f);
  REQUIRE(refResult(10) == 2594.f);
  REQUIRE(refResult(11) == 2632.f);
  REQUIRE(refResult(12) == 3506.f);
  REQUIRE(refResult(13) == 2288.f);
  REQUIRE(refResult(14) == 3602.f);
  REQUIRE(refResult(15) == 3656.f);

  REQUIRE(rbdata[0] == 2242.f);
  REQUIRE(rbdata[1] == 568.f);
  REQUIRE(rbdata[2] == 770.f);
  REQUIRE(rbdata[3] == 792.f);
  REQUIRE(rbdata[4] == 1426.f);
  REQUIRE(rbdata[5] == 1024.f);
  REQUIRE(rbdata[6] == 1586.f);
  REQUIRE(rbdata[7] == 1608.f);
  REQUIRE(rbdata[8] == 2466.f);
  REQUIRE(rbdata[9] == 1656.f);
  REQUIRE(rbdata[10] == 2594.f);
  REQUIRE(rbdata[11] == 2632.f);
  REQUIRE(rbdata[12] == 3506.f);
  REQUIRE(rbdata[13] == 2288.f);
  REQUIRE(rbdata[14] == 3602.f);
  REQUIRE(rbdata[15] == 3656.f);
}

SHADER_STRUCT(TestConsts3,
  float3 vec1;
  uint u1;
  float3 vec2;
  uint u2;
);

TEST_CASE_METHOD(GraphicsFixture, "test vec3 passing to gpu.") {

  higanbana::ShaderArgumentsLayout argsLayout = gpu().createShaderArgumentsLayout(ShaderArgumentsLayoutDescriptor()
    .readOnly(ShaderResourceType::Buffer, "float3", "input")
    .readWrite(ShaderResourceType::Buffer, "float", "result"));

  auto comp = gpu().createComputePipeline(ComputePipelineDescriptor()
    .setInterface(PipelineInterfaceDescriptor()
      .constants<TestConsts3>()
      .shaderArguments(0, argsLayout))
    .setShader("vec3 copy")
    .setThreadGroups(uint3(1, 1, 1)));

  auto buffer = gpu().createBuffer(ResourceDescriptor()
    .setFormat(FormatType::Float32)
    .setUsage(ResourceUsage::GpuRW)
    .setCount(4*3)); // 1:1 copy of input, 1x 4x4 float matrix and 4x float4 vectors

  auto bufferUAV = gpu().createBufferUAV(buffer);

  auto refVec = float3({16, 1, 2});
  auto refVec2 = float3({116, 11, 12});

  auto graph = gpu().createGraph();

  auto node = graph.createPass("computeCopy");
  {
    vector<float3> lol{refVec, refVec2};
    
    auto dyn = gpu().dynamicBuffer(makeMemView(lol), FormatType::Float32RGB);
    auto dyn1 = gpu().dynamicBuffer(makeMemView(lol), FormatType::Float32RGB);
    auto dyn2 = gpu().dynamicBuffer(makeMemView(lol), FormatType::Float32RGB);

    auto argsDesc = ShaderArgumentsDescriptor("computeCopyArgs", argsLayout);
    argsDesc.bind("result", bufferUAV);
    argsDesc.bind("input", dyn);

    auto args = gpu().createShaderArguments(argsDesc);

    auto binding = node.bind(comp);
    binding.arguments(0, args);
    TestConsts3 consts{};
    consts.vec1 = refVec;
    consts.vec2 = refVec2;
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

  auto refResult = math::mul(refVec, refVec2);

  REQUIRE(refResult(0) == 1856.f);
  REQUIRE(refResult(1) == 11.f);
  REQUIRE(refResult(2) == 24.f);

  REQUIRE(rbdata[0] == 1891.f);
  REQUIRE(rbdata[1] == 1891.f);
  REQUIRE(rbdata[2] == 1891.f);
  REQUIRE(rbdata[3] == 1891.f);
  REQUIRE(rbdata[4] == 1891.f);
  REQUIRE(rbdata[5] == 1891.f);

  REQUIRE(rbdata[6] == refVec.x);
  REQUIRE(rbdata[7] == refVec.y);
  REQUIRE(rbdata[8] == refVec.z);

  REQUIRE(rbdata[9] == refVec2.x);
  REQUIRE(rbdata[10] == refVec2.y);
  REQUIRE(rbdata[11] == refVec2.z);
}