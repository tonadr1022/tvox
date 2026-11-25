#include "ModelLoad.hpp"
#include "core/Logger.hpp"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/gtx/string_cast.hpp>

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
} // namespace

Model load_model(const std::filesystem::path& path) {
  Model model;

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

  for (size_t mesh_i = 0; mesh_i < data->meshes_count; mesh_i++) {
    const auto& mesh = data->meshes[mesh_i];
    for (size_t primitive_i = 0; primitive_i < mesh.primitives_count; primitive_i++) {
      const auto& primitive = mesh.primitives[primitive_i];
      for (size_t attribute_i = 0; attribute_i < primitive.attributes_count; attribute_i++) {
        const auto& attribute = primitive.attributes[attribute_i];
        switch (attribute.type) {
        case cgltf_attribute_type_position: {
          const auto& accessor = attribute.data;
          size_t floats_per_element = cgltf_num_components(accessor->type);
          size_t element_count = accessor->count;
          size_t total_num_floats = floats_per_element * element_count;
          std::vector<glm::vec3> vertices(element_count);
          ASSERT(cgltf_accessor_unpack_floats(accessor, &vertices.data()->x, total_num_floats) ==
                 total_num_floats);
          for (auto v : vertices) {
            model.vertices.emplace_back(v);
          }
          break;
        }

        case cgltf_attribute_type_invalid:
        case cgltf_attribute_type_normal:
        case cgltf_attribute_type_tangent:
        case cgltf_attribute_type_texcoord:
        case cgltf_attribute_type_color:
        case cgltf_attribute_type_joints:
        case cgltf_attribute_type_weights:
        case cgltf_attribute_type_custom:
        case cgltf_attribute_type_max_enum:
          break;
        }
      }
      const auto& index_accessor = primitive.indices;
      std::vector<uint32_t> indices(index_accessor->count);
      ASSERT(cgltf_accessor_unpack_indices(primitive.indices, indices.data(), sizeof(uint32_t),
                                           index_accessor->count) == index_accessor->count);
      model.indices = indices;
    }
  }

  cgltf_free(data);

  return model;
}

} // namespace gfx
