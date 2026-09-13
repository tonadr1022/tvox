#include "shader_shared.h"

struct VOut {
  float4 position : SV_Position;
};

float4 main(VOut input) { return float4(1, 1, 1, 1); }
