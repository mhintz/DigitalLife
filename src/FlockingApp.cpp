#include "FlockingApp.hpp"

void FlockingApp::setup() {
	mFboSide = sqrt(mNumBirds);
	mNumBirds = mFboSide * mFboSide;

	auto fboTexFmt = gl::Texture2d::Format()
		.internalFormat(GL_RGB32F)
		.wrap(GL_REPEAT)
		.minFilter(GL_NEAREST)
		.magFilter(GL_NEAREST);
	auto fboDefaultFmt = gl::Fbo::Format().disableDepth().colorTexture(fboTexFmt);

	// Initialize the positions FBO
	Surface32f initialPos(mFboSide, mFboSide, true);
	auto posIter = initialPos.getIter();
	while (posIter.line()) {
		while (posIter.pixel()) {
			// random position on the screen
			vec2 pos(randFloat((float) mRenderFboSize.x), randFloat((float) mRenderFboSize.y));
			posIter.r() = pos.x;
			posIter.g() = pos.y;
			// Note: the z and w coordinates don't matter at the moment - they're never used by the shader
		}
	}
	auto posTex = gl::Texture2d::create(initialPos, fboTexFmt);
	auto posFboFmt = gl::Fbo::Format().disableDepth().attachment(GL_COLOR_ATTACHMENT0, posTex);

	mPositionsSource = gl::Fbo::create(mFboSide, mFboSide, posFboFmt);
	mPositionsDest = gl::Fbo::create(mFboSide, mFboSide, fboDefaultFmt);

	// Initialize the velocities FBO
	Surface32f initialVel(mFboSide, mFboSide, true);
	auto velIter = initialVel.getIter();
	while (velIter.line()) {
		while (velIter.pixel()) {
			// random velocity direction
			vec2 vel = randVec2();
			velIter.r() = vel.x;
			velIter.g() = vel.y;
			// Note: the z and w coordinates don't matter at the moment - they're never used by the shader
		}
	}
	auto velTex = gl::Texture2d::create(initialVel, fboTexFmt);
	auto velFboFmt = gl::Fbo::Format().disableDepth().attachment(GL_COLOR_ATTACHMENT0, velTex);

	mVelocitiesSource = gl::Fbo::create(mFboSide, mFboSide, velFboFmt);
	mVelocitiesDest = gl::Fbo::create(mFboSide, mFboSide, fboDefaultFmt);

	// Initialize the birds update routine
	mBirdPosUpdateProg = gl::GlslProg::create(ci::app::loadAsset("runBirds_v.glsl"), ci::app::loadAsset("runBirdsPosition_f.glsl"));
	mBirdPosUpdateProg->uniform("uGridSide", mFboSide);
	mBirdPosUpdateProg->uniform("uScreenWidth", mRenderFboSize.x);
	mBirdPosUpdateProg->uniform("uScreenHeight", mRenderFboSize.y);
	mBirdPosUpdateProg->uniform("uPositions", mPosTextureBind);
	mBirdPosUpdateProg->uniform("uVelocities", mVelTextureBind);

	mBirdVelUpdateProg = gl::GlslProg::create(ci::app::loadAsset("runBirds_v.glsl"), ci::app::loadAsset("runBirdsVelocity_f.glsl"));
	mBirdVelUpdateProg->uniform("uGridSide", mFboSide);
	mBirdVelUpdateProg->uniform("uPositions", mPosTextureBind);
	mBirdVelUpdateProg->uniform("uVelocities", mVelTextureBind);

	// Initialize the bird positions index VBO
	std::vector<vec2> posIndex(mNumBirds);
	for (int ypos = 0; ypos < mFboSide; ypos++) {
		for (int xpos = 0; xpos < mFboSide; xpos++) {
			posIndex[ypos * mFboSide + xpos] = (vec2(xpos, ypos) + 0.5f) / (float) mFboSide;
		}
	}
	auto birdsVbo = gl::Vbo::create(GL_ARRAY_BUFFER, posIndex, GL_STREAM_DRAW);
	auto birdsBufferLayout = geom::BufferLayout({ geom::AttribInfo(geom::CUSTOM_0, 2, 0, 0) });
	mBirdIndexMesh = gl::VboMesh::create(posIndex.size(), GL_POINTS, { { birdsBufferLayout, birdsVbo } });

	// Initialize the birds render routine
	mBirdRenderProg = gl::GlslProg::create(ci::app::loadAsset("renderBirds_v.glsl"), ci::app::loadAsset("renderBirds_f.glsl"), ci::app::loadAsset("renderBirds_g.glsl"));
	mBirdRenderProg->uniform("uBirdSize", mBirdSize);
	mBirdRenderProg->uniform("uBirdPositions", mPosTextureBind);
	mBirdRenderProg->uniform("uBirdVelocities", mVelTextureBind);
	mBirdRenderBatch = gl::Batch::create(mBirdIndexMesh, mBirdRenderProg, { {geom::CUSTOM_0, "birdIndex"} });

	mBirdRenderFbo = gl::Fbo::create(mRenderFboSize.x, mRenderFboSize.y);

	mCamera.lookAt(vec3(0, 0, 4), vec3(0), vec3(0, 1, 0));
	mCameraUi = CameraUi(& mCamera, ci::app::getWindow());

	mSphereMesh = gl::VboMesh::create(geom::Sphere().colors().center(vec3(0)).radius(1.0f).subdivisions(50));
	mSphereRenderProg = gl::GlslProg::create(ci::app::loadAsset("renderSphere_v.glsl"), ci::app::loadAsset("renderSphere_f.glsl"));
	mSphereRenderBatch = gl::Batch::create(mSphereMesh, mSphereRenderProg);
}

void FlockingApp::update()
{
	gl::ScopedViewport scpView(0, 0, mFboSide, mFboSide);
	gl::ScopedMatrices scpMat;
	gl::setMatricesWindow(mFboSide, mFboSide);

	// Update velocities first
	{
		gl::ScopedGlslProg scpShader(mBirdVelUpdateProg);
		gl::ScopedTextureBind scpPosTex(mPositionsSource->getColorTexture(), mPosTextureBind);
		gl::ScopedTextureBind scpVelTex(mVelocitiesSource->getColorTexture(), mVelTextureBind);

		gl::ScopedFramebuffer scpFbo(mVelocitiesDest);
		gl::drawSolidRect(Rectf(0, 0, mFboSide, mFboSide));
	}

	// Update positions second
	{
		gl::ScopedGlslProg scpShader(mBirdPosUpdateProg);
		gl::ScopedTextureBind scpPosTex(mPositionsSource->getColorTexture(), mPosTextureBind);
		gl::ScopedTextureBind scpVelTex(mVelocitiesSource->getColorTexture(), mVelTextureBind);

		gl::ScopedFramebuffer scpFbo(mPositionsDest);
		gl::drawSolidRect(Rectf(0, 0, mFboSide, mFboSide));
	}

	std::swap(mPositionsSource, mPositionsDest);
	std::swap(mVelocitiesSource, mVelocitiesDest);
}

void FlockingApp::draw()
{
	// Draw the birds into the FBO
	{
		gl::ScopedFramebuffer scpFbo(mBirdRenderFbo);

		gl::ScopedMatrices scpMat;
		gl::setMatricesWindow(mRenderFboSize.x, mRenderFboSize.y);
		gl::ScopedViewport scpView(0, 0, mRenderFboSize.x, mRenderFboSize.y);

		gl::clear(Color(0, 0, 0));

		gl::ScopedColor scpColor(Color(1, 1, 1));

		gl::ScopedTextureBind scpPosTex(mPositionsSource->getColorTexture(), mPosTextureBind);
		gl::ScopedTextureBind scpVelTex(mVelocitiesSource->getColorTexture(), mVelTextureBind);

		mBirdRenderBatch->draw();
	}

	// Draw the sphere
	{
		gl::ScopedDepth scpDepth(true);
		gl::ScopedFaceCulling scpFace(true, GL_BACK);

		gl::ScopedMatrices scpMat;
		gl::setMatrices(mCamera);

		gl::clear(Color(0, 0, 0));

		gl::ScopedTextureBind scpBirdTex(mBirdRenderFbo->getColorTexture());

		mSphereRenderBatch->draw();
	}
}
