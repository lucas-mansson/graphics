#include "assignment5.hpp"

#include "EDAF80/parametric_shapes.hpp"
#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/ShaderProgramManager.hpp"
#include "core/helpers.hpp"
#include "core/node.hpp"

#include <array>
#include <cstdlib>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/ext/vector_float3.hpp>
#include <imgui.h>
#include <tinyfiledialogs.h>

#include <clocale>
#include <stdexcept>

struct Sphere {
  Node node;
  float radius;
};

struct Plane {
  Node node;
  float height;
  float width;
  glm::vec3 normal;
};

bool checkSpherePlaneCollision(Sphere sphere, Plane plane) {
  auto const sphereCenter = sphere.node.get_transform().GetTranslation();
  auto const planePt = plane.node.get_transform().GetTranslation();
  glm::vec3 n = glm::normalize(plane.normal);

  float distance = glm::dot(n, sphereCenter - planePt);

  return std::abs(distance) <= sphere.radius;
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

  Plane paddle1;
  Node paddle1Node;
  glm::vec3 paddle1Pos = glm::vec3(paddleX, paddleY, paddleDistanceFromOrigo);
  paddle1Node.set_geometry(paddleShape);
  paddle1Node.set_program(&texcoord_shader);
  paddle1Node.get_transform().SetTranslate(paddle1Pos);
  paddle1.node = paddle1Node;
  paddle1.normal = glm::vec3(0.0f, 0.0f, 1.0f);
  paddle1.height = paddleSideSize;
  paddle1.width = paddleSideSize;
  nodes.push_back(&paddle1.node);

  Plane paddle2;
  Node paddle2Node;
  glm::vec3 paddle2Pos = glm::vec3(paddleX, paddleY, -paddleDistanceFromOrigo);
  paddle2Node.set_geometry(paddleShape);
  paddle2Node.set_program(&texcoord_shader);
  paddle2Node.get_transform().SetTranslate(paddle2Pos);
  paddle2.node = paddle2Node;
  paddle2.normal = glm::vec3(0.0f, 0.0f, 1.0f);
  paddle2.height = paddleSideSize;
  paddle2.width = paddleSideSize;
  nodes.push_back(&paddle2.node);

  // 4 sides
  const int nbrBorders = 4;
  const float borderLength = 30.0f;
  const float borderHeight = 10.0f;
  const float borderY = -borderHeight / 2.0f;
  bonobo::mesh_data sideshape =
      parametric_shapes::createQuadXY(borderLength, borderHeight);

  std::array<glm::vec3, 4> borderPositions = {
      glm::vec3(-borderLength / 2.0f, borderY, borderLength / 2.0f),
      glm::vec3(-borderLength / 2.0f, borderY, -borderLength / 2.0f),
      glm::vec3(-borderLength / 2.0f, borderY, borderLength / 2.0f),
      glm::vec3(borderLength / 2.0f, borderY, borderLength / 2.0f)};

  std::array<Node, 4> borders;
  for (std::size_t i = 0; i < borders.size(); ++i) {
    Node &border = borders[i];
    border.set_geometry(sideshape);
    border.set_program(&texcoord_shader);
    if (i >= 2) {
      border.get_transform().RotateY(glm::half_pi<float>());
    }
    border.get_transform().SetTranslate(borderPositions[i]);
    nodes.push_back(&border);
  }

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

  auto const ballDirection = glm::vec3(0, 0, 1);

  glClearDepthf(1.0f);
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glEnable(GL_DEPTH_TEST);

  auto lastTime = std::chrono::high_resolution_clock::now();

  bool show_logs = true;
  bool show_gui = true;
  bool shader_reload_failed = false;
  bool show_basis = false;
  float basis_thickness_scale = 1.0f;
  float basis_length_scale = 1.0f;

  while (!glfwWindowShouldClose(window)) {
    auto const nowTime = std::chrono::high_resolution_clock::now();
    auto const deltaTimeUs =
        std::chrono::duration_cast<std::chrono::microseconds>(nowTime -
                                                              lastTime);
    lastTime = nowTime;

    ball.node.get_transform().Translate(ballDirection / 10.0f);

    bool collision = checkSpherePlaneCollision(ball, paddle1);
    std::cout << collision << "\n";
    // check collision
    // If collision, update ballDirection vector

    auto &io = ImGui::GetIO();
    inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

    glfwPollEvents();
    inputHandler.Advance();
    mCamera.Update(deltaTimeUs, inputHandler);

    if (inputHandler.GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
      shader_reload_failed = !program_manager.ReloadAllPrograms();
      if (shader_reload_failed)
        tinyfd_notifyPopup("Shader Program Reload Error",
                           "An error occurred while reloading shader programs; "
                           "see the logs for details.\n"
                           "Rendering is suspended until the issue is solved. "
                           "Once fixed, just reload the shaders again.",
                           "error");
    }
    if (inputHandler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
      show_logs = !show_logs;
    if (inputHandler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
      show_gui = !show_gui;
    if (inputHandler.GetKeycodeState(GLFW_KEY_F11) & JUST_RELEASED)
      mWindowManager.ToggleFullscreenStatusForWindow(window);

    int framebuffer_width, framebuffer_height;
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
    glViewport(0, 0, framebuffer_width, framebuffer_height);

    // TODO: If you need to handle inputs, you can do it here

    mWindowManager.NewImGuiFrame();

    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    if (!shader_reload_failed) {
      // TODO: Render all your geometry here.
      for (auto node : nodes) {
        node->render(mCamera.GetWorldToClipMatrix());
      }
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // TODO: If you want a custom ImGUI window, you can set it up here
    bool const opened =
        ImGui::Begin("Scene Controls", nullptr, ImGuiWindowFlags_None);
    if (opened) {
      ImGui::Checkbox("Show basis", &show_basis);
      ImGui::SliderFloat("Basis thickness scale", &basis_thickness_scale, 0.0f,
                         100.0f);
      ImGui::SliderFloat("Basis length scale", &basis_length_scale, 0.0f,
                         100.0f);
    }
    ImGui::End();

    if (show_basis)
      bonobo::renderBasis(basis_thickness_scale, basis_length_scale,
                          mCamera.GetWorldToClipMatrix());
    if (show_logs)
      Log::View::Render();
    mWindowManager.RenderImGuiFrame(show_gui);

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
