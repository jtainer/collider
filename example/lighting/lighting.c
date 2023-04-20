// 
// Scene lighting handler
//
// 2022, Jonathan Tainer
//

#include <raylib.h>
#include <rlgl.h>
#include <raymath.h>
#include <stdlib.h>
#include "lighting.h"

#define SHADOWMAP_VS_FILE "lighting/shaders/depthMap.vs"
#define SHADOWMAP_FS_FILE "lighting/shaders/depthMap.fs"
#define MODEL_VS_FILE "lighting/shaders/model.vs"
#define MODEL_FS_FILE "lighting/shaders/model.fs"
#define DEPTH_FS_FILE "lighting/shaders/depth.fs"

#define SHADOW_CAMERA_POSITION	(Vector3) { 10.f, 10.f, 10.f }
#define SHADOW_CAMERA_TARGET	(Vector3) { 0.f, 0.f, 0.f }
#define SHADOW_CAMERA_UP	(Vector3) { 0.f, 1.f, 0.f }
#define SHADOW_CAMERA_FOVY	45.f

#define SHADOW_BUFFER_WIDTH	16384

#define MAX_MODELS 8

// Player camera
//extern Camera camera;

// Point light camera
static Camera shadowCamera = { 0 };

// Render target for shadow camera
RenderTexture shadowBuffer = { 0 };

static Shader shadowMapShader = { 0 };
static Shader modelShader = { 0 };
static Shader depthShader = { 0 };

// Various shader locs
static int modelShaderLightViewLoc = 0;
static int modelShaderLightProjLoc = 0;
static int modelShaderLightDirLoc = 0;
static int shadowShaderLightViewLoc = 0;
static int shadowShaderLightProjLoc = 0;

// Buffer to keep track of models used in shadow calculations
static Model* modelArr[MAX_MODELS];
static int modelCount = 0;

static RenderTexture2D LoadRenderTextureWithDepthTexture(int width, int height);

static void LoadShaders() {
	shadowMapShader = LoadShader(SHADOWMAP_VS_FILE, SHADOWMAP_FS_FILE);
	modelShader = LoadShader(MODEL_VS_FILE, MODEL_FS_FILE);
	depthShader = LoadShader(NULL, DEPTH_FS_FILE);

	modelShaderLightViewLoc = GetShaderLocation(modelShader, "matLightView");
	modelShaderLightProjLoc = GetShaderLocation(modelShader, "matLightProjection");
	modelShaderLightDirLoc = GetShaderLocation(modelShader, "lightDir");
	shadowShaderLightViewLoc = GetShaderLocation(shadowMapShader, "matLightView");
	shadowShaderLightProjLoc = GetShaderLocation(shadowMapShader, "matLightProjection");
}

static void UnloadShaders() {
	UnloadShader(shadowMapShader);
	UnloadShader(modelShader);
	UnloadShader(depthShader);
}

void InitLighting() {
	LoadShaders();
	shadowBuffer = LoadRenderTextureWithDepthTexture(SHADOW_BUFFER_WIDTH, SHADOW_BUFFER_WIDTH);
	
	shadowCamera.position = SHADOW_CAMERA_POSITION;
	shadowCamera.target = SHADOW_CAMERA_TARGET;
	shadowCamera.up = SHADOW_CAMERA_UP;
	shadowCamera.fovy = SHADOW_CAMERA_FOVY;
	shadowCamera.projection = CAMERA_ORTHOGRAPHIC;
}

void EndLighting() {
	UnloadShaders();
	UnloadRenderTexture(shadowBuffer);
}

void SetLightPosition(Vector3 position) {
	shadowCamera.position = position;
}

void SetLightTarget(Vector3 target) {
	shadowCamera.target = target;
}

void LightingAddModel(Model* modelPtr) {
	if (modelCount < MAX_MODELS) {
		
		// Apply depth texture to model metalness loc
		for (int i = 0; i < modelPtr->materialCount; i++) {
			modelPtr->materials[i].maps[MATERIAL_MAP_METALNESS].texture = shadowBuffer.depth;
		}

		// Add model pointer to array
		modelArr[modelCount] = modelPtr;
		modelCount++;
	}
	
	
}

void BeginDepthMode() {
	// Calculate shader variables
	float aspect = (float) shadowBuffer.depth.width / (float) shadowBuffer.depth.height;
	double top = RL_CULL_DISTANCE_NEAR * tan(shadowCamera.fovy * 0.5 * DEG2RAD);
	double right = top * aspect;
	Matrix lightView = GetCameraMatrix(shadowCamera);
	Matrix lightProj = MatrixPerspective(shadowCamera.fovy, aspect, RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
	Vector3 lightDir = Vector3Normalize(Vector3Subtract(shadowCamera.target, shadowCamera.position));

	// Update shader variables
	SetShaderValueMatrix(modelShader, modelShaderLightViewLoc, lightView);
	SetShaderValueMatrix(modelShader, modelShaderLightProjLoc, lightProj);
	SetShaderValue(modelShader, modelShaderLightDirLoc, &lightDir, SHADER_UNIFORM_VEC3);
	SetShaderValueMatrix(shadowMapShader, shadowShaderLightViewLoc, lightView);
	SetShaderValueMatrix(shadowMapShader, shadowShaderLightProjLoc, lightProj);

	// Set shadow map shader
	for (int i = 0; i < modelCount; i++) {
		modelArr[i]->materials[0].shader = shadowMapShader;
	}

	BeginTextureMode(shadowBuffer);
	ClearBackground(BLANK);
	BeginMode3D(shadowCamera);
	
	rlSetCullFace(RL_CULL_FACE_FRONT);

}

void EndDepthMode() {
	rlSetCullFace(RL_CULL_FACE_BACK);

	EndMode3D();
	EndTextureMode();
}

void BeginViewMode(Camera camera) {
	// Set model shader
	for (int i = 0; i < modelCount; i++) {
		modelArr[i]->materials[0].shader = modelShader;
	}
	
	BeginMode3D(camera);
}

void EndViewMode() {
	EndMode3D();
}

void DrawDepthBuffer(Rectangle rect) {
	BeginShaderMode(depthShader);
	
	DrawTexturePro(shadowBuffer.depth, (Rectangle) { 0.f, 0.f, (float) shadowBuffer.texture.width, (float) shadowBuffer.texture.height }, rect, (Vector2) { 0.f, 0.f }, 0.f, WHITE);
	
	EndShaderMode();
}

RenderTexture2D LoadRenderTextureWithDepthTexture(int width, int height) {
	RenderTexture2D target = {0};

	target.id = rlLoadFramebuffer(width, height);   // Load an empty framebuffer

	if (target.id > 0) {
		rlEnableFramebuffer(target.id);

		// Create color texture (default to RGBA)
		target.texture.id = rlLoadTexture(NULL, width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
		target.texture.width = width;
		target.texture.height = height;
		target.texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
		target.texture.mipmaps = 1;

		// Create depth texture
		target.depth.id = rlLoadTextureDepth(width, height, false);
		target.depth.width = width;
		target.depth.height = height;
		target.depth.format = 19;
		target.depth.mipmaps = 1;

		// Attach color texture and depth texture to FBO
		rlFramebufferAttach(target.id, target.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
		rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);

		// Check if fbo is complete with attachments (valid)
		if (rlFramebufferComplete(target.id)) TRACELOG(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully", target.id);

		rlDisableFramebuffer();
	} 
	
	else TRACELOG(LOG_WARNING, "FBO: Framebuffer object can not be created");

	return target;
}
