#include "basic_shared.h"
#include "shader_defines.h"

GPU_ROOT(BasicRoot, g_root)

[shader("compute")]
[numthreads(1, 1, 1)]
void comp_main(uint3 dispatch_thread_id: SV_DispatchThreadID) {
  if (dispatch_thread_id.x == 0 && dispatch_thread_id.y == 0 && dispatch_thread_id.z == 0) {
    g_root.output_result[0] = g_root.color_x;
  }
}
