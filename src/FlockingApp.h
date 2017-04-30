#pragma once

#include <vector>

#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"

#include "FboCubeMapLayered.h"

class FlockingApp {
public:
	FlockingApp() {};

	void setup();
	void update();
	ci::gl::TextureCubeMapRef draw();

	float mMaxSpeed = 2.0f;
	float mMaxForce = 0.03f;

	ci::ivec2 mRenderFboSize = ci::ivec2(4000, 2000);

//	int mNumBirds = 8192;
	int mNumBirds = 64 * 64; // 4096

	int mFboSide;

	uint8_t mPosTextureBind = 0;
	uint8_t mVelTextureBind = 1;
	uint8_t mCubeMapCameraMatrixBind = 2;
	uint8_t mRenderedBirdTextureBind = 3;

	ci::gl::FboRef mPositionsSource;
	ci::gl::FboRef mPositionsDest;
	ci::gl::FboRef mVelocitiesSource;
	ci::gl::FboRef mVelocitiesDest;

	ci::gl::GlslProgRef mBirdPosUpdateProg;
	ci::gl::GlslProgRef mBirdVelUpdateProg;

	ci::gl::VboMeshRef mBirdIndexMesh;
	ci::gl::GlslProgRef mBirdRenderProg;
	ci::gl::BatchRef mBirdRenderBatch;

	ci::gl::FboRef mBirdRenderFbo;

	ci::gl::VboMeshRef mSphereMesh;

	FboCubeMapLayeredRef mCubeMapCamera;
	ci::gl::UboRef mCubeMapCameraMatrixBuffer;
	ci::gl::GlslProgRef mTrianglesCubeMapCameraProgram;
};