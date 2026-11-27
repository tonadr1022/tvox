#include "forward_pass_shared.h"
#include "shader.slang"

GPU_ROOT(ForwardRoot, g_root)

struct VOut {
  float4 position : SV_Position;
  float2 uv;
  float color;
};

[shader("vertex")]
VOut vs_main(uint vert_idx: SV_VertexID) {
  VOut output;
  Vertex vertex = g_root.vertices[vert_idx];
  output.position = mul(g_root.view_proj, float4(vertex.position, 1.0));
  output.color = g_root.output_result[0];
  output.uv = vertex.uv;

  return output;
}

[shader("fragment")]
float4 fs_main(VOut input) : SV_Target0 {
  Texture2D<float3> tex = gpu_texture<Texture2D<float3>>(g_root.albedo_idx);
  SamplerState sampler = gpu_sampler(0);
  float3 albedo = tex.Sample(sampler, input.uv);
  return float4(albedo, 1);
}
