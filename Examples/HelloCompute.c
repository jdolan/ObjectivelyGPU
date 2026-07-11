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
 * @brief MSAA sample count for the particle framebuffer.
 */
#define HELLO_MSAA SDL_GPU_SAMPLECOUNT_4

/**
 * @brief The number of particles animated by the compute shader.
 */
#define NUM_PARTICLES 256u

/**
 * @brief A particle position, written by the compute shader and read by the vertex shader.
 */
typedef struct {
  float x, y;
} Particle;

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
   * @brief The @c Framebuffer for particle rendering.
   */
  Framebuffer *framebuffer;

  /**
   * @brief The storage buffer of particle positions; written by compute, read by the vertex stage.
   */
  Buffer *particleBuffer;

  /**
   * @brief The compute pipeline that animates the particles.
   */
  ComputePipeline *computePipeline;

  /**
   * @brief The graphics pipeline that renders the particles as points.
   */
  GraphicsPipeline *graphicsPipeline;

  /**
   * @brief The application start time, in milliseconds.
   */
  Uint64 ticks;
} AppState;

static AppState application;

#pragma mark - Particle system

/**
 * @brief Initializes the particle buffer and the compute and graphics pipelines.
 */
static void initParticles(AppState *app) {

  app->particleBuffer = $(app->renderDevice, createBuffer, &(SDL_GPUBufferCreateInfo) {
    .usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
    .size = NUM_PARTICLES * sizeof(Particle),
  });

  app->computePipeline = $(app->renderDevice, loadComputePipeline, "HelloCompute.comp", &(SDL_GPUComputePipelineCreateInfo) {
    .num_readwrite_storage_buffers = 1,
    .num_uniform_buffers = 1,
    .threadcount_x = NUM_PARTICLES,
    .threadcount_y = 1,
    .threadcount_z = 1,
  });

  Shader *vertexShader = $(app->renderDevice, loadShader, "HelloCompute.vert", &(SDL_GPUShaderCreateInfo) {
    .stage = SDL_GPU_SHADERSTAGE_VERTEX,
    .num_storage_buffers = 1,
  });

  Shader *fragmentShader = $(app->renderDevice, loadShader, "HelloCompute.frag", &(SDL_GPUShaderCreateInfo) {
    .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
  });

  SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = GPU_GraphicsPipeline2D;
  pipelineInfo.vertex_shader = vertexShader->shader;
  pipelineInfo.fragment_shader = fragmentShader->shader;
  pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_POINTLIST;
  pipelineInfo.multisample_state.sample_count = app->framebuffer->sampleCount;
  pipelineInfo.target_info = (SDL_GPUGraphicsPipelineTargetInfo) {
    .color_target_descriptions = &(SDL_GPUColorTargetDescription) {
      .format = app->framebuffer->colorTextures[0]->format,
      .blend_state = GPU_BlendStateOpaque,
    },
    .num_color_targets = 1,
  };

  app->graphicsPipeline = $(app->renderDevice, createGraphicsPipeline, &pipelineInfo);

  release(vertexShader);
  release(fragmentShader);
}

/**
 * @brief Animates the particles via compute, then renders them as points.
 */
static void drawParticles(AppState *app, CommandBuffer *commands) {

  const float time = (float) (SDL_GetTicks() - app->ticks) / 1000.f;
  $(commands, pushComputeUniformData, 0, &time, sizeof(time));

  ComputePass *computePass = $(commands, beginComputePass, NULL, 0,
    &(SDL_GPUStorageBufferReadWriteBinding) { .buffer = app->particleBuffer->buffer }, 1);
  $(computePass, bindPipeline, app->computePipeline);
  $(computePass, dispatchCompute, 1, 1, 1);
  release(computePass);

  const SDL_FColor clearColor = { 0.05f, 0.05f, 0.1f, 1.f };
  const SDL_GPUColorTargetInfo color = $(app->framebuffer, colorTargetInfo, 0, SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_STORE, &clearColor);

  RenderPass *pass = $(commands, beginRenderPass, &color, 1, NULL);
  $(pass, bindPipeline, app->graphicsPipeline);
  $(pass, bindVertexStorageBuffers, 0, (SDL_GPUBuffer *[]) { app->particleBuffer->buffer }, 1);
  $(pass, drawPrimitives, NUM_PARTICLES, 1, 0, 0);
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

  app->window = SDL_CreateWindow("Hello Compute ObjectivelyGPU", HELLO_WINDOW_W, HELLO_WINDOW_H, HELLO_WINDOW_FLAGS);
  GPU_Assert(app->window, "SDL_CreateWindow");

  app->renderDevice = $(alloc(RenderDevice), initWithWindow, app->window, NULL);

  int w = 0, h = 0;
  SDL_GetWindowSizeInPixels(app->window, &w, &h);

  const SDL_GPUTextureFormat colorFormat = $(app->renderDevice, getSwapchainTextureFormat);
  
  app->framebuffer = $(app->renderDevice, createFramebuffer, &(GPU_FramebufferCreateInfo) {
    .size = MakeSize(w, h),
    .colorFormats = { colorFormat },
    .numColorTargets = 1,
    .sampleCount = HELLO_MSAA,
  });

  $(app->renderDevice, setFramebuffer, app->framebuffer);

  app->ticks = SDL_GetTicks();

  initParticles(app);

  return SDL_APP_CONTINUE;
}

/**
 * @brief SDL3 frame iteration callback.
 */
SDL_AppResult SDL_AppIterate(void *appState) {

  AppState *app = appState;

  CommandBuffer *commands = $(app->renderDevice, beginFrame);
  if (commands) {
    drawParticles(app, commands);
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

  release(app->graphicsPipeline);
  release(app->computePipeline);
  release(app->particleBuffer);

  release(app->framebuffer);
  release(app->renderDevice);

  SDL_DestroyWindow(app->window);
  SDL_Quit();
}
