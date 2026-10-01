#include "parametric_shapes.hpp"
#include "core/Log.h"

#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>

#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

static const int DEBUG = true;
template <typename Args> void p(Args arg) {
  if (DEBUG) {
    std::cout << arg << " " << "\n";
  }
}

template <typename T> void p(std::vector<T> v) {
  for (int i = 0; i < v.size(); i++) {
    p(v[i]);
  }
}

template <typename A, std::size_t N> void p(std::array<A, N> v) {
  for (int i = 0; i < v.size(); i++) {
    p(v[i]);
  }
}

bonobo::mesh_data create3DCube(float const width, float const height,
                               float const thickness,
                               unsigned int const split_count = 0u) {
  auto const x_edges_count = split_count + 1u;
  auto const y_edges_count = split_count + 1u;
  auto const z_edges_count = split_count + 1u;
  auto const x_vertices_count = x_edges_count + 1u;
  auto const y_vertices_count = y_edges_count + 1u;
  auto const z_vertices_count = z_edges_count + 1u;

  auto const nbr_vertices = y_vertices_count * x_vertices_count;
  auto vertices = std::vector<glm::vec3>(nbr_vertices);
  auto texcoords = std::vector<glm::vec3>(nbr_vertices);

  float const dx = width / x_edges_count;
  float const dz = height / y_edges_count;

  size_t index = 0;
  for (int i = 0; i < x_vertices_count; i++) {
    float x_vertex = i * dx;
    for (int j = 0; j < y_vertices_count; j++) {
      float z_vertex = j * dz;
      vertices[index] = glm::vec3(x_vertex, 0.0f, z_vertex);

      auto const tex_x =
          static_cast<float>(i) / (static_cast<float>(x_vertices_count));
      auto const tex_y =
          static_cast<float>(j) / (static_cast<float>(y_vertices_count));
      auto const tex_z = 0.0f;
      texcoords[index] = glm::vec3(tex_x, tex_y, tex_z);
      index++;
    }
  }

  auto index_sets = std::vector<glm::uvec3>(2u * y_edges_count * x_edges_count);
  index = 0u;
  for (unsigned int i = 0u; i < x_edges_count; ++i) {
    for (unsigned int j = 0u; j < y_edges_count; ++j) {
      int a = (y_vertices_count * (i + 0u) + (j + 0u));
      int b = (y_vertices_count * (i + 0u) + (j + 1u));
      int c = (y_vertices_count * (i + 1u) + (j + 0u));
      int d = (y_vertices_count * (i + 1u) + (j + 1u));

      index_sets[index++] = glm::uvec3(a, b, d);

      index_sets[index++] = glm::uvec3(a, d, c);
    }
  }

  bonobo::mesh_data data;

  // Vertex Attribute Object
  auto const vertexArrayObjectPtr = &data.vao;
  glGenVertexArrays(1, vertexArrayObjectPtr);
  assert(data.vao != 0u);
  glBindVertexArray(*vertexArrayObjectPtr);

  // Vertices
  auto const vertices_offset = 0u;
  auto const vertices_size =
      static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3));
  const auto vertices_index =
      static_cast<unsigned int>(bonobo::shader_bindings::vertices);
  const auto vertices_bufsize = sizeof(vertices);
  const auto vertices_buffer_location = vertices.data();
  const auto vertices_buffer_usage = GL_STATIC_DRAW;

  // Texture coords
  auto const texcoords_offset = vertices_offset + vertices_size;
  auto const texcoords_size =
      static_cast<GLsizeiptr>(texcoords.size() * sizeof(glm::vec3));

  // Buffer Object
  auto const bo_size = static_cast<GLsizeiptr>(vertices_size + texcoords_size);
  auto const buffer_object_ptr = &data.bo;

  glGenBuffers(1, buffer_object_ptr);
  assert(data.bo != 0u);
  glBindBuffer(GL_ARRAY_BUFFER, *buffer_object_ptr);
  glBufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STATIC_DRAW);

  // Vertices
  const auto vertices_nbr_components = vertices[0].length();
  const auto vertices_component_type = GL_FLOAT;
  const auto vertices_normalize = GL_FALSE;
  const auto vertices_stride = 0;
  const auto vertices_offset_first_component =
      reinterpret_cast<GLvoid const *>(0x0);

  glBufferSubData(GL_ARRAY_BUFFER, vertices_offset, vertices_size,
                  static_cast<GLvoid const *>(vertices.data()));
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::vertices));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::vertices), 3, GL_FLOAT,
      GL_FALSE, 0, reinterpret_cast<GLvoid const *>(0x0));

  // Texture coordinates
  glBufferSubData(GL_ARRAY_BUFFER, texcoords_offset, texcoords_size,
                  static_cast<GLvoid const *>(texcoords.data()));
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::texcoords));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::texcoords), 3,
      GL_FLOAT, GL_FALSE, 0,
      reinterpret_cast<GLvoid const *>(texcoords_offset));

  // Indices
  auto const indices_buf_obj_ptr = &data.ibo;
  auto const indices_bufsize = index_sets.size() * sizeof(glm::uvec3);

  glGenBuffers(1, indices_buf_obj_ptr);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *indices_buf_obj_ptr);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_bufsize, index_sets.data(),
               GL_STATIC_DRAW);

  const auto nbr_indicies = index_sets.size() * index_sets[0].length();

  data.indices_nb = nbr_indicies;

  // All the data has been recorded, we can unbind them.
  glBindVertexArray(0u);
  // glBindBuffer(GL_ARRAY_BUFFER, 0u);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);
  return data;
}

bonobo::mesh_data
parametric_shapes::createQuadXY(float const width, float const height,
                                unsigned int const horizontal_split_count,
                                unsigned int const vertical_split_count) {

  auto const horizontal_edges_count = horizontal_split_count + 1u;
  auto const vertical_edges_count = vertical_split_count + 1u;
  auto const horizontal_vertices_count = horizontal_edges_count + 1u;
  auto const vertical_vertices_count = vertical_edges_count + 1u;

  auto const nbr_vertices = vertical_vertices_count * horizontal_vertices_count;
  auto vertices = std::vector<glm::vec3>(nbr_vertices);
  auto texcoords = std::vector<glm::vec3>(nbr_vertices);

  float const dx = width / horizontal_edges_count;
  float const dy = height / vertical_edges_count;

  size_t index = 0;
  for (int i = 0; i < horizontal_vertices_count; i++) {
    float x_vertex = i * dx;
    for (int j = 0; j < vertical_vertices_count; j++) {
      float y_vertex = j * dy;
      vertices[index] = glm::vec3(x_vertex, y_vertex, 0.0f);

      auto const tex_x = static_cast<float>(i) /
                         (static_cast<float>(horizontal_vertices_count));
      auto const tex_y =
          static_cast<float>(j) / (static_cast<float>(vertical_vertices_count));
      auto const tex_z = 0.0f;
      texcoords[index] = glm::vec3(tex_x, tex_y, tex_z);
      index++;
    }
  }

  auto index_sets = std::vector<glm::uvec3>(2u * vertical_edges_count *
                                            horizontal_edges_count);
  index = 0u;
  for (unsigned int i = 0u; i < horizontal_edges_count; ++i) {
    for (unsigned int j = 0u; j < vertical_edges_count; ++j) {
      int a = (vertical_vertices_count * (i + 0u) + (j + 0u));
      int b = (vertical_vertices_count * (i + 0u) + (j + 1u));
      int c = (vertical_vertices_count * (i + 1u) + (j + 0u));
      int d = (vertical_vertices_count * (i + 1u) + (j + 1u));

      index_sets[index++] = glm::uvec3(a, b, d);

      index_sets[index++] = glm::uvec3(a, d, c);
    }
  }

  bonobo::mesh_data data;

  // Vertex Attribute Object
  auto const vertexArrayObjectPtr = &data.vao;
  glGenVertexArrays(1, vertexArrayObjectPtr);
  assert(data.vao != 0u);
  glBindVertexArray(*vertexArrayObjectPtr);

  // Vertices
  auto const vertices_offset = 0u;
  auto const vertices_size =
      static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3));
  const auto vertices_index =
      static_cast<unsigned int>(bonobo::shader_bindings::vertices);
  const auto vertices_bufsize = sizeof(vertices);
  const auto vertices_buffer_location = vertices.data();
  const auto vertices_buffer_usage = GL_STATIC_DRAW;

  // Texture coords
  auto const texcoords_offset = vertices_offset + vertices_size;
  auto const texcoords_size =
      static_cast<GLsizeiptr>(texcoords.size() * sizeof(glm::vec3));

  // Buffer Object
  auto const bo_size = static_cast<GLsizeiptr>(vertices_size + texcoords_size);
  auto const buffer_object_ptr = &data.bo;

  glGenBuffers(1, buffer_object_ptr);
  assert(data.bo != 0u);
  glBindBuffer(GL_ARRAY_BUFFER, *buffer_object_ptr);
  glBufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STATIC_DRAW);

  // Vertices
  const auto vertices_nbr_components = vertices[0].length();
  const auto vertices_component_type = GL_FLOAT;
  const auto vertices_normalize = GL_FALSE;
  const auto vertices_stride = 0;
  const auto vertices_offset_first_component =
      reinterpret_cast<GLvoid const *>(0x0);

  glBufferSubData(GL_ARRAY_BUFFER, vertices_offset, vertices_size,
                  static_cast<GLvoid const *>(vertices.data()));
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::vertices));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::vertices), 3, GL_FLOAT,
      GL_FALSE, 0, reinterpret_cast<GLvoid const *>(0x0));

  // Texture coordinates
  glBufferSubData(GL_ARRAY_BUFFER, texcoords_offset, texcoords_size,
                  static_cast<GLvoid const *>(texcoords.data()));
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::texcoords));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::texcoords), 3,
      GL_FLOAT, GL_FALSE, 0,
      reinterpret_cast<GLvoid const *>(texcoords_offset));

  // Indices
  auto const indices_buf_obj_ptr = &data.ibo;
  auto const indices_bufsize = index_sets.size() * sizeof(glm::uvec3);

  glGenBuffers(1, indices_buf_obj_ptr);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *indices_buf_obj_ptr);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_bufsize, index_sets.data(),
               GL_STATIC_DRAW);

  const auto nbr_indicies = index_sets.size() * index_sets[0].length();

  data.indices_nb = nbr_indicies;

  // All the data has been recorded, we can unbind them.
  glBindVertexArray(0u);
  // glBindBuffer(GL_ARRAY_BUFFER, 0u);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);
  return data;
}

bonobo::mesh_data
parametric_shapes::createQuad(float const width, float const height,
                              unsigned int const horizontal_split_count,
                              unsigned int const vertical_split_count) {

  auto const horizontal_edges_count = horizontal_split_count + 1u;
  auto const vertical_edges_count = vertical_split_count + 1u;
  auto const horizontal_vertices_count = horizontal_edges_count + 1u;
  auto const vertical_vertices_count = vertical_edges_count + 1u;

  auto const nbr_vertices = vertical_vertices_count * horizontal_vertices_count;
  auto vertices = std::vector<glm::vec3>(nbr_vertices);
  auto texcoords = std::vector<glm::vec3>(nbr_vertices);

  float const dx = width / horizontal_edges_count;
  float const dz = height / vertical_edges_count;

  size_t index = 0;
  for (int i = 0; i < horizontal_vertices_count; i++) {
    float x_vertex = i * dx;
    for (int j = 0; j < vertical_vertices_count; j++) {
      float z_vertex = j * dz;
      vertices[index] = glm::vec3(x_vertex, 0.0f, z_vertex);

      auto const tex_x = static_cast<float>(i) /
                         (static_cast<float>(horizontal_vertices_count));
      auto const tex_y =
          static_cast<float>(j) / (static_cast<float>(vertical_vertices_count));
      auto const tex_z = 0.0f;
      texcoords[index] = glm::vec3(tex_x, tex_y, tex_z);
      index++;
    }
  }

  auto index_sets = std::vector<glm::uvec3>(2u * vertical_edges_count *
                                            horizontal_edges_count);
  index = 0u;
  for (unsigned int i = 0u; i < horizontal_edges_count; ++i) {
    for (unsigned int j = 0u; j < vertical_edges_count; ++j) {
      int a = (vertical_vertices_count * (i + 0u) + (j + 0u));
      int b = (vertical_vertices_count * (i + 0u) + (j + 1u));
      int c = (vertical_vertices_count * (i + 1u) + (j + 0u));
      int d = (vertical_vertices_count * (i + 1u) + (j + 1u));

      index_sets[index++] = glm::uvec3(a, b, d);

      index_sets[index++] = glm::uvec3(a, d, c);
    }
  }

  bonobo::mesh_data data;

  // Vertex Attribute Object
  auto const vertexArrayObjectPtr = &data.vao;
  glGenVertexArrays(1, vertexArrayObjectPtr);
  assert(data.vao != 0u);
  glBindVertexArray(*vertexArrayObjectPtr);

  // Vertices
  auto const vertices_offset = 0u;
  auto const vertices_size =
      static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3));
  const auto vertices_index =
      static_cast<unsigned int>(bonobo::shader_bindings::vertices);
  const auto vertices_bufsize = sizeof(vertices);
  const auto vertices_buffer_location = vertices.data();
  const auto vertices_buffer_usage = GL_STATIC_DRAW;

  // Texture coords
  auto const texcoords_offset = vertices_offset + vertices_size;
  auto const texcoords_size =
      static_cast<GLsizeiptr>(texcoords.size() * sizeof(glm::vec3));

  // Buffer Object
  auto const bo_size = static_cast<GLsizeiptr>(vertices_size + texcoords_size);
  auto const buffer_object_ptr = &data.bo;

  glGenBuffers(1, buffer_object_ptr);
  assert(data.bo != 0u);
  glBindBuffer(GL_ARRAY_BUFFER, *buffer_object_ptr);
  glBufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STATIC_DRAW);

  // Vertices
  const auto vertices_nbr_components = vertices[0].length();
  const auto vertices_component_type = GL_FLOAT;
  const auto vertices_normalize = GL_FALSE;
  const auto vertices_stride = 0;
  const auto vertices_offset_first_component =
      reinterpret_cast<GLvoid const *>(0x0);

  glBufferSubData(GL_ARRAY_BUFFER, vertices_offset, vertices_size,
                  static_cast<GLvoid const *>(vertices.data()));
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::vertices));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::vertices), 3, GL_FLOAT,
      GL_FALSE, 0, reinterpret_cast<GLvoid const *>(0x0));

  // Texture coordinates
  glBufferSubData(GL_ARRAY_BUFFER, texcoords_offset, texcoords_size,
                  static_cast<GLvoid const *>(texcoords.data()));
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::texcoords));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::texcoords), 3,
      GL_FLOAT, GL_FALSE, 0,
      reinterpret_cast<GLvoid const *>(texcoords_offset));

  // Indices
  auto const indices_buf_obj_ptr = &data.ibo;
  auto const indices_bufsize = index_sets.size() * sizeof(glm::uvec3);

  glGenBuffers(1, indices_buf_obj_ptr);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *indices_buf_obj_ptr);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_bufsize, index_sets.data(),
               GL_STATIC_DRAW);

  const auto nbr_indicies = index_sets.size() * index_sets[0].length();

  data.indices_nb = nbr_indicies;

  // All the data has been recorded, we can unbind them.
  glBindVertexArray(0u);
  // glBindBuffer(GL_ARRAY_BUFFER, 0u);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);
  return data;
}

bonobo::mesh_data
parametric_shapes::createSphere(float const radius,
                                unsigned int const horizontal_split_count,
                                unsigned int const vertical_split_count) {
  // 1. generate the various vertex attributes (position, normal, tangent,
  // binormal, and texture coordinates),
  auto const horizontal_edges_count = horizontal_split_count + 1u;
  auto const vertical_edges_count = vertical_split_count + 1u;

  auto const horizontal_vertices_count = horizontal_edges_count + 1u;
  auto const vertical_vertices_count = vertical_edges_count + 1u;

  auto const nbr_vertices =
      vertical_vertices_count * (horizontal_vertices_count + 1);

  auto vertices = std::vector<glm::vec3>(nbr_vertices);
  auto normals = std::vector<glm::vec3>(nbr_vertices);
  auto texcoords = std::vector<glm::vec3>(nbr_vertices);
  auto tangents = std::vector<glm::vec3>(nbr_vertices);
  auto binormals = std::vector<glm::vec3>(nbr_vertices);

  float d_theta = glm::two_pi<float>() / horizontal_edges_count;
  float d_phi = glm::pi<float>() / vertical_edges_count;

  size_t index = 0u;
  float theta = 0.0f;
  for (unsigned int i = 0u; i < horizontal_vertices_count + 1; ++i) {
    assert(0 <= theta && theta <= glm::two_pi<float>());

    float const cos_theta = std::cos(theta);
    float const sin_theta = std::sin(theta);

    float phi = 0.0f;
    for (unsigned int j = 0u; j < vertical_vertices_count; ++j) {
      assert(0 <= phi && phi <= glm::pi<float>());

      float const cos_phi = std::cos(phi);
      float const sin_phi = std::sin(phi);

      // vertex
      auto const x_vertex = radius * sin_theta * sin_phi;
      auto const y_vertex = -radius * cos_phi;
      auto const z_vertex = radius * cos_theta * sin_phi;
      vertices[index] = glm::vec3(x_vertex, y_vertex, z_vertex);

      // tangent simplified
      auto const x_tangent = cos_theta;
      auto const y_tangent = 0;
      auto const z_tangent = -sin_theta;
      auto const tangent = glm::vec3(x_tangent, y_tangent, z_tangent);
      tangents[index] = tangent;

      // binormal simplified
      auto const x_binormal = sin_theta * cos_phi;
      auto const y_binormal = sin_phi;
      auto const z_binormal = cos_theta * cos_phi;
      auto const binormal = glm::vec3(x_binormal, y_binormal, z_binormal);
      binormals[index] = binormal;

      // normal
      auto const normal = glm::cross(tangent, binormal);
      normals[index] = normal;

      // texture coords
      auto const tex_x = static_cast<float>(i) /
                         (static_cast<float>(horizontal_vertices_count));
      auto const tex_y =
          static_cast<float>(j) / (static_cast<float>(vertical_vertices_count));
      auto const tex_z =
          0.0f; // static_cast<float>(i) /
                //(static_cast<float>(horizontal_vertices_count));
      texcoords[index] = glm::vec3(tex_x, tex_y, tex_z);

      phi += d_phi;
      ++index;
    }
    theta += d_theta;
  }

  // 2. generate the indices to group the vertices into triangles,
  auto index_sets = std::vector<glm::uvec3>(2u * vertical_edges_count *
                                            horizontal_edges_count);

  index = 0u;
  for (unsigned int i = 0u; i < horizontal_edges_count; ++i) {
    for (unsigned int j = 0u; j < vertical_edges_count; ++j) {
      index_sets[index++] =
          glm::uvec3((horizontal_vertices_count * (i + 0u) + (j + 0u)),
                     (horizontal_vertices_count * (i + 0u) + (j + 1u)),
                     (horizontal_vertices_count * (i + 1u) + (j + 1u)));

      index_sets[index++] =
          glm::uvec3((horizontal_vertices_count * (i + 0u) + (j + 0u)),
                     (horizontal_vertices_count * (i + 1u) + (j + 1u)),
                     (horizontal_vertices_count * (i + 1u) + (j + 0u)));
    }
  }

  // 3. upload all that data to the GPU,
  // 4. configure the vertex array object.
  bonobo::mesh_data data;

  // Vertex Attribute Object
  auto const vertexArrayObjectPtr = &data.vao;
  glGenVertexArrays(1, vertexArrayObjectPtr);
  assert(data.vao != 0u);
  glBindVertexArray(*vertexArrayObjectPtr);

  // Vertices
  auto const vertices_offset = 0u;
  auto const vertices_size =
      static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3));
  const auto vertices_index =
      static_cast<unsigned int>(bonobo::shader_bindings::vertices);
  const auto vertices_bufsize = sizeof(vertices);
  const auto vertices_buffer_location = vertices.data();
  const auto vertices_buffer_usage = GL_STATIC_DRAW;

  // Normals
  auto const normals_offset = vertices_size;
  auto const normals_size =
      static_cast<GLsizeiptr>(normals.size() * sizeof(glm::vec3));

  // Texture coords
  auto const texcoords_offset = normals_offset + normals_size;
  auto const texcoords_size =
      static_cast<GLsizeiptr>(texcoords.size() * sizeof(glm::vec3));

  // tangents
  auto const tangents_offset = texcoords_offset + texcoords_size;
  auto const tangents_size =
      static_cast<GLsizeiptr>(tangents.size() * sizeof(glm::vec3));

  // binormals
  auto const binormals_offset = tangents_offset + tangents_size;
  auto const binormals_size =
      static_cast<GLsizeiptr>(binormals.size() * sizeof(glm::vec3));

  // Buffer Object
  auto const bo_size =
      static_cast<GLsizeiptr>(vertices_size + normals_size + texcoords_size +
                              tangents_size + binormals_size);
  auto const buffer_object_ptr = &data.bo;

  glGenBuffers(1, buffer_object_ptr);
  assert(data.bo != 0u);
  glBindBuffer(GL_ARRAY_BUFFER, *buffer_object_ptr);
  glBufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STATIC_DRAW);

  // Vertices
  const auto vertices_nbr_components = vertices[0].length();
  const auto vertices_component_type = GL_FLOAT;
  const auto vertices_normalize = GL_FALSE;
  const auto vertices_stride = 0;
  const auto vertices_offset_first_component =
      reinterpret_cast<GLvoid const *>(0x0);

  glBufferSubData(GL_ARRAY_BUFFER, vertices_offset, vertices_size,
                  static_cast<GLvoid const *>(vertices.data()));
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::vertices));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::vertices), 3, GL_FLOAT,
      GL_FALSE, 0, reinterpret_cast<GLvoid const *>(0x0));

  // Normals
  glBufferSubData(GL_ARRAY_BUFFER, normals_offset, normals_size,
                  static_cast<GLvoid const *>(normals.data()));
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::normals));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::normals), 3, GL_FLOAT,
      GL_FALSE, 0, reinterpret_cast<GLvoid const *>(normals_offset));

  // Texture coordinates
  glBufferSubData(GL_ARRAY_BUFFER, texcoords_offset, texcoords_size,
                  static_cast<GLvoid const *>(texcoords.data()));
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::texcoords));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::texcoords), 3,
      GL_FLOAT, GL_FALSE, 0,
      reinterpret_cast<GLvoid const *>(texcoords_offset));

  // Tangents
  glBufferSubData(GL_ARRAY_BUFFER, tangents_offset, tangents_size,
                  static_cast<GLvoid const *>(tangents.data()));
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::tangents));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::tangents), 3, GL_FLOAT,
      GL_FALSE, 0, reinterpret_cast<GLvoid const *>(tangents_offset));

  // Binormals
  glBufferSubData(GL_ARRAY_BUFFER, binormals_offset, binormals_size,
                  static_cast<GLvoid const *>(binormals.data()));
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::binormals));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::binormals), 3,
      GL_FLOAT, GL_FALSE, 0,
      reinterpret_cast<GLvoid const *>(binormals_offset));

  // Indices
  auto const indices_buf_obj_ptr = &data.ibo;
  auto const indices_bufsize = index_sets.size() * sizeof(glm::uvec3);

  glGenBuffers(1, indices_buf_obj_ptr);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *indices_buf_obj_ptr);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_bufsize, index_sets.data(),
               GL_STATIC_DRAW);

  const auto nbr_indicies = index_sets.size() * index_sets[0].length();

  data.indices_nb = nbr_indicies;

  // All the data has been recorded, we can unbind them.
  glBindVertexArray(0u);
  // glBindBuffer(GL_ARRAY_BUFFER, 0u);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);
  return data;
}

bonobo::mesh_data
parametric_shapes::createTorus(float const major_radius,
                               float const minor_radius,
                               unsigned int const major_split_count,
                               unsigned int const minor_split_count) {
  //! \todo (Optional) Implement this function
  return bonobo::mesh_data();
}

bonobo::mesh_data
parametric_shapes::createCircleRing(float const radius,
                                    float const spread_length,
                                    unsigned int const circle_split_count,
                                    unsigned int const spread_split_count) {
  auto const circle_slice_edges_count = circle_split_count + 1u;
  auto const spread_slice_edges_count = spread_split_count + 1u;
  auto const circle_slice_vertices_count = circle_slice_edges_count + 1u;
  auto const spread_slice_vertices_count = spread_slice_edges_count + 1u;
  auto const vertices_nb =
      circle_slice_vertices_count * spread_slice_vertices_count;

  auto vertices = std::vector<glm::vec3>(vertices_nb);
  auto normals = std::vector<glm::vec3>(vertices_nb);
  auto texcoords = std::vector<glm::vec3>(vertices_nb);
  auto tangents = std::vector<glm::vec3>(vertices_nb);
  auto binormals = std::vector<glm::vec3>(vertices_nb);

  float const spread_start = radius - 0.5f * spread_length;
  float const d_theta =
      glm::two_pi<float>() / (static_cast<float>(circle_slice_edges_count));
  float const d_spread =
      spread_length / (static_cast<float>(spread_slice_edges_count));

  // generate vertices iteratively
  size_t index = 0u;
  float theta = 0.0f;
  for (unsigned int i = 0u; i < circle_slice_vertices_count; ++i) {
    float const cos_theta = std::cos(theta);
    float const sin_theta = std::sin(theta);

    float distance_to_centre = spread_start;
    for (unsigned int j = 0u; j < spread_slice_vertices_count; ++j) {
      // vertex
      vertices[index] = glm::vec3(distance_to_centre * cos_theta,
                                  distance_to_centre * sin_theta, 0.0f);

      // texture coordinates
      texcoords[index] =
          glm::vec3(static_cast<float>(j) /
                        (static_cast<float>(spread_slice_vertices_count)),
                    static_cast<float>(i) /
                        (static_cast<float>(circle_slice_vertices_count)),
                    0.0f);

      // tangent
      auto const t = glm::vec3(cos_theta, sin_theta, 0.0f);
      tangents[index] = t;

      // binormal
      auto const b = glm::vec3(-sin_theta, cos_theta, 0.0f);
      binormals[index] = b;

      // normal
      auto const n = glm::cross(t, b);
      normals[index] = n;

      distance_to_centre += d_spread;
      ++index;
    }

    theta += d_theta;
  }

  // create index array
  auto index_sets = std::vector<glm::uvec3>(2u * circle_slice_edges_count *
                                            spread_slice_edges_count);

  // generate indices iteratively
  index = 0u;
  for (unsigned int i = 0u; i < circle_slice_edges_count; ++i) {
    for (unsigned int j = 0u; j < spread_slice_edges_count; ++j) {
      index_sets[index] =
          glm::uvec3(spread_slice_vertices_count * (i + 0u) + (j + 0u),
                     spread_slice_vertices_count * (i + 0u) + (j + 1u),
                     spread_slice_vertices_count * (i + 1u) + (j + 1u));
      ++index;

      index_sets[index] =
          glm::uvec3(spread_slice_vertices_count * (i + 0u) + (j + 0u),
                     spread_slice_vertices_count * (i + 1u) + (j + 1u),
                     spread_slice_vertices_count * (i + 1u) + (j + 0u));
      ++index;
    }
  }

  bonobo::mesh_data data;
  glGenVertexArrays(1, &data.vao);
  assert(data.vao != 0u);
  glBindVertexArray(data.vao);

  auto const vertices_offset = 0u;
  auto const vertices_size =
      static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3));

  auto const normals_offset = vertices_size;
  auto const normals_size =
      static_cast<GLsizeiptr>(normals.size() * sizeof(glm::vec3));

  auto const texcoords_offset = normals_offset + normals_size;
  auto const texcoords_size =
      static_cast<GLsizeiptr>(texcoords.size() * sizeof(glm::vec3));

  auto const tangents_offset = texcoords_offset + texcoords_size;
  auto const tangents_size =
      static_cast<GLsizeiptr>(tangents.size() * sizeof(glm::vec3));

  auto const binormals_offset = tangents_offset + tangents_size;
  auto const binormals_size =
      static_cast<GLsizeiptr>(binormals.size() * sizeof(glm::vec3));

  auto const bo_size =
      static_cast<GLsizeiptr>(vertices_size + normals_size + texcoords_size +
                              tangents_size + binormals_size);
  glGenBuffers(1, &data.bo);
  assert(data.bo != 0u);
  glBindBuffer(GL_ARRAY_BUFFER, data.bo);
  glBufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STATIC_DRAW);

  // Vertices
  glBufferSubData(GL_ARRAY_BUFFER, vertices_offset, vertices_size,
                  static_cast<GLvoid const *>(vertices.data()));
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::vertices));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::vertices), 3, GL_FLOAT,
      GL_FALSE, 0, reinterpret_cast<GLvoid const *>(0x0));

  // normals
  glBufferSubData(GL_ARRAY_BUFFER, normals_offset, normals_size,
                  static_cast<GLvoid const *>(normals.data()));
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::normals));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::normals), 3, GL_FLOAT,
      GL_FALSE, 0, reinterpret_cast<GLvoid const *>(normals_offset));

  // texture coordinates
  glBufferSubData(GL_ARRAY_BUFFER, texcoords_offset, texcoords_size,
                  static_cast<GLvoid const *>(texcoords.data()));
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::texcoords));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::texcoords), 3,
      GL_FLOAT, GL_FALSE, 0,
      reinterpret_cast<GLvoid const *>(texcoords_offset));

  // tangents
  glBufferSubData(GL_ARRAY_BUFFER, tangents_offset, tangents_size,
                  static_cast<GLvoid const *>(tangents.data()));
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::tangents));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::tangents), 3, GL_FLOAT,
      GL_FALSE, 0, reinterpret_cast<GLvoid const *>(tangents_offset));

  // binormals
  glBufferSubData(GL_ARRAY_BUFFER, binormals_offset, binormals_size,
                  static_cast<GLvoid const *>(binormals.data()));
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::binormals));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::binormals), 3,
      GL_FLOAT, GL_FALSE, 0,
      reinterpret_cast<GLvoid const *>(binormals_offset));

  glBindBuffer(GL_ARRAY_BUFFER, 0u);

  data.indices_nb = static_cast<GLsizei>(index_sets.size() * 3u);
  glGenBuffers(1, &data.ibo);
  assert(data.ibo != 0u);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(index_sets.size() * sizeof(glm::uvec3)),
               reinterpret_cast<GLvoid const *>(index_sets.data()),
               GL_STATIC_DRAW);

  glBindVertexArray(0u);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

  return data;
}
