#include "ModelLoad.hpp"
#include "core/Logger.hpp"
#include "graphics/util/UploadQueue.hpp"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/gtx/string_cast.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"

namespace gfx {

namespace {
const char* cgltf_result_to_string(cgltf_result result) {
  switch (result) {
  case cgltf_result_success:
    return "cgltf_result_success";
  case cgltf_result_data_too_short:
    return "cgltf_result_data_too_short";
  case cgltf_result_unknown_format:
    return "cgltf_result_unknown_format";
  case cgltf_result_invalid_json:
    return "cgltf_result_invalid_json";
  case cgltf_result_invalid_gltf:
    return "cgltf_result_invalid_gltf";
  case cgltf_result_invalid_options:
    return "cgltf_result_invalid_options";
  case cgltf_result_file_not_found:
    return "cgltf_result_file_not_found";
  case cgltf_result_io_error:
    return "cgltf_result_io_error";
  case cgltf_result_out_of_memory:
    return "cgltf_result_out_of_memory";
  case cgltf_result_legacy_gltf:
    return "cgltf_result_legacy_gltf";
  case cgltf_result_max_enum:
    return "cgltf_result_max_enum";
    break;
  }
  FATAL_IF(0);
}

struct ImageLoad {
  uint32_t x{}, y{}, z{};
  void* data;
};

std::optional<ImageLoad> load_image(const std::filesystem::path& path) {
  std::optional<ImageLoad> result{};
  int x{}, y{}, channels{};
  void* data = stbi_load(path.c_str(), &x, &y, &channels, 4);
  if (!data) {
    return result;
  }
  return result;
}

} // namespace

Model load_model(rhi::Device& device, UploadQueue& upload_queue,
                 IndexAllocator& texture_descriptor_index_allocator,
                 const std::filesystem::path& path) {
  Model model;
  auto parent_dir = path.parent_path();

  cgltf_options options{};
  cgltf_data* data{};
  cgltf_result result = cgltf_parse_file(&options, path.string().c_str(), &data);
  if (result != cgltf_result_success) {
    LERROR("Failed to load gltf model: {}", cgltf_result_to_string(result));
    return model;
  }

  result = cgltf_load_buffers(&options, data, path.string().c_str());
  if (result != cgltf_result_success) {
    LERROR("Failed to load gltf buffers: {}", cgltf_result_to_string(result));
    return model;
  }

  std::vector<float> scratch;
  for (size_t mesh_i = 0; mesh_i < data->meshes_count; mesh_i++) {
    const auto& mesh = data->meshes[mesh_i];
    for (size_t primitive_i = 0; primitive_i < mesh.primitives_count; primitive_i++) {
      const auto& primitive = mesh.primitives[primitive_i];
      std::vector<Vertex> vertices;
      for (size_t attribute_i = 0; attribute_i < primitive.attributes_count; attribute_i++) {
        const auto& attribute = primitive.attributes[attribute_i];
        const auto& accessor = attribute.data;
        scratch.resize(accessor->count * 4);
        size_t components_per_element = cgltf_num_components(accessor->type);
        size_t element_count = accessor->count;
        if (vertices.empty()) {
          vertices.resize(element_count);
        } else {
          ASSERT(element_count == vertices.size());
        }
        size_t total_comopnents = components_per_element * element_count;
        switch (attribute.type) {
        case cgltf_attribute_type_position: {
          ASSERT(cgltf_accessor_unpack_floats(accessor, scratch.data(), total_comopnents) ==
                 total_comopnents);
          ASSERT(components_per_element == 3);
          for (size_t i = 0; i < element_count; i++) {
            vertices[i].position.x = scratch[i * 3];
            vertices[i].position.y = scratch[i * 3 + 1];
            vertices[i].position.z = scratch[i * 3 + 2];
          }
          break;
        }
        case cgltf_attribute_type_texcoord: {
          ASSERT(cgltf_accessor_unpack_floats(accessor, scratch.data(), total_comopnents) ==
                 total_comopnents);
          for (size_t i = 0; i < element_count; i++) {
            vertices[i].uv.x = scratch[i * 2];
            vertices[i].uv.y = scratch[i * 2 + 1];
          }
          break;
        }

        case cgltf_attribute_type_invalid:
        case cgltf_attribute_type_normal:
        case cgltf_attribute_type_tangent:
        case cgltf_attribute_type_color:
        case cgltf_attribute_type_joints:
        case cgltf_attribute_type_weights:
        case cgltf_attribute_type_custom:
        case cgltf_attribute_type_max_enum:
          break;
        }
      }

      for (auto v : vertices) {
        model.vertices.emplace_back(v);
      }
      const auto& index_accessor = primitive.indices;
      std::vector<uint32_t> indices(index_accessor->count);
      ASSERT(cgltf_accessor_unpack_indices(primitive.indices, indices.data(), sizeof(uint32_t),
                                           index_accessor->count) == index_accessor->count);
      model.indices = indices;
    }
  }

  for (size_t image_i = 0; image_i < data->images_count; image_i++) {
    const auto& image = data->images[image_i];
    auto& texture = model.textures.emplace_back();
    int x{}, y{}, channels{};
    auto dir = parent_dir / image.uri;
    int desired_channels = 4;
    void* data = stbi_load(dir.string().c_str(), &x, &y, &channels, desired_channels);
    ASSERT(data);
    TextureUploadDesc upload_desc{
        .src_size = {x, y, 1},
        .data = data,
        .array_layers = 1,
        .mip_count = 1,
        .bytes_per_row = static_cast<uint32_t>(x * desired_channels),
    };
    device.create_texture(texture, rhi::TextureDesc{
                                       .width = static_cast<uint32_t>(x),
                                       .height = static_cast<uint32_t>(y),
                                       .depth = 1,
                                       .array_length = 1,
                                       .mip_count = 1,
                                       .format = rhi::Format::Rgba8Unorm,
                                       .usage = rhi::TextureUsage::ShaderRead,
                                   });
    upload_queue.upload_to_texture(texture, upload_desc);
    texture.index = texture_descriptor_index_allocator.alloc();
    device.write_texture_desc({.texture_type = texture.desc.type}, texture, texture.index);
    stbi_image_free(data);
  }

  cgltf_free(data);

  return model;
}

} // namespace gfx
