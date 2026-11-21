typealias vec2 = float2;
typealias vec3 = float3;
typealias vec4 = float4;

[[shader("compute")]] [[numthreads(16, 16, 1)]]
void main(uint2 dispatch_thread_id : SV_DispatchThreadID) {

}

