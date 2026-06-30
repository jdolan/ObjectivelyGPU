/*
 * ObjectivelyGPU: Object oriented graphics framework for SDL3 and C.
 * Copyright (C) 2026 Jay Dolan <jay@jaydolan.com>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 */

#define SDL_MAIN_USE_CALLBACKS

#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>

#include <Objectively.h>
#include <ObjectivelyGPU.h>

#ifdef SDL_PLATFORM_IOS
# define HELLO_WINDOW_W      0
# define HELLO_WINDOW_H      0
# define HELLO_WINDOW_FLAGS  (SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_FULLSCREEN)
#else
# define HELLO_WINDOW_W      1024
# define HELLO_WINDOW_H      720
# define HELLO_WINDOW_FLAGS  SDL_WINDOW_HIGH_PIXEL_DENSITY
#endif

/**
 * @brief MSAA sample count for the scene framebuffer.
 */
#define HELLO_MSAA SDL_GPU_SAMPLECOUNT_4

/**
 * @brief The Scene type.
 */
typedef struct {
  /**
   * @brief The cube vertex buffer.
   */
  Buffer *vertexBuffer;

  /**
   * @brief The graphics pipeline.
   */
  GraphicsPipeline *pipeline;

  /**
   * @brief The cube angles.
   */
  vec2 angles;
} Scene;

/**
 * @brief SDL application state passed via pointer to callbacks.
 */
typedef struct {

  /**
   * @brief The @c SDL_Window.
   */
  SDL_Window *window;

  /**
   * @brief The ObjectivelyGPU @c RenderDevice.
   */
  RenderDevice *renderDevice;

  /**
   * @brief The @c Framebuffer for scene rendering.
   */
  Framebuffer *framebuffer;

  /**
   * @brief Simulation time in milliseconds.
   */
  Uint64 ticks;

  /**
   * @brief The @c Scene.
   */
  Scene scene;
} AppState;

static AppState application;

#pragma mark - Scene management

typedef struct {
  vec3 position;
  vec3 color;
} Vertex;

static const Vertex vertices[] = {
  { { -0.5f,  0.5f, -0.5f }, { 1, 0, 0 } }, { {  0.5f, -0.5f, -0.5f }, { 0, 0, 1 } }, { { -0.5f, -0.5f, -0.5f }, { 0, 1, 0 } },
  { { -0.5f,  0.5f, -0.5f }, { 1, 0, 0 } }, { {  0.5f,  0.5f, -0.5f }, { 1, 1, 0 } }, { {  0.5f, -0.5f, -0.5f }, { 0, 0, 1 } },
  { { -0.5f,  0.5f,  0.5f }, { 1, 1, 1 } }, { { -0.5f, -0.5f, -0.5f }, { 0, 1, 0 } }, { { -0.5f, -0.5f,  0.5f }, { 0, 1, 1 } },
  { { -0.5f,  0.5f,  0.5f }, { 1, 1, 1 } }, { { -0.5f,  0.5f, -0.5f }, { 1, 0, 0 } }, { { -0.5f, -0.5f, -0.5f }, { 0, 1, 0 } },
  { { -0.5f,  0.5f,  0.5f }, { 1, 1, 1 } }, { {  0.5f,  0.5f, -0.5f }, { 1, 1, 0 } }, { { -0.5f,  0.5f, -0.5f }, { 1, 0, 0 } },
  { { -0.5f,  0.5f,  0.5f }, { 1, 1, 1 } }, { {  0.5f,  0.5f,  0.5f }, { 0, 0, 0 } }, { {  0.5f,  0.5f, -0.5f }, { 1, 1, 0 } },
  { {  0.5f,  0.5f, -0.5f }, { 1, 1, 0 } }, { {  0.5f, -0.5f,  0.5f }, { 1, 0, 1 } }, { {  0.5f, -0.5f, -0.5f }, { 0, 0, 1 } },
  { {  0.5f,  0.5f, -0.5f }, { 1, 1, 0 } }, { {  0.5f,  0.5f,  0.5f }, { 0, 0, 0 } }, { {  0.5f, -0.5f,  0.5f }, { 1, 0, 1 } },
  { {  0.5f,  0.5f,  0.5f }, { 0, 0, 0 } }, { { -0.5f, -0.5f,  0.5f }, { 0, 1, 1 } }, { {  0.5f, -0.5f,  0.5f }, { 1, 0, 1 } },
  { {  0.5f,  0.5f,  0.5f }, { 0, 0, 0 } }, { { -0.5f,  0.5f,  0.5f }, { 1, 1, 1 } }, { { -0.5f, -0.5f,  0.5f }, { 0, 1, 1 } },
  { { -0.5f, -0.5f, -0.5f }, { 0, 1, 0 } }, { {  0.5f, -0.5f,  0.5f }, { 1, 0, 1 } }, { { -0.5f, -0.5f,  0.5f }, { 0, 1, 1 } },
  { { -0.5f, -0.5f, -0.5f }, { 0, 1, 0 } }, { {  0.5f, -0.5f, -0.5f }, { 0, 0, 1 } }, { {  0.5f, -0.5f,  0.5f }, { 1, 0, 1 } },
};

/**
 * @brief Initializes the @c Scene.
 */
static void initScene(AppState *app) {

  Scene *scene = &app->scene;

  scene->vertexBuffer = $(app->renderDevice, createBufferWithConstMem, SDL_GPU_BUFFERUSAGE_VERTEX, vertices, sizeof(vertices));

  Shader *vertexShader = $(app->renderDevice, loadShader, "Hello.vert", &(SDL_GPUShaderCreateInfo) {
    .stage = SDL_GPU_SHADERSTAGE_VERTEX,
    .num_uniform_buffers = 1,
  });

  Shader *fragmentShader = $(app->renderDevice, loadShader, "Hello.frag", &(SDL_GPUShaderCreateInfo) {
    .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
  });

  const SDL_GPUVertexBufferDescription vbd = {
    .slot = 0,
    .pitch = sizeof(Vertex),
    .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
  };

  const SDL_GPUVertexAttribute attrs[] = {
    {
      .location = 0,
      .buffer_slot = 0,
      .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
      .offset = offsetof(Vertex, position),
    },
    {
      .location = 1,
      .buffer_slot = 0,
      .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
      .offset = offsetof(Vertex, color),
    },
  };

  SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = GPU_GraphicsPipeline3D;
  pipelineInfo.vertex_shader = vertexShader->shader;
  pipelineInfo.fragment_shader = fragmentShader->shader;
  pipelineInfo.vertex_input_state = (SDL_GPUVertexInputState) {
    .vertex_buffer_descriptions = &vbd,
    .num_vertex_buffers = 1,
    .vertex_attributes = attrs,
    .num_vertex_attributes = (Uint32) SDL_arraysize(attrs),
  };
  pipelineInfo.multisample_state.sample_count = app->framebuffer->sampleCount;
  pipelineInfo.target_info = (SDL_GPUGraphicsPipelineTargetInfo) {
    .color_target_descriptions = &(SDL_GPUColorTargetDescription) {
      .format = app->framebuffer->colorTextures[0]->format,
      .blend_state = GPU_BlendStateOpaque,
    },
    .num_color_targets = 1,
    .depth_stencil_format = app->framebuffer->depthTexture->format,
    .has_depth_stencil_target = true,
  };

  scene->pipeline = $(app->renderDevice, createGraphicsPipeline, &pipelineInfo);

  release(vertexShader);
  release(fragmentShader);
}

/**
 * @brief Renders a single frame of the @c Scene.
 */
static void drawScene(AppState *app, CommandBuffer *commands) {

  const Uint64 ticks = SDL_GetTicks();
  const float dt = (ticks - app->ticks) / 1000.f;
  app->ticks = ticks;

  Scene *scene = &app->scene;

  scene->angles.x = SDL_fmodf(scene->angles.x + dt * 30.f, 360.f);
  scene->angles.y = SDL_fmodf(scene->angles.y + dt * 60.f, 360.f);

  mat4 modelView = mat4_rotation(scene->angles.x, vec3_new(1.f, 0.f, 0.f));
  modelView = mat4_mul(mat4_rotation(scene->angles.y, vec3_new(0.f, 1.f, 0.f)), modelView);
  modelView = mat4_mul(mat4_translation(vec3_new(0.f, 0.f, -2.5f)), modelView);

  const mat4 projection = mat4_perspective(45.f, (float) app->framebuffer->size.w / (float) app->framebuffer->size.h, 0.01f, 100.f);
  const mat4 modelViewProjection = mat4_mul(projection, modelView);
  $(commands, pushVertexUniformData, 0, modelViewProjection.f, sizeof(modelViewProjection));

  const SDL_FColor clearColor = { 0.1f, 0.1f, 0.2f, 1.f };
  const SDL_GPUColorTargetInfo color = $(app->framebuffer, colorTargetInfo, 0, SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_STORE, &clearColor);
  const SDL_GPUDepthStencilTargetInfo depth = $(app->framebuffer, depthTargetInfo, SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_DONT_CARE, 1.f);

  RenderPass *pass = $(commands, beginRenderPass, &color, 1, &depth);
  $(pass, bindPipeline, scene->pipeline);
  $(pass, bindVertexBuffers, 0, &(SDL_GPUBufferBinding) { .buffer = scene->vertexBuffer->buffer }, 1);
  $(pass, drawPrimitives, (Uint32) SDL_arraysize(vertices), 1, 0, 0);
  release(pass);
}

#pragma mark - SDL application callbacks

/**
 * @brief SDL3 application initialization callback.
 */
SDL_AppResult SDL_AppInit(void **appState, int argc, char *argv[]) {

  AppState *app = *appState = &application;

  GPU_Assert(SDL_Init(SDL_INIT_VIDEO), "SDL_Init");

#ifdef EXAMPLES
  $$(Resource, addResourcePath, EXAMPLES);
#endif

  app->window = SDL_CreateWindow("Hello ObjectivelyGPU", HELLO_WINDOW_W, HELLO_WINDOW_H, HELLO_WINDOW_FLAGS);
  GPU_Assert(app->window, "SDL_CreateWindow");

  app->renderDevice = $(alloc(RenderDevice), initWithWindow, app->window);

  int w = 0, h = 0;
  SDL_GetWindowSizeInPixels(app->window, &w, &h);

  const SDL_GPUTextureFormat colorFormat = $(app->renderDevice, getSwapchainTextureFormat);
  app->framebuffer = $(app->renderDevice, createFramebuffer, &(GPU_FramebufferCreateInfo) {
    .size = MakeSize(w, h),
    .colorFormats = { colorFormat },
    .numColorTargets = 1,
    .depthFormat = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
    .sampleCount = HELLO_MSAA,
  });

  $(app->renderDevice, setFramebuffer, app->framebuffer);

  initScene(app);

  return SDL_APP_CONTINUE;
}

/**
 * @brief SDL3 frame iteration callback.
 */
SDL_AppResult SDL_AppIterate(void *appState) {

  AppState *app = appState;

  CommandBuffer *commands = $(app->renderDevice, beginFrame);
  if (commands) {
    drawScene(app, commands);
    $(app->renderDevice, endFrame);
  }

  return SDL_APP_CONTINUE;
}

/**
 * @brief SDL3 event callback.
 */
SDL_AppResult SDL_AppEvent(void *appState, SDL_Event *event) {

  switch (event->type) {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      return SDL_APP_SUCCESS;
    default:
      return SDL_APP_CONTINUE;
  }
}

/**
 * @brief SDL3 quit callback.
 */
void SDL_AppQuit(void *appState, SDL_AppResult result) {

  AppState *app = appState;

  $(app->renderDevice, waitForIdle);
  release(app->scene.pipeline);
  release(app->scene.vertexBuffer);

  release(app->framebuffer);
  release(app->renderDevice);

  SDL_DestroyWindow(app->window);
  SDL_Quit();
}
