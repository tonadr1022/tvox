
typealias vec2 = float2;
typealias vec3 = float3;
typealias vec4 = float4;

struct VOut {
  vec4 position : SV_Position;
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
  return output;
}

[shader("fragment")]
float4 fs_main(VOut input) : SV_Target0 {
  return float4(1, 1, 1, 1);
}
