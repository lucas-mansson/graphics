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
#include <chrono>
#include <cstdlib>
#include <functional>
#include <glm/common.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/type_ptr.hpp>
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
}

void load_shader(GLuint &shader, ShaderProgramManager &program_manager,
                 std::string name, std::string shaderFileName) {

  program_manager.CreateAndRegisterProgram(
      name.c_str(),
      {{ShaderType::vertex, shaderFileName + ".vert"},
       {ShaderType::fragment, shaderFileName + ".frag"}},
      shader);
  if (shader == 0u) {
    LogError("Failed to load fallback shader");
    exit(1);
  }
}

void edaf80::Assignment5::run() {
  setUpCamera(mCamera);

  ShaderProgramManager program_manager;

  GLuint fallback_shader = 0u;
  load_shader(fallback_shader, program_manager, "Fallback", "common/fallback");

  GLuint texcoord_shader = 0u;
  load_shader(texcoord_shader, program_manager, "Texture coords",
              "EDAF80/texcoord");

  GLuint rect_shader = 0u;
  load_shader(rect_shader, program_manager, "Rect", "EDAF80/pong_rect");

  GLuint ball_shader = 0u;
  load_shader(ball_shader, program_manager, "Ball", "EDAF80/pong_ball");

  GLuint skybox_shader = 0u;
  load_shader(skybox_shader, program_manager, "Skybox", "EDAF80/skybox");

  glm::vec3 light_position = glm::vec3(0.0f, 20.0f, 0.0f);
  glm::vec3 camera_position = glm::vec3(0.0f);

  auto rectUniforms = [&](glm::vec3 color, float opacity = 1.0f) {
      return [&light_position, &camera_position, color, opacity](GLuint program) {
          glUniform3fv(glGetUniformLocation(program, "base_color"), 1,
              glm::value_ptr(color));
          glUniform3fv(glGetUniformLocation(program, "light_position"), 1,
              glm::value_ptr(light_position));
          glUniform3fv(glGetUniformLocation(program, "camera_position"), 1,
              glm::value_ptr(camera_position));
          glUniform1f(glGetUniformLocation(program, "opacity"), opacity);
      };
  };

  auto ballUniforms = [&](GLuint program) {
      glUniform3fv(glGetUniformLocation(program, "light_position"), 1,
          glm::value_ptr(light_position));
      glUniform3fv(glGetUniformLocation(program, "camera_position"), 1,
          glm::value_ptr(camera_position));
      glUniform1f(glGetUniformLocation(program, "shininess"), 64.0f);
      };


  GLuint skybox_texture = bonobo::loadTextureCubeMap(
      config::resources_path("cubemaps/NissiBeach2/posx.jpg"),
      config::resources_path("cubemaps/NissiBeach2/negx.jpg"),
      config::resources_path("cubemaps/NissiBeach2/posy.jpg"),
      config::resources_path("cubemaps/NissiBeach2/negy.jpg"),
      config::resources_path("cubemaps/NissiBeach2/posz.jpg"),
      config::resources_path("cubemaps/NissiBeach2/negz.jpg"));

  Node skybox;
  skybox.set_geometry(parametric_shapes::createSphere(100.0f, 50u, 50u));
  skybox.set_program(&skybox_shader);
  skybox.add_texture("cubemap", skybox_texture, GL_TEXTURE_CUBE_MAP);

  std::vector<Node *> nodes;
  std::vector<Node *> glassNodes;

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
    paddle.node.set_program(&rect_shader, rectUniforms(i == 0 ? glm::vec3(0.2f, 0.6f, 1.0f) : glm::vec3(1.0f, 0.4f, 0.2f)));
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

  const glm::vec3 borderColor(0.7f, 0.85f, 1.0f);
  const float glassOpacity = 0.2f;

  std::array<Rectangle, 2> sideBorders;
  for (int i = 0; i < 2; i++) {
    Rectangle &border = sideBorders[i];
    border.node.set_geometry(borderShape);
    border.node.set_program(&rect_shader, rectUniforms(borderColor, glassOpacity));
    border.node.get_transform().SetTranslate(sideBorderPositions[i]);
    border.node.get_transform().RotateY(glm::half_pi<float>());

    border.normal = glm::vec3(0.0f, 0.0f, 1.0f);
    border.height = borderHeight;
    border.width = borderWidth;

    glassNodes.push_back(&sideBorders[i].node);
  }

  std::array<glm::vec3, 2> backBorderPositions = {
      glm::vec3(-borderWidth / 2.0f, borderY, borderWidth / 2.0f),
      glm::vec3(-borderWidth / 2.0f, borderY, -borderWidth / 2.0f),
  };
  std::array<Rectangle, 2> backBorders;
  for (size_t i = 0; i < backBorders.size(); i++) {
    Rectangle &border = backBorders[i];
    border.node.set_geometry(borderShape);
    border.node.set_program(&rect_shader, rectUniforms(borderColor, glassOpacity));
    border.node.get_transform().SetTranslate(backBorderPositions[i]);

    border.normal = glm::vec3(0, 0, 1);
    border.height = borderHeight;
    border.width = borderWidth;
    glassNodes.push_back(&border.node); 
  }

  Sphere ball;
  const float ballRadius = 1.0f;
  bonobo::mesh_data ballShape =
      parametric_shapes::createSphere(ballRadius, 100u, 100u);
  Node ballNode;
  ballNode.set_geometry(ballShape);
  ballNode.set_program(&ball_shader, ballUniforms);

  ball.node = ballNode;
  ball.radius = ballRadius;
  nodes.push_back(&ball.node);

  glClearDepthf(1.0f);
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  auto lastTime = std::chrono::high_resolution_clock::now();

  float const strafeSpeed = 0.1f;
  float const ballSpeed = 0.1f;

  auto const initalBallPosition = glm::vec3(0, 0, 0);
  auto const initalBallDirection = glm::vec3(0, 0, 1);  
  auto ballDirection = initalBallDirection;

  GameState currentGameState = WAITING_TO_START;
  PointStandings pointStandings{0, 0, 3};

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

    int framebuffer_width, framebuffer_height;
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
    glViewport(0, 0, framebuffer_width, framebuffer_height);

    camera_position = mCamera.mWorld.GetTranslation();

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

    skybox.get_transform().SetTranslate(camera_position);
    skybox.render(mCamera.GetWorldToClipMatrix());
    glClear(GL_DEPTH_BUFFER_BIT);

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

      // Bakvägg 0 (bakom Spelare 1, Blå) -> Poäng till Spelare 2 (Röd)
      CollisionResult backBoardCollison0 =
          checkRectangleSphereCollision(ball, backBorders.at(0));
      if (backBoardCollison0.collision) {
        ball.node.get_transform().SetTranslate(initalBallPosition);
        ballDirection = -initalBallDirection;
        pointStandings.player2Pts++;

        if (pointStandings.player2Pts >= pointStandings.pointsToWin) {
          currentGameState = ENDED;
        } else {
          currentGameState = WAITING_TO_START;
        }
      }

      // Bakvägg 1 (bakom Spelare 2, Röd) -> Poäng till Spelare 1 (Blå)
      CollisionResult backBoardCollison1 =
          checkRectangleSphereCollision(ball, backBorders.at(1));
      if (backBoardCollison1.collision) {
        ball.node.get_transform().SetTranslate(initalBallPosition);
        ballDirection = initalBallDirection;
        pointStandings.player1Pts++;

        if (pointStandings.player1Pts >= pointStandings.pointsToWin) {
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

    glDepthMask(GL_FALSE);
    for (auto node : glassNodes) {
        node->render(mCamera.GetWorldToClipMatrix());
    }
    glDepthMask(GL_TRUE);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    ImGui::Begin("Point standings");
    ImGui::Text("First to 3 wins!");
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "Player 1 (Bla): %d", pointStandings.player1Pts);
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.2f, 1.0f), "Player 2 (Rod): %d", pointStandings.player2Pts);
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