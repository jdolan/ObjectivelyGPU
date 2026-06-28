/*
 * ObjectivelyGPU: Object oriented MVC framework for SDL3 and GNU C.
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

#include <SDL3/SDL.h>

#include <Objectively.h>
#include <ObjectivelyGPU.h>

#ifndef EXAMPLES
# define EXAMPLES "."
#endif

typedef struct {
  RenderDevice *renderDevice;
  Framebuffer *framebuffer;
  SDL_GPUGraphicsPipeline *pipeline;
  SDL_GPUBuffer *vertexBuffer;
} Scene;

typedef struct {
	vec3 position;
	vec3 color;
} Vertex;

static const Vertex vertexes[] = {
	{ -0.5f,  0.5f, -0.5f,  1, 0, 0 }, {  0.5f, -0.5f, -0.5f, 0, 0, 1 }, { -0.5f, -0.5f, -0.5f, 0, 1, 0 },
	{ -0.5f,  0.5f, -0.5f,  1, 0, 0 }, {  0.5f,  0.5f, -0.5f, 1, 1, 0 }, {  0.5f, -0.5f, -0.5f, 0, 0, 1 },
	{ -0.5f,  0.5f,  0.5f,  1, 1, 1 }, { -0.5f, -0.5f, -0.5f, 0, 1, 0 }, { -0.5f, -0.5f,  0.5f, 0, 1, 1 },
	{ -0.5f,  0.5f,  0.5f,  1, 1, 1 }, { -0.5f,  0.5f, -0.5f, 1, 0, 0 }, { -0.5f, -0.5f, -0.5f, 0, 1, 0 },
	{ -0.5f,  0.5f,  0.5f,  1, 1, 1 }, {  0.5f,  0.5f, -0.5f, 1, 1, 0 }, { -0.5f,  0.5f, -0.5f, 1, 0, 0 },
	{ -0.5f,  0.5f,  0.5f,  1, 1, 1 }, {  0.5f,  0.5f,  0.5f, 0, 0, 0 }, {  0.5f,  0.5f, -0.5f, 1, 1, 0 },
	{  0.5f,  0.5f, -0.5f,  1, 1, 0 }, {  0.5f, -0.5f,  0.5f, 1, 0, 1 }, {  0.5f, -0.5f, -0.5f, 0, 0, 1 },
	{  0.5f,  0.5f, -0.5f,  1, 1, 0 }, {  0.5f,  0.5f,  0.5f, 0, 0, 0 }, {  0.5f, -0.5f,  0.5f, 1, 0, 1 },
	{  0.5f,  0.5f,  0.5f,  0, 0, 0 }, { -0.5f, -0.5f,  0.5f, 0, 1, 1 }, {  0.5f, -0.5f,  0.5f, 1, 0, 1 },
	{  0.5f,  0.5f,  0.5f,  0, 0, 0 }, { -0.5f,  0.5f,  0.5f, 1, 1, 1 }, { -0.5f, -0.5f,  0.5f, 0, 1, 1 },
	{ -0.5f, -0.5f, -0.5f,  0, 1, 0 }, {  0.5f, -0.5f,  0.5f, 1, 0, 1 }, { -0.5f, -0.5f,  0.5f, 0, 1, 1 },
	{ -0.5f, -0.5f, -0.5f,  0, 1, 0 }, {  0.5f, -0.5f, -0.5f, 0, 0, 1 }, {  0.5f, -0.5f,  0.5f, 1, 0, 1 },
};

/**
 * @brief
 */
static void drawScene(Scene *scene) {

  static vec2 angles;
  static Uint64 lastTicks;

  Uint64 ticks = SDL_GetTicks();
  float dt = (float) (ticks - lastTicks) / 1000.0f;
  lastTicks = ticks;

  angles.x += dt * 30.0f;
  angles.y += dt * 60.0f;

  while (angles.x >= 360.0f) {
    angles.x -= 360.0f;
  }
  while (angles.y >= 360.0f) {
    angles.y -= 360.0f;
  }

  CommandBuffer *cmd = $(scene->renderDevice, acquireCommandBuffer);

  SwapchainTexture swapchain = { 0 };
  $(cmd, waitAndAcquireSwapchainTexture, &swapchain);

  mat4 modelView = mat4_rotation(angles.x, vec3_new(1.f, 0.f, 0.f));
  modelView = mat4_mul(mat4_rotation(angles.y, vec3_new(0.f, 1.f, 0.f)), modelView);
  modelView = mat4_mul(mat4_translation(vec3_new(0.f, 0.f, -2.5f)), modelView);

  const mat4 projection = mat4_perspective(45.f, (float) swapchain.size.w / (float) swapchain.size.h, 0.01f, 100.f);
  const mat4 modelViewProjection = mat4_mul(projection, modelView);

  SDL_GPUColorTargetInfo colorTarget = {
    .texture = swapchain.texture,
    .clear_color = { 0.1f, 0.1f, 0.2f, 1.0f },
    .load_op = SDL_GPU_LOADOP_CLEAR,
    .store_op = SDL_GPU_STOREOP_STORE,
  };

  SDL_GPUDepthStencilTargetInfo depthTarget = $(scene->framebuffer, depthTargetInfo, SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_DONT_CARE, 1.f);

  RenderPass *renderPass = $(cmd, beginRenderPass, &colorTarget, 1, &depthTarget);
  $(renderPass, bindPipeline, scene->pipeline);
  $(renderPass, bindVertexBuffers, 0, &(SDL_GPUBufferBinding) {
    .buffer = scene->vertexBuffer,
    .offset = 0,
  }, 1);
  $(cmd, pushVertexUniformData, 0, modelViewProjection.f, sizeof(modelViewProjection));
  $(renderPass, drawPrimitives, (Uint32) SDL_arraysize(vertexes), 1, 0, 0);
  release(renderPass);

  $(cmd, submit);
  release(cmd);
}

/**
 * @brief
 */
int main(int argc, char **argv) {

  $$(Resource, addResourcePath, EXAMPLES);

  GPU_Assert(SDL_Init(SDL_INIT_VIDEO), "SDL_Init");

	SDL_Window *window = SDL_CreateWindow("Hello ObjectivelyGPU", 1024, 720, SDL_WINDOW_HIGH_PIXEL_DENSITY);
  GPU_Assert(window, "SDL_CreateWindow");

  RenderDevice *renderDevice = $(alloc(RenderDevice), initWithWindow, window);
  GPU_Assert(renderDevice, "RenderDevice init");

	int w, h;
  SDL_GetWindowSizeInPixels(window, &w, &h);

  Framebuffer *framebuffer = $(alloc(Framebuffer), initWithDevice, renderDevice,
    &MakeSize(w, h),
    SDL_GPU_TEXTUREFORMAT_INVALID,
    SDL_GPU_TEXTUREFORMAT_D16_UNORM);

	SDL_GPUShader *vertexShader = $(renderDevice, loadShader, "Hello.vert", &(SDL_GPUShaderCreateInfo) {
		.entrypoint = "vs_main",
		.stage = SDL_GPU_SHADERSTAGE_VERTEX,
		.num_uniform_buffers = 1,
	});

  SDL_GPUShader *fragmentShader = $(renderDevice, loadShader, "Hello.frag", &(SDL_GPUShaderCreateInfo) {
		.entrypoint = "fs_main",
		.stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
	});

	SDL_GPUColorTargetDescription colorTargetDescription = {
		.format = $(renderDevice, getSwapchainTextureFormat, window),
	};

  SDL_GPUVertexBufferDescription vertexBufferDescription = {
		.slot = 0,
		.pitch = sizeof(Vertex),
		.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
		.instance_step_rate = 0,
	};

	SDL_GPUVertexAttribute vertexAttributes[] = {
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

	SDL_GPUGraphicsPipeline *pipeline = $(renderDevice, createGraphicsPipeline, &(SDL_GPUGraphicsPipelineCreateInfo) {
		.vertex_shader = vertexShader,
		.fragment_shader = fragmentShader,
		.vertex_input_state = {
			.vertex_buffer_descriptions = &vertexBufferDescription,
			.num_vertex_buffers = 1,
			.vertex_attributes = vertexAttributes,
			.num_vertex_attributes = (Uint32) SDL_arraysize(vertexAttributes),
		},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.rasterizer_state = {
			.fill_mode = SDL_GPU_FILLMODE_FILL,
			.cull_mode = SDL_GPU_CULLMODE_BACK,
			.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
			.enable_depth_clip = true,
		},
		.multisample_state = {
			.sample_count = SDL_GPU_SAMPLECOUNT_1,
		},
		.depth_stencil_state = {
			.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL,
			.enable_depth_test = true,
			.enable_depth_write = true,
		},
		.target_info = {
			.color_target_descriptions = &colorTargetDescription,
			.num_color_targets = 1,
			.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
			.has_depth_stencil_target = true,
		},
	});

	$(renderDevice, releaseShader, vertexShader);
	$(renderDevice, releaseShader, fragmentShader);

  SDL_GPUBuffer *vertexBuffer = $(renderDevice, createBufferWithConstMem, SDL_GPU_BUFFERUSAGE_VERTEX, vertexes, sizeof(vertexes));

	bool running = true;
	while (running) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_EVENT_QUIT:
				case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
					running = false;
					break;
				default:
					break;
			}
		}

    drawScene(&(Scene) {
      .renderDevice = renderDevice,
      .framebuffer = framebuffer,
      .pipeline = pipeline,
      .vertexBuffer = vertexBuffer
    });
	}

	$(renderDevice, waitForIdle);
	$(renderDevice, releaseGraphicsPipeline, pipeline);
	$(renderDevice, releaseBuffer, vertexBuffer);
	release(framebuffer);
	release(renderDevice);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return EXIT_SUCCESS;
}
