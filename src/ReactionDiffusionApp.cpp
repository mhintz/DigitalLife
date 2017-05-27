#include "ReactionDiffusionApp.h"

extern uint32_t OUTPUT_CUBE_MAP_SIDE;

void ReactionDiffusionApp::setup() {
	auto colorTextureFormat = gl::TextureCubeMap::Format()
		.internalFormat(GL_RGB32F)
		.wrap(GL_CLAMP_TO_EDGE)
		.minFilter(GL_NEAREST)
		.magFilter(GL_NEAREST)
		.mipmap(false);

	GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };

	{
		mSourceTex = gl::TextureCubeMap::create(mCubeMapSide, mCubeMapSide, colorTextureFormat);
		glGenFramebuffers(1, & mSourceFbo);
		gl::ScopedFramebuffer scpFbo(GL_FRAMEBUFFER, mSourceFbo);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mSourceTex->getId(), 0);
		glDrawBuffers(1, drawBuffers);
		gl::clear(Color(0, 1, 0)); // Clear to all "A"
	}

	{
		mDestTex = gl::TextureCubeMap::create(mCubeMapSide, mCubeMapSide, colorTextureFormat);
		glGenFramebuffers(1, & mDestFbo);
		gl::ScopedFramebuffer scpFbo(GL_FRAMEBUFFER, mDestFbo);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mDestTex->getId(), 0);
		glDrawBuffers(1, drawBuffers);
		gl::clear(Color(0, 1, 0)); // Clear to all "A"
	}

	mRDProgram = gl::GlslProg::create(ci::app::loadAsset("RDRunReactionDiffusion_v.glsl"), ci::app::loadAsset("RDRunReactionDiffusion_f.glsl"), ci::app::loadAsset("RDRunReactionDiffusion_g.glsl"));
	mRDProgram->uniform("gridSideLength", mCubeMapSide);
	mRDProgram->uniform("uPrevFrame", mRDReadFboBinding);
	// mRDProgram->uniform("feedRateA", mTypeEpsilon_microbes[0]);
	// mRDProgram->uniform("killRateB", mTypeEpsilon_microbes[1]);
	mRDProgram->uniform("feedRateA", mTypeAlpha_waves[0]);
	mRDProgram->uniform("killRateB", mTypeAlpha_waves[1]);

	// This is a very small GL draw buffer ;)
	{
		vec3 pointBuf[1] = { vec3(mCubeMapSide / 2, mCubeMapSide / 2, 0) };
		auto pointVbo = gl::Vbo::create(GL_ARRAY_BUFFER, sizeof(vec3), pointBuf, GL_STATIC_DRAW);
		auto pointVboLayout = geom::BufferLayout({ geom::AttribInfo(geom::POSITION, 3, 0, 0) });
		mPointMesh = gl::VboMesh::create(1, GL_POINTS, { { pointVboLayout, pointVbo } });
	}

	mDisruptShader = gl::GlslProg::create(ci::app::loadAsset("RDRunReactionDiffusion_v.glsl"), ci::app::loadAsset("RDDisruptReactionDiffusion_f.glsl"), ci::app::loadAsset("RDRunReactionDiffusion_g.glsl"));

	mRenderRDProgram = gl::GlslProg::create(ci::app::loadAsset("RDRenderReactionDiffusion_v.glsl"), ci::app::loadAsset("RDRenderReactionDiffusion_f.glsl"), ci::app::loadAsset("RDRenderReactionDiffusion_g.glsl"));
	mRenderRDProgram->uniform("uGridSampler", mRDRenderTextureBinding);

	mCubeMapFacesMesh = makeCubeMapFaceMesh();
	mRenderCubeMapBatch = gl::Batch::create(mCubeMapFacesMesh, mRenderRDProgram, { { geom::CUSTOM_0, "aFaceIndex" } });

	auto cameraCubeMapFormat = gl::TextureCubeMap::Format()
		.magFilter(GL_LINEAR)
		.minFilter(GL_LINEAR)
		.internalFormat(GL_RGB8);

	auto cubeMapFboFmt = FboCubeMapLayered::Format().colorFormat(cameraCubeMapFormat);

	mCubeMapCamera = FboCubeMapLayered::create(OUTPUT_CUBE_MAP_SIDE, OUTPUT_CUBE_MAP_SIDE, cubeMapFboFmt);

	setupCircleRD(20);
}

void ReactionDiffusionApp::update() {
	gl::ScopedDepth scpDepth(false);

	gl::ScopedViewport scpView(0, 0, mCubeMapSide, mCubeMapSide);

	gl::ScopedMatrices scpMat;
	gl::setMatricesWindow(mCubeMapSide, mCubeMapSide);

	gl::ScopedGlslProg scpShader(mRDProgram);

	// Update the reaction-diffusion system multiple times per frame to speed things up
	for (int i = 0; i < mUpdatesPerFrame; i++) {
		// Bind the source texture to read the previous state
		gl::ScopedTextureBind scpTex(mSourceTex, mRDReadFboBinding);
		// Bind the destination FBO (with the destination texture attached) to write the new state
		gl::ScopedFramebuffer scpFbo(GL_FRAMEBUFFER, mDestFbo);
		
		// Draw a single point, using the GS to expand this one vertex into the six layers of the CubeMap texture
		gl::draw(mPointMesh);

		// Swap both the Texture pointers and the Fbo IDs to ping pong the buffers
		std::swap(mSourceTex, mDestTex);
		std::swap(mSourceFbo, mDestFbo);
	}
}

void ReactionDiffusionApp::disrupt(vec3 dir) {
	dir.y *= -1;
	// ^^^^ This is a total hack
	// I honestly don't know why this is necessary but it is :/

	gl::ScopedDepth scpDepth(false);

	gl::ScopedViewport scpView(0, 0, mCubeMapSide, mCubeMapSide);
	gl::ScopedMatrices scpMat;
	gl::setMatricesWindow(mCubeMapSide, mCubeMapSide);

	mDisruptShader->uniform("uDisruptionPoint", normalize(dir));

	gl::ScopedGlslProg scpShader(mDisruptShader);

	gl::ScopedFramebuffer scpFbo(GL_FRAMEBUFFER, mSourceFbo);

	gl::draw(mPointMesh);
}

gl::TextureCubeMapRef ReactionDiffusionApp::draw() {
	gl::ScopedFramebuffer scpFbo(GL_FRAMEBUFFER, mCubeMapCamera->getId());

	gl::ScopedViewport scpView(0, 0, mCubeMapCamera->getWidth(), mCubeMapCamera->getHeight());

	gl::clear(Color(0, 0, 0));

	gl::ScopedTextureBind scpTex(mDestTex, mRDRenderTextureBinding);

	mRenderCubeMapBatch->draw();

	return mCubeMapCamera->getColorTex();
}

void ReactionDiffusionApp::setupCircleRD(float rad) {
	gl::ScopedViewport scpView(0, 0, mCubeMapSide, mCubeMapSide);
	gl::ScopedMatrices scpMat;
	gl::setMatricesWindow(mCubeMapSide, mCubeMapSide);

	GLuint tempFbo;
	glGenFramebuffers(1, & tempFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, tempFbo);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, mSourceTex->getId(), 0);

	GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawBuffers);

	gl::clear(Color(0, 1, 0));

	gl::ScopedColor scpC(Color(0, 0, 1));

	gl::drawStrokedCircle(vec2(mCubeMapSide / 2.0f, mCubeMapSide / 2.0f), rad, 8.0f);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, & tempFbo);
}