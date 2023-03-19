// 
// Oriented bounding box collisions
//
// 2023, Jonathan Tainer
//

#include "collider.h"
#include <raymath.h>
#include <stdio.h>

//*******************************************************************
// Various collider transformations. Physics engine should call these
// to apply movement to each collider.
//*******************************************************************

// Applies the matrices in the struct to the local verts to calculate
// vertex positions in global space
static void UpdateColliderGlobalVerts(Collider* col) {
	Matrix matTemp = MatrixMultiply(col->matRotate, col->matTranslate);
	for (int i = 0; i < COLLIDER_VERTEX_COUNT; i++) {
		col->vertGlobal[i] = Vector3Transform(col->vertLocal[i], matTemp);
	}
}

// All colliders are axis-aligned bounding boxes in local space
Collider CreateCollider(Vector3 min, Vector3 max) {
	Collider c = { 0 };
	c.vertLocal[0] = (Vector3) { min.x, min.y, min.z };
	c.vertLocal[1] = (Vector3) { min.x, min.y, max.z };
	c.vertLocal[2] = (Vector3) { min.x, max.y, min.z };
	c.vertLocal[3] = (Vector3) { min.x, max.y, max.z };
	c.vertLocal[4] = (Vector3) { max.x, min.y, min.z };
	c.vertLocal[5] = (Vector3) { max.x, min.y, max.z };
	c.vertLocal[6] = (Vector3) { max.x, max.y, min.z };
	c.vertLocal[7] = (Vector3) { max.x, max.y, max.z };

	c.matRotate = MatrixIdentity();
	c.matTranslate = MatrixIdentity();
	UpdateColliderGlobalVerts(&c);
	return c;
}

// Overwrites collider rotation matrix
// Updates global vertex positions
void SetColliderRotation(Collider* col, Vector3 axis, float ang) {
	col->matRotate = MatrixRotate(axis, ang);
	UpdateColliderGlobalVerts(col);
}

// Multiplies current collider rotation matrix by new one
// Updates global vertex positions
void AddColliderRotation(Collider* col, Vector3 axis, float ang) {
	Matrix matTemp = MatrixRotate(axis, ang);
	col->matRotate = MatrixMultiply(col->matRotate, matTemp);
	UpdateColliderGlobalVerts(col);
}

// Overwrites collider translation matrix
// Updates global vertex positions
void SetColliderTranslation(Collider* col, Vector3 pos) {
	col->matTranslate = MatrixTranslate(pos.x, pos.y, pos.z);
	UpdateColliderGlobalVerts(col);
}

// Adds new translation matrix to current translation matrix
// Updates global vertex positions
void AddColliderTranslation(Collider* col, Vector3 pos) {
	Matrix matTemp = MatrixTranslate(pos.x, pos.y, pos.z);
	col->matTranslate = MatrixMultiply(col->matTranslate, matTemp);
	UpdateColliderGlobalVerts(col);
}

// Returns overall transform, first rotation then translation
Matrix GetColliderTransform(Collider* col) {
	return MatrixMultiply(col->matRotate, col->matTranslate);
}

//*******************************************************************
//		COLLISION DETECTION STUFF BEGINS HERE
//*******************************************************************

// First check along each face normal
// Then check along the cross products of the pairs of the face normals
//
// vec must point to a buffer large enough to store 15 Vector3
static void GetCollisionVectors(Collider* a, Collider* b, Vector3* vec) {
	Vector3 x = { 1.f, 0.f, 0.f };
	Vector3 y = { 0.f, 1.f, 0.f };
	Vector3 z = { 0.f, 0.f, 1.f };

	vec[0] = Vector3Transform(x, a->matRotate);
	vec[1] = Vector3Transform(y, a->matRotate);
	vec[2] = Vector3Transform(z, a->matRotate);

	vec[3] = Vector3Transform(x, b->matRotate);
	vec[4] = Vector3Transform(y, b->matRotate);
	vec[5] = Vector3Transform(z, b->matRotate);

	int i = 6;
	for (int j = 0; j < 3; j++) {
		for (int k = 3; k < 6; k++) {
			if (Vector3Equals(vec[j], vec[k])) vec[i] = x;
			else vec[i] = Vector3Normalize(Vector3CrossProduct(vec[j], vec[k]));
			i++;
		}
	}
}


// Iterate through all verts, project on test vector, find min and max values
// Returns min and max in x and y members, respectively
Vector2 GetColliderProjectionBounds(Collider* col, Vector3 vec) {
	Vector2 bounds = { 0 };
	float proj = Vector3DotProduct(col->vertGlobal[0], vec);
	bounds.x = bounds.y = proj;
	for (int i = 1; i < COLLIDER_VERTEX_COUNT; i++) {
		proj = Vector3DotProduct(col->vertGlobal[i], vec);
		bounds.x = fmin(bounds.x, proj);
		bounds.y = fmax(bounds.y, proj);
	}
	return bounds;
}

static bool BoundsOverlap(Vector2 a, Vector2 b) {
	
	// If the min of one projection is greater than the max of the
	// other projection then the projections do not overlap
	if (a.x > b.y) return false;
	if (b.x > a.y) return false;
	return true;
}

// Calculate the amount of overlap along the axis being checked
static float GetOverlap(Vector2 a, Vector2 b) {
	if (a.x > b.y) return 0.f;
	if (b.x > a.y) return 0.f;
	if (a.x > b.x) return b.y - a.x;
	else return b.x - a.y;
}

// Returns true if two colliders overlap, and false otherwise
bool TestColliderPair(Collider* a, Collider* b) {
	Vector3 testVec[15];

	// Calculate all test vectors
	// First 6 are the surface normals of each collider
	// Last 9 are the cross products of each pair of surface normals
	GetCollisionVectors(a, b, testVec);
	for (int i = 0; i < 15; i++) {

		// Project each collider onto the test vector
		// Get min and max points of the projection
		Vector2 apro, bpro;
		apro = GetColliderProjectionBounds(a, testVec[i]);
		bpro = GetColliderProjectionBounds(b, testVec[i]);

		if (!BoundsOverlap(apro, bpro)) return false;
	}
	return true;
}

// Returns a displacement vector that, when added to the position of
// collider 'a', will resolve the collision. It examines the
// correction needed for each test vector (normals and their cross
// products) and returns the smallest one. If the returned vector is
// zero then the colliders do not overlap.
Vector3 GetCollisionCorrection(Collider* a, Collider* b) {
	float overlapMin = 100.f;
	Vector3 overlapDir = { 0 };
	
	Vector3 testVec[15];
	GetCollisionVectors(a, b, testVec);
	for (int i = 0; i < 15; i++) {
		Vector2 apro, bpro;
		apro = GetColliderProjectionBounds(a, testVec[i]);
		bpro = GetColliderProjectionBounds(b, testVec[i]);

		float overlap = GetOverlap(apro, bpro);
		if (overlap == 0.f) return Vector3Zero();
		if (fabs(overlap) < fabs(overlapMin)) {
			overlapMin = overlap;
			overlapDir = testVec[i];
		}
	}

	return Vector3Scale(overlapDir, overlapMin);
}
