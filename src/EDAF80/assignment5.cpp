#include "assignment5.hpp"

#include "EDAF80/parametric_shapes.hpp"
#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/InputHandler.h"
#include "core/ShaderProgramManager.hpp"
#include "core/helpers.hpp"
#include "core/node.hpp"

#include <GLFW/glfw3.h>
#include <array>
#include <cstdlib>
#include <glm/common.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <imgui.h>
#include <tinyfiledialogs.h>

#include <clocale>
#include <stdexcept>

struct Sphere {
  Node node;
  float radius;
};

struct Rectangle {
  Node node;
  float height;
  float width;
  glm::vec3 normal;
};

struct CollisionResult {
  bool collision;
  glm::vec3 collisionNormal;
};

// TODO: Fix this with transformations instead of two separate functions
CollisionResult checkXYRectangleSphereCollision(Sphere sphere, Rectangle rec) {
  auto const sphereCenter = sphere.node.get_transform().GetTranslation();
  auto const recPt = rec.node.get_transform().GetTranslation();

  glm::vec3 closestPoint;
  closestPoint.x = glm::clamp(sphereCenter.x, recPt.x, recPt.x + rec.width);
  closestPoint.y = glm::clamp(sphereCenter.y, recPt.y, recPt.y + rec.height);
  closestPoint.z = recPt.z;

  auto const delta = sphereCenter - closestPoint;
  float distSq =
      glm::dot(sphereCenter - closestPoint, sphereCenter - closestPoint);

  CollisionResult result;
  result.collision = distSq <= sphere.radius * sphere.radius;
  result.collisionNormal = glm::normalize(delta);

  return result;
}

CollisionResult checkYZRectangleSphereCollision(Sphere sphere, Rectangle rec) {
  auto const sphereCenter = sphere.node.get_transform().GetTranslation();
  auto const recPt = rec.node.get_transform().GetTranslation();

  glm::vec3 closestPoint;
  closestPoint.x = recPt.x;
  closestPoint.y = glm::clamp(sphereCenter.y, recPt.y, recPt.y + rec.height);
  closestPoint.z = glm::clamp(sphereCenter.z, recPt.z - rec.width, recPt.z);

  auto const delta = sphereCenter - closestPoint;
  float distSq =
      glm::dot(sphereCenter - closestPoint, sphereCenter - closestPoint);

  CollisionResult result;
  result.collision = distSq <= sphere.radius * sphere.radius;
  result.collisionNormal = glm::normalize(delta);

  return result;
}

edaf80::Assignment5::Assignment5(WindowManager &windowManager)
    : mCamera(0.5f * glm::half_pi<float>(),
              static_cast<float>(config::resolution_x) /
                  static_cast<float>(config::resolution_y),
              0.01f, 1000.0f),
      inputHandler(), mWindowManager(windowManager), window(nullptr) {
  WindowManager::WindowDatum window_datum{inputHandler,
                                          mCamera,
                                          config::resolution_x,
                                          config::resolution_y,
                                          0,
                                          0,
                                          0,
                                          0};

  window = mWindowManager.CreateGLFWWindow("EDAF80: Assignment 5", window_datum,
                                           config::msaa_rate);
  if (window == nullptr) {
    throw std::runtime_error("Failed to get a window: aborting!");
  }

  bonobo::init();
}

edaf80::Assignment5::~Assignment5() { bonobo::deinit(); }

void edaf80::Assignment5::run() {
  // Set up the camera
  mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 30.0f, 25.0f));
  mCamera.mWorld.LookAt(glm::vec3(0, 0, 0));
  mCamera.mMouseSensitivity = glm::vec2(0.003f);
  mCamera.mMovementSpeed = glm::vec3(3.0f); // 3 m/s => 10.8 km/h

  // Create the shader programs
  ShaderProgramManager program_manager;
  GLuint fallback_shader = 0u;
  program_manager.CreateAndRegisterProgram(
      "Fallback",
      {{ShaderType::vertex, "common/fallback.vert"},
       {ShaderType::fragment, "common/fallback.frag"}},
      fallback_shader);
  if (fallback_shader == 0u) {
    LogError("Failed to load fallback shader");
    return;
  }

  // TODO: Insert the creation of other shader programs.
  GLuint texcoord_shader = 0u;
  program_manager.CreateAndRegisterProgram(
      "Texture coords",
      {{ShaderType::vertex, "EDAF80/texcoord.vert"},
       {ShaderType::fragment, "EDAF80/texcoord.frag"}},
      texcoord_shader);
  if (texcoord_shader == 0u)
    LogError("Failed to load texcoord shader");

  // TODO: Load your geometry
  std::vector<Node *> nodes;

  const float paddleSideSize = 5.0f;
  bonobo::mesh_data paddleShape =
      parametric_shapes::createQuadXY(paddleSideSize, paddleSideSize);
  const float paddleX = -paddleSideSize / 2.0f;
  const float paddleY = -paddleSideSize / 2.0f;
  const float paddleDistanceFromOrigo = 12.0f;

  std::array<glm::vec3, 2> paddlePositions = {
      glm::vec3(paddleX, paddleY, paddleDistanceFromOrigo),
      glm::vec3(paddleX, paddleY, -paddleDistanceFromOrigo),
  };
  std::array<Rectangle, 2> paddles;
  for (int i = 0; i < 2; i++) {
    Rectangle &paddle = paddles[i];
    paddle.node.set_geometry(paddleShape);
    paddle.node.set_program(&texcoord_shader);
    paddle.node.get_transform().SetTranslate(paddlePositions[i]);

    paddle.normal = glm::vec3(0.0f, 0.0f, 1.0f);
    paddle.height = paddleSideSize;
    paddle.width = paddleSideSize;

    nodes.push_back(&paddles[i].node);
  }

  // 4 sides
  const int nbrBorders = 4;
  const float borderWidth = 30.0f;
  const float borderHeight = 10.0f;
  const float borderY = -borderHeight / 2.0f;
  bonobo::mesh_data borderShape =
      parametric_shapes::createQuadXY(borderWidth, borderHeight);

  std::array<glm::vec3, 2> sideBorderPositions = {
      glm::vec3(-borderWidth / 2.0f, borderY, borderWidth / 2.0f),
      glm::vec3(borderWidth / 2.0f, borderY, borderWidth / 2.0f),
  };
  std::array<Rectangle, 2> sideBorders;
  for (int i = 0; i < 2; i++) {
    Rectangle &border = sideBorders[i];
    border.node.set_geometry(borderShape);
    border.node.set_program(&texcoord_shader);
    border.node.get_transform().SetTranslate(sideBorderPositions[i]);
    border.node.get_transform().RotateY(glm::half_pi<float>());

    border.normal = glm::vec3(0.0f, 0.0f, 1.0f);
    border.height = borderHeight;
    border.width = borderWidth;

    nodes.push_back(&sideBorders[i].node);
  }

  Rectangle backBorder1;
  Node backBorder1Node;
  auto const backborder1Position =
      glm::vec3(-borderWidth / 2.0f, borderY, borderWidth / 2.0f);
  backBorder1Node.set_geometry(borderShape);
  backBorder1Node.set_program(&texcoord_shader);
  backBorder1Node.get_transform().SetTranslate(backborder1Position);
  backBorder1.node = backBorder1Node;
  backBorder1.normal = glm::vec3(0, 0, 1);
  backBorder1.height = borderHeight;
  backBorder1.width = borderWidth;
  nodes.push_back(&backBorder1.node);

  Rectangle backBorder2;
  Node backBorder2Node;
  auto const backborder2Position =
      glm::vec3(-borderWidth / 2.0f, borderY, -borderWidth / 2.0f);
  backBorder2Node.set_geometry(borderShape);
  backBorder2Node.set_program(&texcoord_shader);
  backBorder2Node.get_transform().SetTranslate(backborder2Position);
  backBorder2.node = backBorder2Node;
  backBorder2.normal = glm::vec3(0, 0, 1);
  backBorder2.height = borderHeight;
  backBorder2.width = borderWidth;
  nodes.push_back(&backBorder2.node);

  Sphere ball;
  const float ballRadius = 1.0f;
  bonobo::mesh_data ballShape =
      parametric_shapes::createSphere(ballRadius, 1000u, 1000u);
  Node ballNode;
  ballNode.set_geometry(ballShape);
  ballNode.set_program(&texcoord_shader);
  ball.node = ballNode;
  ball.radius = ballRadius;
  nodes.push_back(&ball.node);

  auto ballDirection = glm::vec3(1, 0, 0);

  glClearDepthf(1.0f);
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glEnable(GL_DEPTH_TEST);

  auto lastTime = std::chrono::high_resolution_clock::now();

  float const strafe_speed = 0.1f;

  while (!glfwWindowShouldClose(window)) {
    auto const nowTime = std::chrono::high_resolution_clock::now();
    auto const deltaTimeUs =
        std::chrono::duration_cast<std::chrono::microseconds>(nowTime -
                                                              lastTime);
    lastTime = nowTime;

    ball.node.get_transform().Translate(ballDirection / 10.0f);

    CollisionResult collisionBack1 =
        checkXYRectangleSphereCollision(ball, backBorder1);
    if (collisionBack1.collision) {
      return;
    }
    CollisionResult collisionBack2 =
        checkXYRectangleSphereCollision(ball, backBorder2);
    if (collisionBack2.collision) {
      return;
    }

    for (int i = 0; i < paddles.size(); i++) {
      CollisionResult collisionPaddle =
          checkXYRectangleSphereCollision(ball, paddles.at(i));
      if (collisionPaddle.collision) {
        ballDirection = glm::normalize(
            glm::reflect(ballDirection, collisionPaddle.collisionNormal));
      }
    }

    for (int i = 0; i < sideBorders.size(); i++) {
      CollisionResult collisionSide =
          checkYZRectangleSphereCollision(ball, sideBorders.at(i));
      if (collisionSide.collision) {
        ballDirection = glm::normalize(
            glm::reflect(ballDirection, collisionSide.collisionNormal));
      }
    }

    auto &io = ImGui::GetIO();
    inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

    glfwPollEvents();
    inputHandler.Advance();
    mCamera.Update(deltaTimeUs, inputHandler);

    int framebuffer_width, framebuffer_height;
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
    glViewport(0, 0, framebuffer_width, framebuffer_height);

    if ((inputHandler.GetKeycodeState(GLFW_KEY_RIGHT) & PRESSED)) {
      paddles.at(0).node.get_transform().Translate(
          glm::vec3(strafe_speed, 0, 0));
    }
    if ((inputHandler.GetKeycodeState(GLFW_KEY_LEFT) & PRESSED)) {
      paddles.at(0).node.get_transform().Translate(
          glm::vec3(-strafe_speed, 0, 0));
    }
    if ((inputHandler.GetKeycodeState(GLFW_KEY_J) & PRESSED)) {
      paddles.at(1).node.get_transform().Translate(
          glm::vec3(-strafe_speed, 0, 0));
    }
    if ((inputHandler.GetKeycodeState(GLFW_KEY_L) & PRESSED)) {
      paddles.at(1).node.get_transform().Translate(
          glm::vec3(strafe_speed, 0, 0));
    }

    // TODO: If you need to handle inputs, you can do it here

    mWindowManager.NewImGuiFrame();

    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    // TODO: Render all your geometry here.
    for (auto node : nodes) {
      node->render(mCamera.GetWorldToClipMatrix());
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // TODO: If you want a custom ImGUI window, you can set it up here

    // mWindowManager.RenderImGuiFrame(show_gui);
    glfwSwapBuffers(window);
  }
}

int main() {
  std::setlocale(LC_ALL, "");

  Bonobo framework;

  try {
    edaf80::Assignment5 assignment5(framework.GetWindowManager());
    assignment5.run();
  } catch (std::runtime_error const &e) {
    LogError(e.what());
  }
}
