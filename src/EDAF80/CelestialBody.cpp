#include "CelestialBody.hpp"

#include <glm/detail/qualifier.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

#include "core/Log.h"
#include "core/helpers.hpp"

CelestialBody::CelestialBody(bonobo::mesh_data const &shape,
                             GLuint const *program, GLuint diffuse_texture_id) {
  _body.node.set_geometry(shape);
  _body.node.add_texture("diffuse_texture", diffuse_texture_id, GL_TEXTURE_2D);
  _body.node.set_program(program);
}

glm::mat4 CelestialBody::render(std::chrono::microseconds elapsed_time,
                                glm::mat4 const &view_projection,
                                glm::mat4 const &parent_transform,
                                bool show_basis) {
  // Convert the duration from microseconds to seconds.
  auto const elapsed_time_s =
      std::chrono::duration<float>(elapsed_time).count();
  // If a different ratio was needed, for example a duration in
  // milliseconds, the following would have been used:
  // auto const elapsed_time_ms = std::chrono::duration<float,
  // std::milli>(elapsed_time).count();

  glm::mat4 scaling_m = glm::scale(glm::mat4(1.0f), _body.scale);

  glm::mat4 translation_m = glm::translate(
      glm::mat4(1.0f), glm::vec3(_body.orbit.radius, 0.0f, 0.0f));

  // spinning around the y-axis.
  _body.spin.rotation_angle += elapsed_time_s * -glm::half_pi<float>() / 2.0f;
  glm::mat4 spin_y_axis_m = glm::rotate(
      glm::mat4(1.0f), _body.spin.rotation_angle, glm::vec3(0, 1, 0));

  // will tilt the spin plane by the specified axial tilt around the z-axis.
  glm::mat4 spin_tilt_m =
      glm::rotate(glm::mat4(1.0f), _body.spin.axial_tilt, glm::vec3(0, 0, 1));

  // orbit
  _body.orbit.rotation_angle += elapsed_time_s * -glm::half_pi<float>() / 2.0f;
  glm::mat4 orbit_spin_m = glm::rotate(
      glm::mat4(1.0f), _body.orbit.rotation_angle, glm::vec3(0, 1, 0));

  // will tilt the orbit plane by the specified axial tilt around the z-axis.
  glm::mat4 tilt_plane_m =
      glm::rotate(glm::mat4(1.0f), _body.orbit.inclination, glm::vec3(0, 0, 1));

  glm::mat4 planet_m = spin_tilt_m * spin_y_axis_m * scaling_m; // 100% correct
  // glm::mat4 orbit_m = orbit_spin_m * tilt_plane_m * translation_m;

  // glm::mat4 world = parent_transform;
  // parent transformations applied last?
  glm::mat4 world = parent_transform * planet_m;

  if (show_basis) {
    bonobo::renderBasis(1.0f, 2.0f, view_projection, world);
  }

  // Note: The second argument of `node::render()` is supposed to be the
  // parent transform of the node, not the whole world matrix, as the
  // node internally manages its local transforms. However in our case we
  // manage all the local transforms ourselves, so the internal transform
  // of the node is just the identity matrix and we can forward the whole
  // world matrix.
  _body.node.render(view_projection, world);

  glm::mat4 children_transform = translation_m;
  return children_transform;
}

void CelestialBody::add_child(CelestialBody *child) {
  _children.push_back(child);
}

std::vector<CelestialBody *> const &CelestialBody::get_children() const {
  return _children;
}

void CelestialBody::set_orbit(OrbitConfiguration const &configuration) {
  _body.orbit.radius = configuration.radius;
  _body.orbit.inclination = configuration.inclination;
  _body.orbit.speed = configuration.speed;
  _body.orbit.rotation_angle = 0.0f;
}

void CelestialBody::set_scale(glm::vec3 const &scale) { _body.scale = scale; }

void CelestialBody::set_spin(SpinConfiguration const &configuration) {
  _body.spin.axial_tilt = configuration.axial_tilt;
  _body.spin.speed = configuration.speed;
  _body.spin.rotation_angle = 0.0f;
}

void CelestialBody::set_ring(bonobo::mesh_data const &shape,
                             GLuint const *program, GLuint diffuse_texture_id,
                             glm::vec2 const &scale) {
  _ring.node.set_geometry(shape);
  _ring.node.add_texture("diffuse_texture", diffuse_texture_id, GL_TEXTURE_2D);
  _ring.node.set_program(program);

  _ring.scale = scale;

  _ring.is_set = true;
}
