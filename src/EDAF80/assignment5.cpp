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
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/geometric.hpp>
#include <imgui.h>
#include <string>
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

enum GameState {
  WAITING_TO_START,
  STARTED,
  ENDED,
};

struct PointStandings {
  int player1Pts;
  int player2Pts;
  const int pointsToWin;
};

CollisionResult checkRectangleSphereCollision(Sphere sphere, Rectangle rec) {
  // transform sphere into rectangles local space
  glm::mat4 model = rec.node.get_transform().GetMatrix();
  glm::mat4 inverseModel = glm::inverse(model);

  auto const sphereCenter = sphere.node.get_transform().GetTranslation();
  auto const localSphereCenter =
      glm::vec3(inverseModel * glm::vec4(sphereCenter, 1.0f));

  auto const recPt = rec.node.get_transform().GetTranslation();

  glm::vec3 localClosestPoint;
  localClosestPoint.x = glm::clamp(localSphereCenter.x, 0.0f, rec.width);
  localClosestPoint.y = glm::clamp(localSphereCenter.y, 0.0f, rec.height);
  localClosestPoint.z = 0.0f;

  glm::vec3 closestPoint =
      glm::vec3(model * glm::vec4(localClosestPoint, 1.0f));

  auto const delta = sphereCenter - closestPoint;
  float distSq = glm::dot(delta, delta);

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

void setUpCamera(FPSCameraf &mCamera) {
  mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 30.0f, 25.0f));
  mCamera.mWorld.LookAt(glm::vec3(0, 0, 0));
  mCamera.mMouseSensitivity = glm::vec2(0.003f);
  // mCamera.mMovementSpeed = glm::vec3(3.0f); // 3 m/s => 10.8 km/h
}

void load_shader(GLuint &shader, ShaderProgramManager &program_manager,
                 std::string name, std::string shaderFileName) {

  program_manager.CreateAndRegisterProgram(
      "Fallback",
      {{ShaderType::vertex, shaderFileName + ".vert"},
       {ShaderType::fragment, shaderFileName + ".frag"}},
      shader);
  if (shader == 0u) {
    LogError("Failed to load fallback shader");
    exit(1);
  }
}

void edaf80::Assignment5::run() {
  // Set up the camera
  setUpCamera(mCamera);

  // Create the shader programs
  // TODO: Insert the creation of other shader programs.
  ShaderProgramManager program_manager;
  GLuint fallback_shader = 0u;
  load_shader(fallback_shader, program_manager, "Fallback", "common/fallback");

  GLuint texcoord_shader = 0u;
  load_shader(texcoord_shader, program_manager, "Texture coords",
              "EDAF80/texcoord");

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

  const float borderWidth = 30.0f;
  const float borderHeight = 10.0f;
  const float borderX = borderWidth / 2.0f;
  const float borderY = -borderHeight / 2.0f;
  const float borderZ = borderWidth / 2.0f;
  bonobo::mesh_data borderShape =
      parametric_shapes::createQuadXY(borderWidth, borderHeight);
  std::array<glm::vec3, 2> sideBorderPositions = {
      glm::vec3(borderX, borderY, borderZ),
      glm::vec3(-borderX, borderY, borderZ),
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

  std::array<glm::vec3, 2> backBorderPositions = {
      glm::vec3(-borderWidth / 2.0f, borderY, borderWidth / 2.0f),
      glm::vec3(-borderWidth / 2.0f, borderY, -borderWidth / 2.0f),
  };
  std::array<Rectangle, 2> backBorders;
  for (int i = 0; i < backBorders.size(); i++) {
    Rectangle &border = backBorders[i];
    border.node.set_geometry(borderShape);
    border.node.set_program(&texcoord_shader);
    border.node.get_transform().SetTranslate(backBorderPositions[i]);

    border.normal = glm::vec3(0, 0, 1);
    border.height = borderHeight;
    border.width = borderWidth;
    nodes.push_back(&border.node);
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

  glClearDepthf(1.0f);
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glEnable(GL_DEPTH_TEST);

  auto lastTime = std::chrono::high_resolution_clock::now();

  float const strafeSpeed = 0.1f;
  float const ballSpeed = 0.1f;

  auto const initalBallPosition = glm::vec3(0, 0, 0);
  auto const initalBallDirection = glm::vec3(0, 0, 1);
  auto ballDirection = initalBallDirection;

  GameState currentGameState = WAITING_TO_START;
  auto const pointsToWin = 3;
  PointStandings pointStandings{
      .player1Pts = 0, .player2Pts = 0, .pointsToWin = 3};

  while (!glfwWindowShouldClose(window)) {

    auto const nowTime = std::chrono::high_resolution_clock::now();
    auto const deltaTimeUs =
        std::chrono::duration_cast<std::chrono::microseconds>(nowTime -
                                                              lastTime);
    lastTime = nowTime;

    auto &io = ImGui::GetIO();
    inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

    glfwPollEvents();
    inputHandler.Advance();
    // mCamera.Update(deltaTimeUs, inputHandler);

    int framebuffer_width, framebuffer_height;
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
    glViewport(0, 0, framebuffer_width, framebuffer_height);

    // TODO: If you need to handle inputs, you can do it here
    if ((inputHandler.GetKeycodeState(GLFW_KEY_RIGHT) & PRESSED)) {
      paddles.at(0).node.get_transform().Translate(
          glm::vec3(strafeSpeed, 0, 0));
    }
    if ((inputHandler.GetKeycodeState(GLFW_KEY_LEFT) & PRESSED)) {
      paddles.at(0).node.get_transform().Translate(
          glm::vec3(-strafeSpeed, 0, 0));
    }
    if ((inputHandler.GetKeycodeState(GLFW_KEY_A) & PRESSED)) {
      paddles.at(1).node.get_transform().Translate(
          glm::vec3(-strafeSpeed, 0, 0));
    }
    if ((inputHandler.GetKeycodeState(GLFW_KEY_D) & PRESSED)) {
      paddles.at(1).node.get_transform().Translate(
          glm::vec3(strafeSpeed, 0, 0));
    }

    mWindowManager.NewImGuiFrame();
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    // TODO: Render all your geometry here.
    switch (currentGameState) {
    case ENDED:
      return;

    case WAITING_TO_START:
      if ((inputHandler.GetKeycodeState(GLFW_KEY_ENTER) & JUST_RELEASED)) {
        currentGameState = STARTED;
      }
      break;

    case STARTED:
      ball.node.get_transform().Translate(ballDirection * ballSpeed);
      CollisionResult backBoardCollison0 =
          checkRectangleSphereCollision(ball, backBorders.at(0));
      if (backBoardCollison0.collision) {
        ball.node.get_transform().SetTranslate(initalBallPosition);
        pointStandings.player1Pts++;

        if (pointStandings.player1Pts == pointStandings.pointsToWin) {
          currentGameState = ENDED;
        } else {
          currentGameState = WAITING_TO_START;
        }
      }
      CollisionResult backBoardCollison1 =
          checkRectangleSphereCollision(ball, backBorders.at(1));
      if (backBoardCollison1.collision) {
        ball.node.get_transform().SetTranslate(initalBallPosition);
        pointStandings.player2Pts++;
        if (pointStandings.player2Pts == pointStandings.pointsToWin) {
          currentGameState = ENDED;
        } else {
          currentGameState = WAITING_TO_START;
        }
      }

      for (auto paddle : paddles) {
        CollisionResult collisionPaddle =
            checkRectangleSphereCollision(ball, paddle);
        if (collisionPaddle.collision) {
          ballDirection = glm::normalize(
              glm::reflect(ballDirection, collisionPaddle.collisionNormal));
        }
      }
      for (auto border : sideBorders) {
        CollisionResult collisionSide =
            checkRectangleSphereCollision(ball, border);
        if (collisionSide.collision) {
          ballDirection = glm::normalize(
              glm::reflect(ballDirection, collisionSide.collisionNormal));
        }
      }

      break;
    }

    for (auto node : nodes) {
      node->render(mCamera.GetWorldToClipMatrix());
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // TODO: If you want a custom ImGUI window, you can set it up here
    ImGui::Begin("Point standings");
    ImGui::Begin("First to 3 wins!");
    ImGui::Text(
        ("Player 1:" + std::to_string(pointStandings.player1Pts)).c_str());
    ImGui::Separator();
    ImGui::Text(
        ("Player 2:" + std::to_string(pointStandings.player2Pts)).c_str());
    ImGui::Separator();
    ImGui::End();

    mWindowManager.RenderImGuiFrame(true);
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
