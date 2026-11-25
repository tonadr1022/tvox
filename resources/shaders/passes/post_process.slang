#include "shader.slang"
#include "post_process_shared.h"

GPU_ROOT(PostProcessRoot, g_root)

struct VOut {
  float4 pos : SV_Position;
  float2 uv : TEXCOORD0;
};

[shader("vertex")]
VOut vs_main(uint vert_idx: SV_VertexID) {
  VOut o;
  o.uv = float2((vert_idx << 1) & 2, vert_idx & 2);
  o.pos = float4(o.uv.x * 2.0 - 1.0, 1.0 - o.uv.y * 2.0, 0.0, 1.0);
  return o;
}

[shader("fragment")]
float4 fs_main(VOut input) : SV_Target0 {
  Texture2D<float3> tex = gpu_texture<Texture2D<float3>>(0);
  int3 location = int3(input.pos.xyz);
  float3 color = tex.Load(location);
  color.x *= (g_root.mult + 1.0) * 0.5;
  return float4(color, 1.0);
}
