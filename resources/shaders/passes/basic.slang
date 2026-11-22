
#include "basic_shared.h"

[[vk::binding(0)]]
ConstantBuffer<BasicRoot> g_root : register(b0);

[shader("compute")]
[numthreads(1, 1, 1)]
void comp_main(uint3 dispatch_thread_id: SV_DispatchThreadID) {
  if (dispatch_thread_id.x == 0 && dispatch_thread_id.y == 0 && dispatch_thread_id.z == 0) {
    g_root.output_result[0] = g_root.color_x;
  }
}
