/*
 * ObjectivelyGPU: Object oriented GPU layer for SDL3 and GNU C.
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
 * @brief The Scene type.
 */
typedef struct {
  /**
   * @brief The cube vertex buffer.
   */
  SDL_GPUBuffer *vertexBuffer;

  /**
   * @brief The graphics pipeline.
   */
  SDL_GPUGraphicsPipeline *pipeline;

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

  SDL_GPUShader *vertexShader = $(app->renderDevice, loadShader, "Hello.vert", &(SDL_GPUShaderCreateInfo) {
    .stage = SDL_GPU_SHADERSTAGE_VERTEX,
    .num_uniform_buffers = 1,
  });

  SDL_GPUShader *fragmentShader = $(app->renderDevice, loadShader, "Hello.frag", &(SDL_GPUShaderCreateInfo) {
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

  const SDL_GPUColorTargetDescription ctd = {
    .format = app->framebuffer->colorFormat,
  };

  scene->pipeline = $(app->renderDevice, createGraphicsPipeline, &(SDL_GPUGraphicsPipelineCreateInfo) {
    .vertex_shader = vertexShader,
    .fragment_shader = fragmentShader,
    .vertex_input_state = {
      .vertex_buffer_descriptions = &vbd,
      .num_vertex_buffers = 1,
      .vertex_attributes = attrs,
      .num_vertex_attributes = (Uint32) SDL_arraysize(attrs),
    },
    .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
    .rasterizer_state = {
      .fill_mode = SDL_GPU_FILLMODE_FILL,
      .cull_mode = SDL_GPU_CULLMODE_BACK,
      .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
      .enable_depth_clip = true,
    },
    .depth_stencil_state = {
      .compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL,
      .enable_depth_test = true,
      .enable_depth_write = true,
    },
    .target_info = {
      .color_target_descriptions = &ctd,
      .num_color_targets = 1,
      .depth_stencil_format = app->framebuffer->depthFormat,
      .has_depth_stencil_target = true,
    },
  });

  $(app->renderDevice, releaseShader, vertexShader);
  $(app->renderDevice, releaseShader, fragmentShader);
}

/**
 * @brief Renders a single frame of the @c Scene.
 */
static void drawScene(AppState *app, CommandBuffer *cmd) {

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
  $(cmd, pushVertexUniformData, 0, modelViewProjection.f, sizeof(modelViewProjection));

  const SDL_FColor clearColor = { 0.1f, 0.1f, 0.2f, 1.f };
  const SDL_GPUColorTargetInfo color = $(app->framebuffer, colorTargetInfo, SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_STORE, &clearColor);
  const SDL_GPUDepthStencilTargetInfo depth = $(app->framebuffer, depthTargetInfo, SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_DONT_CARE, 1.f);

  RenderPass *pass = $(cmd, beginRenderPass, &color, 1, &depth);
  $(pass, bindPipeline, scene->pipeline);
  $(pass, bindVertexBuffers, 0, &(SDL_GPUBufferBinding) { .buffer = scene->vertexBuffer }, 1);
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

  const SDL_GPUTextureFormat colorFormat = $(app->renderDevice, getSwapchainTextureFormat, app->window);
  app->framebuffer = $(alloc(Framebuffer), initWithDevice, app->renderDevice,
    &MakeSize(w, h),
    colorFormat,
    SDL_GPU_TEXTUREFORMAT_D16_UNORM);

  initScene(app);

  return SDL_APP_CONTINUE;
}

/**
 * @brief SDL3 frame iteration callback.
 */
SDL_AppResult SDL_AppIterate(void *appState) {

  AppState *app = appState;

  CommandBuffer *cmd = $(app->renderDevice, acquireCommandBuffer);

  SwapchainTexture swapchain = { 0 };
  $(cmd, waitAndAcquireSwapchainTexture, &swapchain);

  if (!swapchain.texture) {
    $(cmd, cancel);
    release(cmd);
    return SDL_APP_CONTINUE;
  }

  $(app->framebuffer, resize, &swapchain.size);

  drawScene(app, cmd);

  $(cmd, blitTexture, &(SDL_GPUBlitInfo) {
    .source = {
      .texture = app->framebuffer->colorTexture,
      .w = (Uint32) swapchain.size.w,
      .h = (Uint32) swapchain.size.h,
    },
    .destination = {
      .texture = swapchain.texture,
      .w = (Uint32) swapchain.size.w,
      .h = (Uint32) swapchain.size.h,
    },
    .load_op = SDL_GPU_LOADOP_DONT_CARE,
    .filter = SDL_GPU_FILTER_NEAREST,
  });

  $(app->renderDevice, submit, cmd);
  release(cmd);

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
  $(app->renderDevice, releaseGraphicsPipeline, app->scene.pipeline);
  $(app->renderDevice, releaseBuffer, app->scene.vertexBuffer);

  release(app->framebuffer);
  release(app->renderDevice);

  SDL_DestroyWindow(app->window);
  SDL_Quit();
}
