#pragma once

#include <vector>

#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"
#include "cinder/Camera.h"
#include "cinder/CameraUi.h"

using namespace ci;

class FlockingApp {
public:
	FlockingApp() {};

	void setup();
	void update();
	void draw();

	float mBirdSize = 3.0f;
	float mMaxSpeed = 2.0f;
	float mMaxForce = 0.03f;

	ivec2 mRenderFboSize = ivec2(2000, 1000);

	int mNumBirds = 8192;

	int mFboSide;

	uint8_t mPosTextureBind = 0;
	uint8_t mVelTextureBind = 1;

	gl::FboRef mPositionsSource;
	gl::FboRef mPositionsDest;
	gl::FboRef mVelocitiesSource;
	gl::FboRef mVelocitiesDest;

	gl::GlslProgRef mBirdPosUpdateProg;
	gl::GlslProgRef mBirdVelUpdateProg;

	gl::VboMeshRef mBirdIndexMesh;
	gl::GlslProgRef mBirdRenderProg;
	gl::BatchRef mBirdRenderBatch;

	gl::FboRef mBirdRenderFbo;

	CameraPersp mCamera;
	CameraUi mCameraUi;

	gl::VboMeshRef mSphereMesh;
	gl::GlslProgRef mSphereRenderProg;
	gl::BatchRef mSphereRenderBatch;
};