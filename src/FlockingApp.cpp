#include "FlockingApp.h"

using namespace ci;

extern uint32_t OUTPUT_CUBE_MAP_SIDE;

void FlockingApp::setup() {
	mFboSide = sqrt(mNumBirds);
	mNumBirds = mFboSide * mFboSide;

	auto fboTexFmt = gl::Texture2d::Format()
		.internalFormat(GL_RGBA32F) // Turns out the A is important
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
			vec3 pos = randVec3();
			posIter.r() = pos.x;
			posIter.g() = pos.y;
			posIter.b() = pos.z;
			posIter.a() = randFloat(glm::two_pi<float>()); // random initial wing position
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
			vec3 vel = randVec3();
			ColorA posC = initialPos.getPixel(velIter.getPos());
			vec3 pos = vec3(posC.r, posC.g, posC.b);
			// Projects the velocity to a plane tangent to the unit sphere
			vel = 0.001f * glm::normalize(vel - glm::dot(vel, pos) * pos);
			velIter.r() = vel.x;
			velIter.g() = vel.y;
			velIter.b() = vel.z;
		}
	}
	auto velTex = gl::Texture2d::create(initialVel, fboTexFmt);
	auto velFboFmt = gl::Fbo::Format().disableDepth().attachment(GL_COLOR_ATTACHMENT0, velTex);

	mVelocitiesSource = gl::Fbo::create(mFboSide, mFboSide, velFboFmt);
	mVelocitiesDest = gl::Fbo::create(mFboSide, mFboSide, fboDefaultFmt);

	// Initialize the birds update routine
	mBirdPosUpdateProg = gl::GlslProg::create(app::loadAsset("FLRunBirds_v.glsl"), app::loadAsset("FLRunBirdsPosition_f.glsl"));
	mBirdPosUpdateProg->uniform("uGridSide", mFboSide);
	mBirdPosUpdateProg->uniform("uPositions", mPosTextureBind);
	mBirdPosUpdateProg->uniform("uVelocities", mVelTextureBind);

	mBirdVelUpdateProg = gl::GlslProg::create(app::loadAsset("FLRunBirds_v.glsl"), app::loadAsset("FLRunBirdsVelocity_f.glsl"));
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
	mBirdRenderProg = gl::GlslProg::create(app::loadAsset("FLRenderBirds_v.glsl"), app::loadAsset("FLRenderBirds_f.glsl"), app::loadAsset("FLRenderBirds_g.glsl"));
	mBirdRenderProg->uniform("uBirdPositions", mPosTextureBind);
	mBirdRenderProg->uniform("uBirdVelocities", mVelTextureBind);
	mBirdRenderBatch = gl::Batch::create(mBirdIndexMesh, mBirdRenderProg, { {geom::CUSTOM_0, "birdIndex"} });

	// Set up the cube map 360 degree camera
	auto cubeMapFormat = gl::TextureCubeMap::Format()
		.magFilter(GL_LINEAR)
		.minFilter(GL_LINEAR)
		.internalFormat(GL_RGB8);

	auto cubeMapFboFmt = FboCubeMapLayered::Format().colorFormat(cubeMapFormat);

	mCubeMapCamera = FboCubeMapLayered::create(OUTPUT_CUBE_MAP_SIDE, OUTPUT_CUBE_MAP_SIDE, cubeMapFboFmt);

	mCubeMapCameraMatrixBuffer = mCubeMapCamera->generateCameraMatrixBuffer();

	// Set up params
	mMenu = params::InterfaceGl::create(app::getWindow(), "Menu", app::toPixels(ivec2(200, 500)));
	mMenu->addParam<float>("Min Speed", & mMinSpeed).min(0.0f).max(1.0f).precision(4).step(0.0001f);
	mMenu->addParam<float>("Max Speed", & mMaxSpeed).min(0.0f).max(1.0f).precision(4).step(0.0001f);
	mMenu->addParam<float>("Min Force", & mMinForce).min(0.0f).max(1.0f).precision(4).step(0.0001f);
	mMenu->addParam<float>("Max Force", & mMaxForce).min(0.0f).max(1.0f).precision(4).step(0.0001f);
	mMenu->addParam<float>("Separation Dist", & mSeparationDist).min(0.0f).max(1.0f).precision(4).step(0.0001f);
	mMenu->addParam<float>("Separation Mod", & mSeparationMod).min(0.0f).max(1.0f).precision(4).step(0.0001f);
	mMenu->addParam<float>("Align Dist", & mAlignDist).min(0.0f).max(1.0f).precision(4).step(0.0001f);
	mMenu->addParam<float>("Align Mod", & mAlignMod).min(0.0f).max(1.0f).precision(4).step(0.0001f);
	mMenu->addParam<float>("Cohesion Dist", & mCohesionDist).min(0.0f).max(1.0f).precision(4).step(0.0001f);
	mMenu->addParam<float>("Cohesion Mod", & mCohesionMod).min(0.0f).max(1.0f).precision(4).step(0.0001f);
}

void FlockingApp::update()
{
	// Update uniforms (assuming params can change any time)
	mBirdVelUpdateProg->uniform("uMinSpeed", mMinSpeed);
	mBirdVelUpdateProg->uniform("uMaxSpeed", mMaxSpeed);

	mBirdVelUpdateProg->uniform("uMinForce", mMinForce);
	mBirdVelUpdateProg->uniform("uMaxForce", mMaxForce);
	
	mBirdVelUpdateProg->uniform("uSeparationDist", mSeparationDist);
	mBirdVelUpdateProg->uniform("uSeparationMod", mSeparationMod);
	mBirdVelUpdateProg->uniform("uAlignDist", mAlignDist);
	mBirdVelUpdateProg->uniform("uAlignMod", mAlignMod);
	mBirdVelUpdateProg->uniform("uCohesionDist", mCohesionDist);
	mBirdVelUpdateProg->uniform("uCohesionMod", mCohesionMod);

	// Run the simulation itself
	gl::ScopedBlend scpBlend(false); // No alpha blending when running the simulation - because alpha is used for data
	gl::ScopedViewport scpView(0, 0, mFboSide, mFboSide);
	gl::ScopedMatrices scpMat;
	gl::setMatricesWindow(mFboSide, mFboSide);

	// Update velocities first
	{
		gl::ScopedGlslProg scpShader(mBirdVelUpdateProg);
		gl::ScopedTextureBind scpPosTex(mPositionsSource->getColorTexture(), mPosTextureBind);
		gl::ScopedTextureBind scpVelTex(mVelocitiesSource->getColorTexture(), mVelTextureBind);

		gl::ScopedFramebuffer scpFbo(mVelocitiesDest);
		gl::clear();
		gl::drawSolidRect(Rectf(0, 0, mFboSide, mFboSide));
	}

	// Update positions second
	{
		gl::ScopedGlslProg scpShader(mBirdPosUpdateProg);
		gl::ScopedTextureBind scpPosTex(mPositionsSource->getColorTexture(), mPosTextureBind);
		gl::ScopedTextureBind scpVelTex(mVelocitiesSource->getColorTexture(), mVelTextureBind);

		gl::ScopedFramebuffer scpFbo(mPositionsDest);
		gl::clear();
		gl::drawSolidRect(Rectf(0, 0, mFboSide, mFboSide));
	}

	std::swap(mPositionsSource, mPositionsDest);
	std::swap(mVelocitiesSource, mVelocitiesDest);
}

gl::TextureCubeMapRef FlockingApp::draw()
{
	// Draw the birds into the 360 degree camera FBO
	{
		// Bind the 360 camera framebuffer
		gl::ScopedFramebuffer scpFbo(GL_FRAMEBUFFER, mCubeMapCamera->getId());

		gl::ScopedDepth scpDepth(true);
		gl::ScopedViewport scpView(0, 0, mCubeMapCamera->getWidth(), mCubeMapCamera->getHeight());

		gl::ScopedFaceCulling scpCull(false);

		gl::clear(Color(0, 0, 0));

		gl::ScopedTextureBind scpPosTex(mPositionsSource->getColorTexture(), mPosTextureBind);
		gl::ScopedTextureBind scpVelTex(mVelocitiesSource->getColorTexture(), mVelTextureBind);

		gl::ScopedColor scpColor(Color(1, 1, 1));

		// Shader uniforms
		mCubeMapCameraMatrixBuffer->bindBufferBase(mCubeMapCameraMatrixBind);
		mBirdRenderProg->uniformBlock("uMatrices", mCubeMapCameraMatrixBind);

		// Draw the birds into the 360 camera
		mBirdRenderBatch->draw();
	}

	// Return the 360 camera's color texture
	return mCubeMapCamera->getColorTex();
}
