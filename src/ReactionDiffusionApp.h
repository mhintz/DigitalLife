#pragma once

#include <memory>
#include <vector>

#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"
#include "cinder/Camera.h"
#include "cinder/CameraUi.h"

#include "glm/gtc/constants.hpp"

#include "FboCubeMapLayered.h"
#include "MeshHelpers.h"

using namespace ci;

class ReactionDiffusionApp {
public:
	ReactionDiffusionApp() {}

	void setup();
	void update();
	gl::TextureCubeMapRef draw();
	void disrupt(ci::vec3 dir);

	void setupCircleRD(float rad);

	float const mTypeAlpha_waves[2] = { 0.010, 0.047 };
	float const mTypeEpsilon_microbes[2] = { 0.018, 0.055 };
	int const mUpdatesPerFrame = 10;
	int const mCubeMapSide = 512;
	int const mRDReadFboBinding = 0;
	int const mRDRenderTextureBinding = 1;

	gl::TextureCubeMapRef mSourceTex;
	GLuint mSourceFbo;
	gl::TextureCubeMapRef mDestTex;
	GLuint mDestFbo;
	gl::GlslProgRef mRDProgram;
	gl::VboMeshRef mPointMesh;

	gl::GlslProgRef mRenderRDProgram;
	gl::VboMeshRef mCubeMapFacesMesh;
	gl::BatchRef mRenderCubeMapBatch;
	FboCubeMapLayeredRef mCubeMapCamera;
};