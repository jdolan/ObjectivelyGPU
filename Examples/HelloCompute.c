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

#define NUM_PARTICLES 256u

typedef struct {
	float x, y;
} Particle;

static void log_sdl_error(const char *what) {
	SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s: %s", what, SDL_GetError());
}

int main(int argc, char **argv) {
	(void) argc;
	(void) argv;

	int status = EXIT_FAILURE;
	SDL_Window *window = NULL;
	RenderDevice *renderDevice = NULL;
	SDL_GPUBuffer *particleBuffer = NULL;
	SDL_GPUShader *vertexShader = NULL;
	SDL_GPUShader *fragmentShader = NULL;
	SDL_GPUGraphicsPipeline *graphicsPipeline = NULL;
	SDL_GPUComputePipeline *computePipeline = NULL;

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

	window = SDL_CreateWindow("ObjectivelyGPU HelloCompute", 800, 600, SDL_WINDOW_HIGH_PIXEL_DENSITY);
	if (!window) {
		log_sdl_error("SDL_CreateWindow");
		goto cleanup;
	}

	renderDevice = $(alloc(RenderDevice), initWithWindow, window);

	particleBuffer = $(renderDevice, createBuffer, &(SDL_GPUBufferCreateInfo) {
		.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
		.size = NUM_PARTICLES * sizeof(Particle),
	});

	computePipeline = $(renderDevice, loadComputePipeline, "HelloCompute.comp", &(SDL_GPUComputePipelineCreateInfo) {
		.entrypoint = "cs_main",
		.num_readwrite_storage_buffers = 1,
		.num_uniform_buffers = 1,
		.threadcount_x = NUM_PARTICLES,
		.threadcount_y = 1,
		.threadcount_z = 1,
	});

	vertexShader = $(renderDevice, loadShader, "HelloCompute.vert", &(SDL_GPUShaderCreateInfo) {
		.entrypoint = "vs_main",
		.stage = SDL_GPU_SHADERSTAGE_VERTEX,
		.num_storage_buffers = 1,
	});
	fragmentShader = $(renderDevice, loadShader, "HelloCompute.frag", &(SDL_GPUShaderCreateInfo) {
		.entrypoint = "fs_main",
		.stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
	});

	SDL_GPUColorTargetDescription colorTargetDescription = {
		.format = $(renderDevice, getSwapchainTextureFormat, window),
	};

	graphicsPipeline = $(renderDevice, createGraphicsPipeline, &(SDL_GPUGraphicsPipelineCreateInfo) {
		.vertex_shader = vertexShader,
		.fragment_shader = fragmentShader,
		.primitive_type = SDL_GPU_PRIMITIVETYPE_POINTLIST,
		.rasterizer_state = {
			.fill_mode = SDL_GPU_FILLMODE_FILL,
			.enable_depth_clip = true,
		},
		.multisample_state = {
			.sample_count = SDL_GPU_SAMPLECOUNT_1,
		},
		.target_info = {
			.color_target_descriptions = &colorTargetDescription,
			.num_color_targets = 1,
		},
	});

	$(renderDevice, releaseShader, vertexShader);
	vertexShader = NULL;
	$(renderDevice, releaseShader, fragmentShader);
	fragmentShader = NULL;

	bool running = true;
	Uint64 startTicks = SDL_GetTicks();

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

		CommandBuffer *cmd = $(renderDevice, acquireCommandBuffer);
		Swapchain swapchain = { 0 };

		if (!$(renderDevice, acquireSwapchainTexture, cmd, &swapchain)) {
			$(cmd, cancel);
			release(cmd);
			continue;
		}

		float time = (float) (SDL_GetTicks() - startTicks) / 1000.0f;
		$(cmd, pushComputeUniformData, 0, &time, sizeof(time));

		SDL_GPUStorageBufferReadWriteBinding storageBuffer = {
			.buffer = particleBuffer,
			.cycle = false,
		};

		ComputePass *computePass = $(cmd, beginComputePass, NULL, 0, &storageBuffer, 1);
		$(computePass, bindPipeline, computePipeline);
		$(computePass, dispatchCompute, 1, 1, 1);
		release(computePass);

		SDL_GPUColorTargetInfo colorTarget = {
			.texture = swapchain.texture,
			.clear_color = { 0.05f, 0.05f, 0.1f, 1.0f },
			.load_op = SDL_GPU_LOADOP_CLEAR,
			.store_op = SDL_GPU_STOREOP_STORE,
		};

		RenderPass *renderPass = $(cmd, beginRenderPass, &colorTarget, 1, NULL);
		SDL_GPUBuffer *vertexStorageBuffers[] = { particleBuffer };
		$(renderPass, bindPipeline, graphicsPipeline);
		$(renderPass, bindVertexStorageBuffers, 0, vertexStorageBuffers, 1);
		$(renderPass, drawPrimitives, NUM_PARTICLES, 1, 0, 0);
		release(renderPass);

		$(cmd, submit);
		release(cmd);
	}

	status = EXIT_SUCCESS;

cleanup:
	if (renderDevice) {
		$(renderDevice, waitForIdle);
	}

	if (graphicsPipeline) {
		$(renderDevice, releaseGraphicsPipeline, graphicsPipeline);
	}
	if (computePipeline) {
		$(renderDevice, releaseComputePipeline, computePipeline);
	}
	if (vertexShader) {
		$(renderDevice, releaseShader, vertexShader);
	}
	if (fragmentShader) {
		$(renderDevice, releaseShader, fragmentShader);
	}
	if (particleBuffer) {
		$(renderDevice, releaseBuffer, particleBuffer);
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
