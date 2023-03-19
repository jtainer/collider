// 
// Oriented bounding box collisions
//
// 2023, Jonathan Tainer
//

#ifndef COLLISION_H
#define COLLISION_H

#include <raylib.h>

#define COLLIDER_VERTEX_COUNT 8
#define COLLIDER_NORMAL_COUNT 3

typedef struct Collider {
	// Vertex positions in local (model) space
	Vector3 vertLocal[COLLIDER_VERTEX_COUNT];

	// Vertex positions in global (world) space
	Vector3 vertGlobal[COLLIDER_VERTEX_COUNT];

	// Rotation about origin in local space
	Matrix matRotate;

	// Translation applied after rotation
	Matrix matTranslate;
} Collider;

// Calculate verts, use identity matrix by default
Collider CreateCollider(Vector3 min, Vector3 max);

// Rotate object starting from origin
void SetColliderRotation(Collider* col, Vector3 axis, float ang);

// Rotate objects starting from current position
void AddColliderRotation(Collider* col, Vector3 axis, float ang);

// Translate object starting from origin
void SetColliderTranslation(Collider* col, Vector3 pos);

// Translate object starting from current position
void AddColliderTranslation(Collider* col, Vector3 pos);

Matrix GetColliderTransform(Collider* col);

// Use separating axis theorem to detect overlap
bool TestColliderPair(Collider* a, Collider* b);

// Find translation needed to resolve a collision
Vector3 GetCollisionCorrection(Collider* a, Collider* b);

#endif
