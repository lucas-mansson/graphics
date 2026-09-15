#include "interpolation.hpp"
#include <cmath>
#include <glm/ext/matrix_float2x2.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/fwd.hpp>

glm::vec3 interpolation::evalLERP(glm::vec3 const &p0, glm::vec3 const &p1,
                                  float const x) {

  return (1.0f - x) * p0 + x * p1;
}

glm::vec3 interpolation::evalCatmullRom(glm::vec3 const &p0,
                                        glm::vec3 const &p1,
                                        glm::vec3 const &p2,
                                        glm::vec3 const &p3, float const t,
                                        float const x) {
  auto const v1 =
      glm::vec4(1.0f, x, std::pow<float>(x, 2), std::pow<float>(x, 3));

  auto const col1 = glm::vec4(0, -t, 2 * t, -t);
  auto const col2 = glm::vec4(1, 0, t - 3, 2 - t);
  auto const col3 = glm::vec4(0, t, 3 - 2 * t, t - 2);
  auto const col4 = glm::vec4(0, 0, -t, t);

  auto const m1 = glm::mat4(col1, col2, col3, col4);
  auto const m2 = glm::mat<4, 3, float>(p0, p1, p2, p3);

  return m2 * (v1 * m1);
}
