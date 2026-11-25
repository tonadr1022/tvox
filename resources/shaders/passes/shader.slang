

#define GPU_ROOT(type, var_name)                                                                   \
  [[vk::binding(0)]]                                                                               \
  ConstantBuffer<type> var_name : register(b0);

[[vk::binding(1)]]
ConstantBuffer<uint64_t> g_texture_desc_heap_base : register(b1);

[ForceInline]
T gpu_texture_from_resource_id<T : IOpaqueDescriptor>(uint64_t id) {
  __requirePrelude(
      R"(template<typename T> T gpu_texture_from_resource_id_impl(ulong id) { return *reinterpret_cast<thread T*>(&id); })");
  __intrinsic_asm "gpu_texture_from_resource_id_impl<$TR>($0)";
}

[ForceInline]
T gpu_texture<T : IOpaqueDescriptor>(uint index) {
  return gpu_texture_from_resource_id<T>(g_texture_desc_heap_base + uint64_t(index));
}
