#include "basic_shared.h"

[[vk::binding(0)]]
ConstantBuffer<BasicRoot> g_root : register(b0);

struct VOut {
  float4 position : SV_Position;
  float color;
};

[shader("vertex")]
VOut vs_main(uint vert_idx: SV_VertexID) {
  VOut output;
  if (vert_idx == 0) {
    output.position = float4(-0.5, -0.5, 0, 1);
  } else if (vert_idx == 1) {
    output.position = float4(0.5, -0.5, 0, 1);
  } else {
    output.position = float4(0, 0.5, 0, 1);
  }

  // output.color = 0.5;
  output.color = g_root.color_x;
  // test
  // test2

  return output;
}

[shader("fragment")]
float4 fs_main(VOut input) : SV_Target0 {
  return float4(input.color, input.color, input.color, 1);
}
