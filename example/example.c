// 
// testing oriented bounding box collisions
//
// 2023, Jonathan Tainer
//

#include <raylib.h>
#include <raymath.h>
#include <collider.h>
#include <lighting.h>

typedef struct RigidBody {
	Model model;
	Collider collider;
} RigidBody;

int main() {

	// Window setup
	const int windowWidth = 1920;
	const int windowHeight = 1080;
	const char* windowTitle = "collider demo";
	SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN | FLAG_VSYNC_HINT);
	InitWindow(windowWidth, windowHeight, windowTitle);
	SetTargetFPS(120);
	DisableCursor();

	// Setup 3rd person camera
	Camera camera = { 0 };
	camera.projection = CAMERA_PERSPECTIVE;
	camera.fovy = 45.f;
	camera.position = (Vector3) { -5.f, 2.f, 5.f };
	camera.target = (Vector3) { 0.f, 1.f, 0.f };
	camera.up = (Vector3) { 0.f, 1.f, 0.f };

	// Initialize all the colliders and models
	RigidBody plane, player, block, ramp;

	// Create plane
	Vector3 dim = { 100.f, 1.f, 100.f };
	Vector3 min = { -dim.x/2, -dim.y/2, -dim.z/2 };
	Vector3 max = { dim.x/2, dim.y/2, dim.z/2 };
	Vector3 pos = { 0.f, 0.f, 0.f };
	Vector3 axis = { 0.f, 1.f, 0.f };
	float ang = 0.f;
	plane.collider = CreateCollider(min, max);
	plane.model = LoadModelFromMesh(GenMeshCube(dim.x, dim.y, dim.z));
	SetColliderRotation(&plane.collider, axis, ang);
	SetColliderTranslation(&plane.collider, pos);
	plane.model.transform = GetColliderTransform(&plane.collider);

	// Create player
	dim = (Vector3) { 1.f, 1.f, 1.f };
	min = (Vector3) { -dim.x/2, -dim.y/2, -dim.z/2 };
	max = (Vector3) { dim.x/2, dim.y/2, dim.z/2 };
	pos = (Vector3) { 0.f, 1.f, 0.f };
	axis = (Vector3) { 0.f, 1.f, 0.f };
	ang = 0.f;
	player.collider = CreateCollider(min, max);
	player.model = LoadModelFromMesh(GenMeshCube(dim.x, dim.y, dim.z));
	SetColliderRotation(&player.collider, axis, ang);
	SetColliderTranslation(&player.collider, pos);
	player.model.transform = GetColliderTransform(&player.collider);
	Vector3 playerVel = Vector3Zero();

	// Create block
	dim = (Vector3) { 5.f, 5.f, 5.f };
	min = (Vector3) { -dim.x/2, -dim.y/2, -dim.z/2 };
	max = (Vector3) { dim.x/2, dim.y/2, dim.z/2 };
	pos = (Vector3) { 10.f, 1.f, 10.f };
	axis = (Vector3) { 0.f, 1.f, 0.f };
	ang = 0.f;
	block.collider = CreateCollider(min, max);
	block.model = LoadModelFromMesh(GenMeshCube(dim.x, dim.y, dim.z));
	SetColliderRotation(&block.collider, axis, ang);
	SetColliderTranslation(&block.collider, pos);
	block.model.transform = GetColliderTransform(&block.collider);

	// Create ramp
	dim = (Vector3) { 5.f, 1.f, 20.f };
	min = (Vector3) { -dim.x/2, -dim.y/2, -dim.z/2 };
	max = (Vector3) { dim.x/2, dim.y/2, dim.z/2 };
	pos = (Vector3) { -10.f, 0.f, 0.f };
	axis = (Vector3) { 1.f, 0.f, 0.f };
	ang = M_PI/3;
	ramp.collider = CreateCollider(min, max);
	ramp.model = LoadModelFromMesh(GenMeshCube(dim.x, dim.y, dim.z));
	SetColliderRotation(&ramp.collider, axis, ang);
	SetColliderTranslation(&ramp.collider, pos);
	ramp.model.transform = GetColliderTransform(&ramp.collider);

	// Setup lighting handler
	InitLighting();
	SetLightPosition((Vector3) { 30.f, 40.f, 20.f });
	SetLightTarget((Vector3) { 0.f, 0.f, 0.f });
	LightingAddModel(&player.model);
	LightingAddModel(&plane.model);
	LightingAddModel(&block.model);
	LightingAddModel(&ramp.model);

	while (!WindowShouldClose()) {

		// Using built-in camera controller because I am lazy
		UpdateCamera(&camera, CAMERA_THIRD_PERSON);

		// Translate and rotate player collider to follow camera target
		SetColliderTranslation(&player.collider, camera.target);
		ang = atan2f(camera.position.x - camera.target.x, camera.position.z - camera.target.z);
		axis = (Vector3) { 0.f, 1.f, 0.f };
		SetColliderRotation(&player.collider, axis, ang);
		
		// Apply gravity:
		const float speed = 5.f;
		float dt = Clamp(GetFrameTime(), 0.f, 1.f/30.f);
		playerVel.x = 0.f;
		playerVel.z = 0.f;
		playerVel.y = IsKeyPressed(KEY_SPACE) ? 20.f: playerVel.y - 0.8f;
		playerVel.y = fmax(-20.f, playerVel.y);
		Vector3 playerDisp = Vector3Scale(playerVel, dt);
		AddColliderTranslation(&player.collider, playerDisp);
		

		// Calculate the correction needed to resolve collisions between the player and all other colliders
		// Then add the correction to the position of the player
		// The order in which the corrections are applied can change the results
		Vector3 corr;
		corr = GetCollisionCorrection(&player.collider, &block.collider);
		AddColliderTranslation(&player.collider, corr);
		corr = GetCollisionCorrection(&player.collider, &ramp.collider);
		AddColliderTranslation(&player.collider, corr);
		corr = GetCollisionCorrection(&player.collider, &plane.collider);
		AddColliderTranslation(&player.collider, corr);

		// Move camera to follow player collider
		// (only translation, rotation and distance stay the same here)
		Vector3 playerPos = Vector3Transform(Vector3Zero(), player.collider.matTranslate);
		Vector3 cameraOffset = Vector3Subtract(camera.position, camera.target);
		camera.target = playerPos;
		camera.position = Vector3Add(camera.target, cameraOffset);
		player.model.transform = GetColliderTransform(&player.collider);

		// Render lighting depth map
		BeginDepthMode();
		DrawModel(plane.model, Vector3Zero(), 1.f, RED);
		DrawModel(player.model, Vector3Zero(), 1.f, BLUE);
		DrawModel(block.model, Vector3Zero(), 1.f, GREEN);
		DrawModel(ramp.model, Vector3Zero(), 1.f, YELLOW);
		EndDepthMode();

		// Render scene
		BeginDrawing();
		ClearBackground(BLACK);
		BeginViewMode(camera);
		DrawModel(plane.model, Vector3Zero(), 1.f, RED);
		DrawModel(player.model, Vector3Zero(), 1.f, BLUE);
		DrawModel(block.model, Vector3Zero(), 1.f, GREEN);
		DrawModel(ramp.model, Vector3Zero(), 1.f, YELLOW);
		DrawModelWires(plane.model, Vector3Zero(), 1.f, WHITE);
		DrawModelWires(player.model, Vector3Zero(), 1.f, WHITE);
		DrawModelWires(block.model, Vector3Zero(), 1.f, WHITE);
		DrawModelWires(ramp.model, Vector3Zero(), 1.f, WHITE);
		EndViewMode();
		DrawFPS(10, 10);
		EndDrawing();
	}

	UnloadModel(plane.model);
	UnloadModel(player.model);
	UnloadModel(block.model);
	UnloadModel(ramp.model);
	EndLighting();
	CloseWindow();
	return 0;
}
