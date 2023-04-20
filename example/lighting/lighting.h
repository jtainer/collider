// 
// Scene lighting handler
//
// 2022, Jonathan Tainer
//

#ifndef LIGHTING_H
#define LIGHTING_H

void InitLighting();

void EndLighting();

void SetLightPosition(Vector3);

void SetLightTarget(Vector3);

void LightingAddModel(Model*);

void BeginDepthMode();

void EndDepthMode();

void BeginViewMode(Camera);

void EndViewMode();

void DrawDepthBuffer();

#endif
