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

#include <stdlib.h>

#include <SDL3/SDL.h>

#include <Objectively.h>
#include <ObjectivelyGPU.h>

typedef struct {
	float x, y, z;
	float r, g, b;
} VertexData;

static const VertexData vertex_data[] = {
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

static void log_sdl_error(const char *what) {
	SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s: %s", what, SDL_GetError());
}

static SDL_GPUTexture *create_depth_texture(const RenderDevice *renderDevice, SDL_Size size) {
	if (size.w <= 0 || size.h <= 0) {
		return NULL;
	}

	return $(renderDevice, createTexture, &(SDL_GPUTextureCreateInfo) {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
		.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
		.width = (Uint32) size.w,
		.height = (Uint32) size.h,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.sample_count = SDL_GPU_SAMPLECOUNT_1,
	}, NULL);
}

static void upload_vertex_buffer(const RenderDevice *renderDevice, SDL_GPUBuffer *buffer) {
	SDL_GPUTransferBuffer *transferBuffer = $(renderDevice, createTransferBuffer, &(SDL_GPUTransferBufferCreateInfo) {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = sizeof(vertex_data),
	});

	void *mapped = $(renderDevice, mapTransferBuffer, transferBuffer, false);
	SDL_memcpy(mapped, vertex_data, sizeof(vertex_data));
	$(renderDevice, unmapTransferBuffer, transferBuffer);

	CommandBuffer *cmd = $(renderDevice, acquireCommandBuffer);
	CopyPass *copyPass = $(cmd, beginCopyPass);

	$(copyPass, uploadBuffer,
	  &(SDL_GPUTransferBufferLocation) {
		.transfer_buffer = transferBuffer,
		.offset = 0,
	  },
	  &(SDL_GPUBufferRegion) {
		.buffer = buffer,
		.offset = 0,
		.size = sizeof(vertex_data),
	  },
	  false);

	release(copyPass);
	$(cmd, submit);
	release(cmd);

	$(renderDevice, releaseTransferBuffer, transferBuffer);
}


int main(int argc, char **argv) {
	(void) argc;
	(void) argv;

	int status = EXIT_FAILURE;
	SDL_Window *window = NULL;
	RenderDevice *renderDevice = NULL;
	SDL_GPUBuffer *vertexBuffer = NULL;
	SDL_GPUTexture *depthTexture = NULL;
	SDL_GPUShader *vertexShader = NULL;
	SDL_GPUShader *fragmentShader = NULL;
	SDL_GPUGraphicsPipeline *pipeline = NULL;
	SDL_Size depthSize = MakeSize(0, 0);

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		log_sdl_error("SDL_Init");
		return status;
	}

	const char *basePath = SDL_GetBasePath();
	if (basePath) {
		char shaderDir[512];
		SDL_snprintf(shaderDir, sizeof(shaderDir), "%sShaders", basePath);
		$$(Resource, addResourcePath, shaderDir);
	}

	window = SDL_CreateWindow("ObjectivelyGPU Hello", 800, 600, SDL_WINDOW_HIGH_PIXEL_DENSITY);
	if (!window) {
		log_sdl_error("SDL_CreateWindow");
		goto cleanup;
	}

	renderDevice = $(alloc(RenderDevice), initWithWindow, window);

	vertexBuffer = $(renderDevice, createBuffer, &(SDL_GPUBufferCreateInfo) {
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = sizeof(vertex_data),
	});
	upload_vertex_buffer(renderDevice, vertexBuffer);

	int drawableWidth = 0;
	int drawableHeight = 0;
	if (!SDL_GetWindowSizeInPixels(window, &drawableWidth, &drawableHeight)) {
		log_sdl_error("SDL_GetWindowSizeInPixels");
		goto cleanup;
	}

	depthSize = MakeSize(drawableWidth, drawableHeight);
	depthTexture = create_depth_texture(renderDevice, depthSize);

	vertexShader = $(renderDevice, loadShader, "Hello.vert", &(SDL_GPUShaderCreateInfo) {
		.entrypoint = "vs_main",
		.stage = SDL_GPU_SHADERSTAGE_VERTEX,
		.num_uniform_buffers = 1,
	});
	fragmentShader = $(renderDevice, loadShader, "Hello.frag", &(SDL_GPUShaderCreateInfo) {
		.entrypoint = "fs_main",
		.stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
	});

	SDL_GPUColorTargetDescription colorTargetDescription = {
		.format = $(renderDevice, getSwapchainTextureFormat, window),
	};
	SDL_GPUVertexBufferDescription vertexBufferDescription = {
		.slot = 0,
		.pitch = sizeof(VertexData),
		.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
		.instance_step_rate = 0,
	};
	SDL_GPUVertexAttribute vertexAttributes[] = {
		{
			.location = 0,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			.offset = 0,
		},
		{
			.location = 1,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			.offset = 12,
		},
	};

	pipeline = $(renderDevice, createGraphicsPipeline, &(SDL_GPUGraphicsPipelineCreateInfo) {
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
	vertexShader = NULL;
	$(renderDevice, releaseShader, fragmentShader);
	fragmentShader = NULL;

	bool running = true;
	float angleX = 0.0f;
	float angleY = 0.0f;
	Uint64 lastTicks = SDL_GetTicks();

	while (running) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_EVENT_QUIT:
				case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
					running = false;
					break;
				case SDL_EVENT_KEY_DOWN:
					if (event.key.key == SDLK_ESCAPE) {
						running = false;
					}
					break;
				default:
					break;
			}
		}

		Uint64 ticks = SDL_GetTicks();
		float dt = (float) (ticks - lastTicks) / 1000.0f;
		lastTicks = ticks;

		angleX += dt * 30.0f;
		angleY += dt * 60.0f;

		while (angleX >= 360.0f) {
			angleX -= 360.0f;
		}
		while (angleY >= 360.0f) {
			angleY -= 360.0f;
		}

		CommandBuffer *cmd = $(renderDevice, acquireCommandBuffer);
		Swapchain swapchain = { 0 };

		if (!$(renderDevice, acquireSwapchainTexture, cmd, &swapchain)) {
			$(cmd, cancel);
			release(cmd);
			continue;
		}

		if (depthTexture == NULL || swapchain.size.w != depthSize.w || swapchain.size.h != depthSize.h) {
			depthSize = swapchain.size;
			$(renderDevice, releaseTexture, depthTexture);
			depthTexture = create_depth_texture(renderDevice, depthSize);
		}

		float4x4 mv = float4x4_rotation(angleX, float3_new(1.f, 0.f, 0.f));
		mv = float4x4_mul(float4x4_rotation(angleY, float3_new(0.f, 1.f, 0.f)), mv);
		mv = float4x4_mul(float4x4_translation(float3_new(0.f, 0.f, -2.5f)), mv);
		const float4x4 proj = float4x4_perspective(45.f, (float) swapchain.size.w / (float) swapchain.size.h, 0.01f, 100.f);
		const float4x4 matrixFinal = float4x4_mul(proj, mv);

		SDL_GPUColorTargetInfo colorTarget = {
			.texture = swapchain.texture,
			.clear_color = { 0.1f, 0.1f, 0.2f, 1.0f },
			.load_op = SDL_GPU_LOADOP_CLEAR,
			.store_op = SDL_GPU_STOREOP_STORE,
		};
		SDL_GPUDepthStencilTargetInfo depthTarget = {
			.texture = depthTexture,
			.clear_depth = 1.0f,
			.load_op = SDL_GPU_LOADOP_CLEAR,
			.store_op = SDL_GPU_STOREOP_DONT_CARE,
			.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
			.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
		};

		RenderPass *renderPass = $(cmd, beginRenderPass, &colorTarget, 1, &depthTarget);
		$(renderPass, bindPipeline, pipeline);
		$(renderPass, bindVertexBuffers, 0, &(SDL_GPUBufferBinding) {
			.buffer = vertexBuffer,
			.offset = 0,
		}, 1);
		$(cmd, pushVertexUniformData, 0, matrixFinal.f, sizeof(matrixFinal));
		$(renderPass, drawPrimitives, (Uint32) SDL_arraysize(vertex_data), 1, 0, 0);
		release(renderPass);

		$(cmd, submit);
		release(cmd);
	}

	status = EXIT_SUCCESS;

cleanup:
	if (renderDevice) {
		$(renderDevice, waitForIdle);
	}

	if (pipeline) {
		$(renderDevice, releaseGraphicsPipeline, pipeline);
	}
	if (vertexShader) {
		$(renderDevice, releaseShader, vertexShader);
	}
	if (fragmentShader) {
		$(renderDevice, releaseShader, fragmentShader);
	}
	if (vertexBuffer) {
		$(renderDevice, releaseBuffer, vertexBuffer);
	}
	if (depthTexture) {
		$(renderDevice, releaseTexture, depthTexture);
	}
	if (renderDevice) {
		release(renderDevice);
	}
	if (window) {
		SDL_DestroyWindow(window);
	}

	SDL_Quit();
	return status;
}
