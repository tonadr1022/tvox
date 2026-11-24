#include "forward_pass_shared.h"

GPU_ROOT(ForwardRoot, g_root)

struct VOut {
  float4 position : SV_Position;
  float color;
};

[shader("vertex")]
VOut vs_main(uint vert_idx: SV_VertexID) {
  VOut output;
  Vertex vertex = g_root.vertices[vert_idx];
  output.position = float4(vertex.position, 1.0);
  output.color = g_root.output_result[0];

  return output;
}

[shader("fragment")]
float4 fs_main(VOut input) : SV_Target0 {
  return float4(input.color, input.color, input.color, 1);
}
