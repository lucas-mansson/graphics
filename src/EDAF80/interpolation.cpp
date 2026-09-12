#include "interpolation.hpp"
#include <cmath>
#include <glm/ext/matrix_float2x2.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float2.hpp>

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
  auto const m1 = glm::mat4(0, -t, 2 * t, -t,       // col 1
                            1, 0, t - 3, 2 - t,     // col 2
                            0, t, 3 - 2 * t, t - 2, // col 3
                            0, 0, -t, t);           // col 4
  auto const m2 = glm::mat<4, 3, float>(p0, p1, p2, p3);

  return v1 * m1 * m2;
}
