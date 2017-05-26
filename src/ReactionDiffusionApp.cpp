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

// Returns an int corresponding to the vector's max dimension.
// If the vector is more in +X or -X, returns 0, returns 1 for +Y or -Y, and 2 for +Z or -Z
// The zero vector returns 2
int cubeMapDimensionIntFromVec(vec3 dir) {
	vec3 absV = abs(normalize(dir));
	return absV.x > absV.y && absV.x > absV.z ? 0 : (absV.y > absV.x && absV.y > absV.z ? 1 : 2);
}

// Returns the GLenum corresponding to the cubemap face pointed to by the 3D vector
GLenum cubeMapFaceFromVec(vec3 dir) {
	int cmDim = cubeMapDimensionIntFromVec(dir);

	// If the max value is 0, then the vector is the zero vector, which is an error
	// but in this case the function will return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
	// because cmDim will be 2 and sign(0) = 0
	switch (cmDim) {
		case 0:
			return glm::sign(dir.x) == 1.0 ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
		case 1:
			return glm::sign(dir.y) == 1.0 ? GL_TEXTURE_CUBE_MAP_POSITIVE_Y : GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
		case 2:
			return glm::sign(dir.z) == 1.0 ? GL_TEXTURE_CUBE_MAP_POSITIVE_Z : GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
	}

	// Execution should never reach this point
	assert(0);
}

// maps [-1, 1] to [0, 1]
float map01(float t) {
	return (t + 1.0) * 0.5;
}

// maps [0, 1] to [1, 0]
float inv(float t) {
	return 1.0 - t;
}

// Returns the uv coords on the cubemap face corresponding to the position of the 3D vector
vec2 cubeMapFaceCoordFromVec(vec3 dir) {
	GLenum cmFace = cubeMapFaceFromVec(dir);

	vec3 divd;
	switch (cmFace) {
		case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
			divd = dir / abs(dir.x);
			return vec2(inv(map01(divd.z)), map01(divd.y));
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
			divd = dir / abs(dir.x);
			return vec2(map01(divd.z), map01(divd.y));

		case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
			divd = dir / abs(dir.y);
			return vec2(map01(divd.x), map01(divd.z));
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
			divd = dir / abs(dir.y);
			return vec2(map01(divd.x), inv(map01(divd.z)));

		case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
			divd = dir / abs(dir.z);
			return vec2(map01(divd.x), inv(map01(divd.y)));
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
			divd = dir / abs(dir.z);
			return vec2(inv(map01(divd.x)), inv(map01(divd.y)));

		default:
			// This code should be unreachable
			assert(0);
	}
}

void ReactionDiffusionApp::disrupt(vec3 dir) {
	GLenum cmFace = cubeMapFaceFromVec(dir);
	vec2 cmCoords = cubeMapFaceCoordFromVec(dir);

	gl::ScopedViewport scpView(0, 0, mCubeMapSide, mCubeMapSide);
	gl::ScopedMatrices scpMat;
	gl::setMatricesWindow(mCubeMapSide, mCubeMapSide);

	GLuint tempFbo;
	glGenFramebuffers(1, & tempFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, tempFbo);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, cmFace, mSourceTex->getId(), 0);

	GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawBuffers);

	gl::ScopedColor scpC(Color(0, 1, 0));

	gl::drawSolidCircle(cmCoords * (float) mCubeMapSide, 100.0f);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, & tempFbo);
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