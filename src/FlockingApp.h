#pragma once

#include <vector>

#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"
#include "cinder/params/Params.h"

#include "FboCubeMapLayered.h"

class FlockingApp {
public:
	FlockingApp() {};

	void setup();
	void update();
	ci::gl::TextureCubeMapRef draw();
	void disrupt(ci::vec3 dir);

	float mMinSpeed = 0.0030;
	float mMaxSpeed = 0.0070;

	float mMinForce = 0.0000;
	float mMaxForce = 0.0010;

	float mSeparationDist = 0.0450;
	float mSeparationMod = 0.2203;
	float mAlignDist = 0.0600;
	float mAlignMod = 0.0500;
	float mCohesionDist = 0.0530;
	float mCohesionMod = 0.0500;

	int mNumBirds = 56 * 56; // 3136
	// int mNumBirds = 64 * 64; // 4096
	// int mNumBirds = 8192; // 4096 * 2
	int mFboSide;

	uint8_t mPosTextureBind = 0;
	uint8_t mVelTextureBind = 1;
	uint8_t mCubeMapCameraMatrixBind = 2;

	ci::gl::FboRef mPositionsSource;
	ci::gl::FboRef mPositionsDest;
	ci::gl::FboRef mVelocitiesSource;
	ci::gl::FboRef mVelocitiesDest;

	ci::gl::GlslProgRef mBirdPosUpdateProg;
	ci::gl::GlslProgRef mBirdVelUpdateProg;

	ci::gl::GlslProgRef mBirdDisruptProg;

	ci::gl::VboMeshRef mBirdIndexMesh;
	ci::gl::GlslProgRef mBirdRenderProg;
	ci::gl::BatchRef mBirdRenderBatch;

	FboCubeMapLayeredRef mCubeMapCamera;
	ci::gl::UboRef mCubeMapCameraMatrixBuffer;

	ci::params::InterfaceGlRef mMenu;
};