#include "shader_shared.hlsli"

struct VOut {
  float4 position : SV_Position;
};

VOut vs_main(uint vert_idx : SV_VertexID) {
  VOut output;
  if (vert_idx == 0) {
    output.position = float4(-0.5, -0.5, 0, 1);
  } else if (vert_idx == 1) {
    output.position = float4(0.5, -0.5, 0, 1);
  } else {
    output.position = float4(0, 0.5, 0, 1);
  }
  return output;
}

float4 fs_main(VOut input) : SV_Target0 {
  return float4(1, 1, 1, 1);
}
