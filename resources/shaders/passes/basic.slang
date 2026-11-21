typealias vec2 = float2;
typealias vec3 = float3;
typealias vec4 = float4;

// StructuredBuffer<float> buffer0;
// StructuredBuffer<float> buffer1;
// RWStructuredBuffer<float> result;

[shader("compute")]
[numthreads(1, 1, 1)]
void main(uint3 dispatch_thread_id: SV_DispatchThreadID) {
  uint index = dispatch_thread_id.x;
}
