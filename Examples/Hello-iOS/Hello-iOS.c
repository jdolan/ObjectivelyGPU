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

#define SDL_MAIN_USE_CALLBACKS

#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>

#include <Objectively.h>
#include <ObjectivelyGPU.h>

static const char *vertex_shader_msl =
"#include <metal_stdlib>\n"
"using namespace metal;\n"
"\n"
"struct type_UBO {\n"
"    mat4 ModelViewProj;\n"
"};\n"
"\n"
"struct main0_out {\n"
"    vec4 out_color [[user(locn0)]];\n"
"    vec4 gl_Position [[position]];\n"
"};\n"
"\n"
"struct main0_in {\n"
"    vec3 in_position [[attribute(0)]];\n"
"    vec3 in_color    [[attribute(1)]];\n"
"};\n"
"\n"
"vertex main0_out main0(main0_in in [[stage_in]], constant type_UBO& UBO [[buffer(0)]]) {\n"
"    main0_out out = {};\n"
"    out.out_color  = vec4(in.in_color, 1.0);\n"
"    out.gl_Position = UBO.ModelViewProj * vec4(in.in_position, 1.0);\n"
"    return out;\n"
"}\n";

static const char *fragment_shader_msl =
"#include <metal_stdlib>\n"
"using namespace metal;\n"
"\n"
"struct main0_in {\n"
"    vec4 in_color [[user(locn0)]];\n"
"};\n"
"\n"
"fragment vec4 main0(main0_in in [[stage_in]]) {\n"
"    return in.in_color;\n"
"}\n";

typedef struct {
	float x, y, z;
	float r, g, b;
} Vertex;

static const Vertex vertex_data[] = {
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

static void rotate_matrix(float angle, float x, float y, float z, float *r) {
	float radians = angle * SDL_PI_F / 180.0f;
	float c = SDL_cosf(radians), s = SDL_sinf(radians), c1 = 1.0f - c;
	float length = SDL_sqrtf(x*x + y*y + z*z);
	float u[3] = { x/length, y/length, z/length };
	for (int i = 0; i < 16; i++) r[i] = 0.0f;
	r[15] = 1.0f;
	for (int i = 0; i < 3; i++) {
		r[i*4 + (i+1)%3] = u[(i+2)%3] * s;
		r[i*4 + (i+2)%3] = -u[(i+1)%3] * s;
	}
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			r[i*4+j] += c1 * u[i] * u[j] + (i == j ? c : 0.0f);
}

static void perspective_matrix(float fovy, float aspect, float znear, float zfar, float *r) {
	float f = 1.0f / SDL_tanf((fovy / 180.0f) * SDL_PI_F * 0.5f);
	for (int i = 0; i < 16; i++) r[i] = 0.0f;
	r[0] = f / aspect; r[5] = f;
	r[10] = (znear + zfar) / (znear - zfar); r[11] = -1.0f;
	r[14] = (2.0f * znear * zfar) / (znear - zfar);
}

static void multiply_matrix(const float *lhs, const float *rhs, float *r) {
	float tmp[16];
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++) {
			tmp[j*4+i] = 0.0f;
			for (int k = 0; k < 4; k++)
				tmp[j*4+i] += lhs[k*4+i] * rhs[j*4+k];
		}
	for (int i = 0; i < 16; i++) r[i] = tmp[i];
}

typedef struct {
	SDL_Window *window;
	RenderDevice *renderDevice;
	SDL_GPUBuffer *vertexBuffer;
	SDL_GPUTexture *depthTexture;
	SDL_GPUGraphicsPipeline *pipeline;
	SDL_Size depthSize;
	float angleX, angleY;
	Uint64 lastTicks;
} App;

static App app;

static SDL_GPUShader *create_shader(SDL_GPUShaderStage stage, Uint32 numUniformBuffers) {
	const char *source = stage == SDL_GPU_SHADERSTAGE_VERTEX ? vertex_shader_msl : fragment_shader_msl;
	return $(app.renderDevice, createShader, &(SDL_GPUShaderCreateInfo) {
		.code_size = SDL_strlen(source),
		.code = (const Uint8 *) source,
		.entrypoint = "main0",
		.format = SDL_GPU_SHADERFORMAT_MSL,
		.stage = stage,
		.num_uniform_buffers = numUniformBuffers,
	});
}

static SDL_GPUTexture *create_depth_texture(SDL_Size size) {
	if (size.w <= 0 || size.h <= 0) {
		return NULL;
	}
	return $(app.renderDevice, createTexture, &(SDL_GPUTextureCreateInfo) {
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

SDL_AppResult SDL_AppInit(void **unused, int argc, char *argv[]) {
	(void) unused; (void) argc; (void) argv;

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_memset(&app, 0, sizeof(app));

	app.window = SDL_CreateWindow("ObjectivelyGPU Hello",
		0, 0, SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_FULLSCREEN);
	if (!app.window) {
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateWindow: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	app.renderDevice = $(alloc(RenderDevice), initWithWindow, app.window);

	app.vertexBuffer = $(app.renderDevice, createBuffer, &(SDL_GPUBufferCreateInfo) {
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = sizeof(vertex_data),
	});

	SDL_GPUTransferBuffer *transferBuffer = $(app.renderDevice, createTransferBuffer, &(SDL_GPUTransferBufferCreateInfo) {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = sizeof(vertex_data),
	});
	void *mapped = $(app.renderDevice, mapTransferBuffer, transferBuffer, false);
	SDL_memcpy(mapped, vertex_data, sizeof(vertex_data));
	$(app.renderDevice, unmapTransferBuffer, transferBuffer);

	CommandBuffer *cmd = $(app.renderDevice, acquireCommandBuffer);
	CopyPass *copyPass = $(cmd, beginCopyPass);
	$(copyPass, uploadBuffer,
	  &(SDL_GPUTransferBufferLocation){ .transfer_buffer = transferBuffer },
	  &(SDL_GPUBufferRegion){ .buffer = app.vertexBuffer, .size = sizeof(vertex_data) },
	  false);
	release(copyPass);
	$(cmd, submit);
	release(cmd);
	$(app.renderDevice, releaseTransferBuffer, transferBuffer);

	int w = 0, h = 0;
	SDL_GetWindowSizeInPixels(app.window, &w, &h);
	app.depthSize = MakeSize(w, h);
	app.depthTexture = create_depth_texture(app.depthSize);

	SDL_GPUShader *vertexShader   = create_shader(SDL_GPU_SHADERSTAGE_VERTEX, 1);
	SDL_GPUShader *fragmentShader = create_shader(SDL_GPU_SHADERSTAGE_FRAGMENT, 0);

	SDL_GPUColorTargetDescription colorTarget = {
		.format = $(app.renderDevice, getSwapchainTextureFormat, app.window),
	};
	SDL_GPUVertexBufferDescription vbDesc = {
		.slot = 0,
		.pitch = sizeof(Vertex),
		.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
	};
	SDL_GPUVertexAttribute vertexAttribs[] = {
		{ .location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = 0  },
		{ .location = 1, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = 12 },
	};

	app.pipeline = $(app.renderDevice, createGraphicsPipeline, &(SDL_GPUGraphicsPipelineCreateInfo) {
		.vertex_shader = vertexShader,
		.fragment_shader = fragmentShader,
		.vertex_input_state = {
			.vertex_buffer_descriptions = &vbDesc,
			.num_vertex_buffers = 1,
			.vertex_attributes = vertexAttribs,
			.num_vertex_attributes = (Uint32) SDL_arraysize(vertexAttribs),
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
			.color_target_descriptions = &colorTarget,
			.num_color_targets = 1,
			.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
			.has_depth_stencil_target = true,
		},
	});

	$(app.renderDevice, releaseShader, vertexShader);
	$(app.renderDevice, releaseShader, fragmentShader);

	app.lastTicks = SDL_GetTicks();
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *unused) {
	(void) unused;

	Uint64 ticks = SDL_GetTicks();
	float dt = (float)(ticks - app.lastTicks) / 1000.0f;
	app.lastTicks = ticks;

	app.angleX += dt * 30.0f;
	app.angleY += dt * 60.0f;
	while (app.angleX >= 360.0f) app.angleX -= 360.0f;
	while (app.angleY >= 360.0f) app.angleY -= 360.0f;

	CommandBuffer *cmd = $(app.renderDevice, acquireCommandBuffer);
	SwapchainTexture swapchain = { 0 };
	if (!$(app.renderDevice, acquireSwapchainTexture, cmd, &swapchain)) {
		$(cmd, cancel);
		release(cmd);
		return SDL_APP_CONTINUE;
	}

	if (app.depthTexture == NULL ||
	    swapchain.size.w != app.depthSize.w || swapchain.size.h != app.depthSize.h) {
		app.depthSize = swapchain.size;
		$(app.renderDevice, releaseTexture, app.depthTexture);
		app.depthTexture = create_depth_texture(app.depthSize);
	}

	float matModelView[16], matRotate[16], matProj[16], matFinal[16];
	rotate_matrix(app.angleX, 1, 0, 0, matModelView);
	rotate_matrix(app.angleY, 0, 1, 0, matRotate);
	multiply_matrix(matRotate, matModelView, matModelView);
	matModelView[14] -= 2.5f;
	perspective_matrix(45.0f, (float) swapchain.size.w / (float) swapchain.size.h,
	                   0.01f, 100.0f, matProj);
	multiply_matrix(matProj, matModelView, matFinal);

	SDL_GPUColorTargetInfo colorInfo = {
		.texture = swapchain.texture,
		.clear_color = { 0.1f, 0.1f, 0.2f, 1.0f },
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_STORE,
	};
	SDL_GPUDepthStencilTargetInfo depthInfo = {
		.texture = app.depthTexture,
		.clear_depth = 1.0f,
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_DONT_CARE,
		.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
		.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
	};

	RenderPass *renderPass = $(cmd, beginRenderPass, &colorInfo, 1, &depthInfo);
	$(renderPass, bindPipeline, app.pipeline);
	$(renderPass, bindVertexBuffers, 0, &(SDL_GPUBufferBinding){ .buffer = app.vertexBuffer }, 1);
	$(cmd, pushVertexUniformData, 0, matFinal, sizeof(matFinal));
	$(renderPass, drawPrimitives, (Uint32) SDL_arraysize(vertex_data), 1, 0, 0);
	release(renderPass);

	$(cmd, submit);
	release(cmd);

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *unused, SDL_Event *event) {
	(void) unused;
	if (event->type == SDL_EVENT_QUIT) {
		return SDL_APP_SUCCESS;
	}
	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *unused, SDL_AppResult result) {
	(void) unused; (void) result;

	if (app.renderDevice) {
		$(app.renderDevice, waitForIdle);
	}
	if (app.pipeline) {
		$(app.renderDevice, releaseGraphicsPipeline, app.pipeline);
	}
	if (app.vertexBuffer) {
		$(app.renderDevice, releaseBuffer, app.vertexBuffer);
	}
	if (app.depthTexture) {
		$(app.renderDevice, releaseTexture, app.depthTexture);
	}
	if (app.renderDevice) {
		release(app.renderDevice);
	}
	if (app.window) {
		SDL_DestroyWindow(app.window);
	}
	SDL_Quit();
}
