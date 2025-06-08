#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>

#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"

#include <array>
#include <vector>

constexpr uint32_t cWidth = 1920;
constexpr uint32_t cHeight = 1080;

enum class BlockType : uint32_t
{
	Stone = 0,
	Dirt = 1,
	Air = 2
};

struct AppData
{
	SDL_Window* mWindow;
	std::vector<BlockType> mBlocks;

	// Rendering
	SDL_GPUDevice* mDevice;
	SDL_GPUGraphicsPipeline* mDrawPipeline;
	SDL_GPUComputePipeline* mFillTexturePipeline;
	SDL_GPUSampler* mSampler;
	SDL_GPUBuffer* mVertexBuffer;
	SDL_GPUTexture* mTexture;

	SDL_GPUBuffer* mBlockBuffer;
	SDL_GPUTransferBuffer* mBlockTransferBuffer;
};

template <typename tType>
inline void ZeroStruct(tType& aValue)
{
	SDL_memset(&aValue, 0, sizeof(tType));
}

typedef struct PositionTextureVertex
{
	float x, y, z;
	float u, v;
} PositionTextureVertex;

SDL_GPUShader* LoadShader(
	SDL_GPUDevice* device,
	const char* shaderFilename,
	Uint32 samplerCount,
	Uint32 uniformBufferCount,
	Uint32 storageBufferCount,
	Uint32 storageTextureCount
) {
	// Auto-detect the shader stage from the file name for convenience
	SDL_GPUShaderStage stage;
	if (SDL_strstr(shaderFilename, ".vert"))
	{
		stage = SDL_GPU_SHADERSTAGE_VERTEX;
	}
	else if (SDL_strstr(shaderFilename, ".frag"))
	{
		stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	}
	else
	{
		SDL_Log("Invalid shader stage!");
		return NULL;
	}

	char fullPath[256];
	SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(device);
	SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
	const char* entrypoint;

	if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV) {
		SDL_snprintf(fullPath, sizeof(fullPath), "Shaders/Compiled/SPIRV/%s.spv", shaderFilename);
		format = SDL_GPU_SHADERFORMAT_SPIRV;
		entrypoint = "main";
	}
	else if (backendFormats & SDL_GPU_SHADERFORMAT_MSL) {
		SDL_snprintf(fullPath, sizeof(fullPath), "Shaders/Compiled/MSL/%s.msl", shaderFilename);
		format = SDL_GPU_SHADERFORMAT_MSL;
		entrypoint = "main0";
	}
	else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL) {
		SDL_snprintf(fullPath, sizeof(fullPath), "Shaders/Compiled/DXIL/%s.dxil", shaderFilename);
		format = SDL_GPU_SHADERFORMAT_DXIL;
		entrypoint = "main";
	}
	else {
		SDL_Log("%s", "Unrecognized backend shader format!");
		return NULL;
	}

	size_t codeSize;
	void* code = SDL_LoadFile(fullPath, &codeSize);
	if (code == NULL)
	{
		SDL_Log("Failed to load shader from disk! %s", fullPath);
		return NULL;
	}

	SDL_GPUShaderCreateInfo shaderInfo;
	ZeroStruct(shaderInfo);
	shaderInfo.code = (const Uint8*)code;
	shaderInfo.code_size = codeSize;
	shaderInfo.entrypoint = entrypoint;
	shaderInfo.format = format;
	shaderInfo.stage = stage;
	shaderInfo.num_samplers = samplerCount;
	shaderInfo.num_uniform_buffers = uniformBufferCount;
	shaderInfo.num_storage_buffers = storageBufferCount;
	shaderInfo.num_storage_textures = storageTextureCount;

	SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shaderInfo);
	if (shader == NULL)
	{
		SDL_Log("Failed to create shader!");
		SDL_free(code);
		return NULL;
	}

	SDL_free(code);
	return shader;
}

SDL_GPUComputePipeline* CreateComputePipelineFromShader(
	SDL_GPUDevice* device,
	const char* shaderFilename,
	SDL_GPUComputePipelineCreateInfo* createInfo
) {
	char fullPath[256];
	SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(device);
	SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
	const char* entrypoint;

	if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV) {
		SDL_snprintf(fullPath, sizeof(fullPath), "Shaders/Compiled/SPIRV/%s.spv", shaderFilename);
		format = SDL_GPU_SHADERFORMAT_SPIRV;
		entrypoint = "main";
	}
	else if (backendFormats & SDL_GPU_SHADERFORMAT_MSL) {
		SDL_snprintf(fullPath, sizeof(fullPath), "Shaders/Compiled/MSL/%s.msl", shaderFilename);
		format = SDL_GPU_SHADERFORMAT_MSL;
		entrypoint = "main0";
	}
	else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL) {
		SDL_snprintf(fullPath, sizeof(fullPath), "Shaders/Compiled/DXIL/%s.dxil", shaderFilename);
		format = SDL_GPU_SHADERFORMAT_DXIL;
		entrypoint = "main";
	}
	else {
		SDL_Log("%s", "Unrecognized backend shader format!");
		return NULL;
	}

	size_t codeSize;
	void* code = SDL_LoadFile(fullPath, &codeSize);
	if (code == NULL)
	{
		SDL_Log("Failed to load compute shader from disk! %s", fullPath);
		return NULL;
	}

	// Make a copy of the create data, then overwrite the parts we need
	SDL_GPUComputePipelineCreateInfo newCreateInfo = *createInfo;
	newCreateInfo.code = (const Uint8*)code;
	newCreateInfo.code_size = codeSize;
	newCreateInfo.entrypoint = entrypoint;
	newCreateInfo.format = format;

	SDL_GPUComputePipeline* pipeline = SDL_CreateGPUComputePipeline(device, &newCreateInfo);
	if (pipeline == NULL)
	{
		SDL_Log("Failed to create compute pipeline!");
		SDL_free(code);
		return NULL;
	}

	SDL_free(code);
	return pipeline;
}

SDL_AppResult InitSDLStuff(AppData* app)
{
	app->mWindow = SDL_CreateWindow("PixelSandCompute", cWidth, cHeight, 0);
	app->mDevice = SDL_CreateGPUDevice(
		SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL,
		true,
		NULL);

	SDL_ClaimWindowForGPUDevice(app->mDevice, app->mWindow);

	SDL_GPUShader* vertexShader = LoadShader(app->mDevice, "TexturedQuad.vert", 0, 0, 0, 0);
	if (vertexShader == NULL)
	{
		SDL_Log("Failed to create vertex shader!");
		return SDL_APP_FAILURE;
	}

	SDL_GPUShader* fragmentShader = LoadShader(app->mDevice, "TexturedQuad.frag", 1, 0, 0, 0);
	if (fragmentShader == NULL)
	{
		SDL_Log("Failed to create fragment shader!");
		return SDL_APP_FAILURE;
	}

	SDL_GPUComputePipelineCreateInfo pipelineCreateInfo;
	ZeroStruct(pipelineCreateInfo);
	pipelineCreateInfo.num_readwrite_storage_textures = 1;
	pipelineCreateInfo.num_readonly_storage_buffers = 1;
	pipelineCreateInfo.threadcount_x = 8;
	pipelineCreateInfo.threadcount_y = 8;
	pipelineCreateInfo.threadcount_z = 1;

	app->mFillTexturePipeline = CreateComputePipelineFromShader(
		app->mDevice,
		"FillTexture.comp",
		&pipelineCreateInfo
	);
	SDL_GPUColorTargetDescription colorTargetDescription;
	ZeroStruct(colorTargetDescription);
	colorTargetDescription.format = SDL_GetGPUSwapchainTextureFormat(app->mDevice, app->mWindow);

	SDL_GPUVertexBufferDescription vertexBufferDescription;
	ZeroStruct(vertexBufferDescription);
	vertexBufferDescription.slot = 0;
	vertexBufferDescription.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
	vertexBufferDescription.instance_step_rate = 0;
	vertexBufferDescription.pitch = sizeof(PositionTextureVertex);

	std::array<SDL_GPUVertexAttribute, 2> vertexAttributes;
	ZeroStruct(vertexAttributes[0]);
	vertexAttributes[0].buffer_slot = 0;
	vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
	vertexAttributes[0].location = 0;
	vertexAttributes[0].offset = 0;

	ZeroStruct(vertexAttributes[1]);
	vertexAttributes[1].buffer_slot = 0;
	vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
	vertexAttributes[1].location = 1;
	vertexAttributes[1].offset = sizeof(float) * 3;

	SDL_GPUVertexInputState vertexInputState;
	ZeroStruct(vertexInputState);
	vertexInputState.num_vertex_buffers = 1;
	vertexInputState.vertex_buffer_descriptions = &vertexBufferDescription;
	vertexInputState.num_vertex_attributes = 2,
		vertexInputState.vertex_attributes = vertexAttributes.data();

	SDL_GPUGraphicsPipelineCreateInfo graphicePipelineCreateInfo;
	ZeroStruct(graphicePipelineCreateInfo);
	graphicePipelineCreateInfo.target_info.num_color_targets = 1,
		graphicePipelineCreateInfo.target_info.color_target_descriptions = &colorTargetDescription;
	graphicePipelineCreateInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	graphicePipelineCreateInfo.vertex_input_state = vertexInputState;
	graphicePipelineCreateInfo.vertex_shader = vertexShader;
	graphicePipelineCreateInfo.fragment_shader = fragmentShader;

	app->mDrawPipeline = SDL_CreateGPUGraphicsPipeline(app->mDevice, &graphicePipelineCreateInfo);

	SDL_ReleaseGPUShader(app->mDevice, vertexShader);
	SDL_ReleaseGPUShader(app->mDevice, fragmentShader);

	int w, h;
	SDL_GetWindowSizeInPixels(app->mWindow, &w, &h);

	SDL_GPUTextureCreateInfo textureCreateInfo;
	ZeroStruct(textureCreateInfo);
	textureCreateInfo.type = SDL_GPU_TEXTURETYPE_2D;
	textureCreateInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
	textureCreateInfo.width = w;
	textureCreateInfo.height = h;
	textureCreateInfo.layer_count_or_depth = 1;
	textureCreateInfo.num_levels = 1;
	textureCreateInfo.usage = SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_TEXTUREUSAGE_SAMPLER;

	app->mTexture = SDL_CreateGPUTexture(app->mDevice, &textureCreateInfo);

	SDL_GPUSamplerCreateInfo samplerCreateInfo;
	ZeroStruct(samplerCreateInfo);
	samplerCreateInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
	samplerCreateInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;

	app->mSampler = SDL_CreateGPUSampler(app->mDevice, &samplerCreateInfo);

	SDL_GPUBufferCreateInfo vertexBufferCreateInfo;
	ZeroStruct(vertexBufferCreateInfo);
	vertexBufferCreateInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
	vertexBufferCreateInfo.size = sizeof(PositionTextureVertex) * 6;

	app->mVertexBuffer = SDL_CreateGPUBuffer(app->mDevice, &vertexBufferCreateInfo);

	SDL_GPUTransferBufferCreateInfo transferBufferCreateInfo;
	ZeroStruct(transferBufferCreateInfo);
	transferBufferCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	transferBufferCreateInfo.size = sizeof(PositionTextureVertex) * 6;

	SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(app->mDevice, &transferBufferCreateInfo);

	PositionTextureVertex* transferData = (PositionTextureVertex*)SDL_MapGPUTransferBuffer(
		app->mDevice,
		transferBuffer,
		false
	);

	transferData[0] = PositionTextureVertex{ -1, -1, 0, 0, 0 };
	transferData[1] = PositionTextureVertex{ 1, -1, 0, 1, 0 };
	transferData[2] = PositionTextureVertex{ 1,  1, 0, 1, 1 };
	transferData[3] = PositionTextureVertex{ -1, -1, 0, 0, 0 };
	transferData[4] = PositionTextureVertex{ 1,  1, 0, 1, 1 };
	transferData[5] = PositionTextureVertex{ -1,  1, 0, 0, 1 };

	SDL_UnmapGPUTransferBuffer(app->mDevice, transferBuffer);

	SDL_GPUCommandBuffer* cmdBuf = SDL_AcquireGPUCommandBuffer(app->mDevice);
	SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmdBuf);

	SDL_GPUTransferBufferLocation transferBufferLocation;
	ZeroStruct(transferBufferLocation);
	transferBufferLocation.transfer_buffer = transferBuffer;
	transferBufferLocation.offset = 0;

	SDL_GPUBufferRegion bufferRegion;
	ZeroStruct(bufferRegion);
	bufferRegion.buffer = app->mVertexBuffer;
	bufferRegion.offset = 0;
	bufferRegion.size = sizeof(PositionTextureVertex) * 6;

	SDL_UploadToGPUBuffer(
		copyPass,
		&transferBufferLocation,
		&bufferRegion,
		false
	);

	SDL_EndGPUCopyPass(copyPass);

	SDL_SubmitGPUCommandBuffer(cmdBuf);

	SDL_ReleaseGPUTransferBuffer(app->mDevice, transferBuffer);

	
	// Transfer Buffer for getting the block data to the compute shader
	SDL_GPUTransferBufferCreateInfo blockTransferBufferCreateInfo;
	ZeroStruct(blockTransferBufferCreateInfo);
	blockTransferBufferCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	blockTransferBufferCreateInfo.size = sizeof(BlockType) * cWidth * cHeight;

	app->mBlockTransferBuffer = SDL_CreateGPUTransferBuffer(app->mDevice, &blockTransferBufferCreateInfo);

	// Buffer for the block data on the GPU side
	SDL_GPUBufferCreateInfo blockBufferCreateInfo;
	ZeroStruct(blockBufferCreateInfo);
	blockBufferCreateInfo.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ;
	blockBufferCreateInfo.size = sizeof(BlockType) * cWidth * cHeight;

	app->mBlockBuffer = SDL_CreateGPUBuffer(app->mDevice, &blockBufferCreateInfo);

	return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
	AppData* app = new AppData;
	*appstate = (void*)app;

	// SDL/Renderer
	if (InitSDLStuff(app) != SDL_APP_CONTINUE) {
		return SDL_APP_FAILURE;
	}

	// ImGui
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	ImGui_ImplSDL3_InitForSDLGPU(app->mWindow);
	ImGui_ImplSDLGPU3_InitInfo init_info = {};
	init_info.Device = app->mDevice;
	init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(app->mDevice, app->mWindow);
	init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
	ImGui_ImplSDLGPU3_Init(&init_info);


	// Game stuff
	app->mBlocks.resize(cWidth * cHeight, BlockType::Air);

	for (size_t i = (cWidth * (cHeight * .5f)); i < (cWidth * (cHeight * .75f)); ++i) {
		app->mBlocks[i] = BlockType::Dirt;
	}

	for (size_t i = (cWidth * (cHeight * .75f)); i < app->mBlocks.size(); ++i) {
		app->mBlocks[i] = BlockType::Stone;
	}
	
	return SDL_APP_CONTINUE;
}


SDL_AppResult Render(AppData* app)
{
	SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(app->mDevice);
	if (commandBuffer == NULL)
	{
		SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUTexture* swapchainTexture;
	if (!SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, app->mWindow, &swapchainTexture, NULL, NULL)) {
		SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	if (swapchainTexture != NULL)
	{
		//////////////////////
		// ImGui Upload
        // Rendering
		ImGui::Render();
		ImDrawData* draw_data = ImGui::GetDrawData();

		Imgui_ImplSDLGPU3_PrepareDrawData(draw_data, commandBuffer);

		//////////////////////
		// Copy Pass
		BlockType* transferData = (BlockType*)SDL_MapGPUTransferBuffer(
			app->mDevice,
			app->mBlockTransferBuffer,
			false
		);

		std::copy(app->mBlocks.begin(), app->mBlocks.end(), transferData);

		SDL_UnmapGPUTransferBuffer(app->mDevice, app->mBlockTransferBuffer);

		SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);

		SDL_GPUTransferBufferLocation transferBufferLocation;
		ZeroStruct(transferBufferLocation);
		transferBufferLocation.transfer_buffer = app->mBlockTransferBuffer;
		transferBufferLocation.offset = 0;

		SDL_GPUBufferRegion bufferRegion;
		ZeroStruct(bufferRegion);
		bufferRegion.buffer = app->mBlockBuffer;
		bufferRegion.offset = 0;
		bufferRegion.size = sizeof(BlockType) * cWidth * cHeight;

		SDL_UploadToGPUBuffer(
			copyPass,
			&transferBufferLocation,
			&bufferRegion,
			false
		);

		SDL_EndGPUCopyPass(copyPass);


		//////////////////////
		// Compute Pass

		// Bind Texture we're writing the block data to via Compute
		SDL_GPUStorageTextureReadWriteBinding storageTextureReadWriteBinding;
		ZeroStruct(storageTextureReadWriteBinding);
		storageTextureReadWriteBinding.texture = app->mTexture;

		SDL_GPUComputePass* computePass = SDL_BeginGPUComputePass(
			commandBuffer,
			&storageTextureReadWriteBinding,
			1,
			nullptr,
			0
		);

		int w, h;
		SDL_GetWindowSizeInPixels(app->mWindow, &w, &h);

		SDL_BindGPUComputePipeline(computePass, app->mFillTexturePipeline);

		// Bind Block Data
		SDL_BindGPUComputeStorageBuffers(
			computePass,
			0,
			&app->mBlockBuffer,
			1
		);


		SDL_DispatchGPUCompute(computePass, w / 8, h / 8, 1);
		SDL_EndGPUComputePass(computePass);

		//////////////////////
		// Render Pass
		SDL_GPUColorTargetInfo colorTargetInfo;
		ZeroStruct(colorTargetInfo);
		colorTargetInfo.texture = swapchainTexture;
		colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
		colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
		colorTargetInfo.clear_color.a = 1;
		colorTargetInfo.cycle = false;

		SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(
			commandBuffer,
			&colorTargetInfo,
			1,
			NULL
		);

		SDL_GPUBufferBinding bufferBinding;
		ZeroStruct(bufferBinding);
		bufferBinding.buffer = app->mVertexBuffer;
		bufferBinding.offset = 0;

		SDL_GPUTextureSamplerBinding textureSamplerBinding;
		ZeroStruct(textureSamplerBinding);
		textureSamplerBinding.texture = app->mTexture;
		textureSamplerBinding.sampler = app->mSampler;

		SDL_BindGPUGraphicsPipeline(renderPass, app->mDrawPipeline);
		SDL_BindGPUVertexBuffers(renderPass, 0, &bufferBinding, 1);
		SDL_BindGPUFragmentSamplers(renderPass, 0, &textureSamplerBinding, 1);
		SDL_DrawGPUPrimitives(renderPass, 6, 1, 0, 0);


		// Render ImGui
		ImGui_ImplSDLGPU3_RenderDrawData(draw_data, commandBuffer, renderPass);

		SDL_EndGPURenderPass(renderPass);
	}

	SDL_SubmitGPUCommandBuffer(commandBuffer);


	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
	AppData* app = (AppData*)appstate;

	// ImGui
	ImGui_ImplSDLGPU3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	{
		static float f = 0.0f;
		static int counter = 0;

		ImGuiIO& io = ImGui::GetIO();;

		ImGui::Begin("ImGui Stuff");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
		ImGui::End();
	}

	// Game Update
	float x, y;
	SDL_MouseButtonFlags flags = SDL_GetMouseState(&x, &y);

	if (flags & SDL_BUTTON_LMASK) {
		app->mBlocks[(y * cWidth) + x] = BlockType::Stone;
	}

	if (Render(app) != SDL_APP_CONTINUE) {
		return SDL_APP_FAILURE;
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
	AppData* app = (AppData*)appstate;

	ImGui_ImplSDL3_ProcessEvent(event);

	if (event->common.type == SDL_EVENT_QUIT) {
		return SDL_APP_SUCCESS;
	}

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
	AppData* app = (AppData*)appstate;
	delete app;
}