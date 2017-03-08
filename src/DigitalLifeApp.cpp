#include <vector>

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Camera.h"
#include "cinder/CameraUi.h"

#include "Syphon.h"

#include "MeshHelpers.h"

#include "FlockingApp.h"
#include "ReactionDiffusionApp.h"

using namespace ci;
using namespace ci::app;
using std::vector;

uint32_t OUTPUT_CUBE_MAP_SIDE = 512;

class DigitalLifeApp : public App {
  public:
	static void prepareSettings(Settings * settings);

	void setup() override;
	void update() override;
	void draw() override;

	void keyDown(KeyEvent evt) override;

	gl::FboRef mOutputFbo;
	uint8_t mAppTextureBind = 0;
	gl::BatchRef mOutputBatch;
	ciSyphon::ServerRef mSyphonServer;

	CameraPersp mCamera;
	CameraUi mCameraUi;
	gl::GlslProgRef mRenderTexAsSphereShader;

	ReactionDiffusionApp mReactionDiffusionApp;
	FlockingApp mFlockingApp;
};

void DigitalLifeApp::prepareSettings(Settings * settings) {
	settings->setTitle("Digital Life");
}

void DigitalLifeApp::setup() {
	mOutputFbo = gl::Fbo::create(6 * OUTPUT_CUBE_MAP_SIDE, OUTPUT_CUBE_MAP_SIDE);

	auto outputMesh = makeCubeMapToRowLayoutMesh(OUTPUT_CUBE_MAP_SIDE);
	auto outputShader = gl::GlslProg::create(loadAsset("drawCubeMapToRect_v.glsl"), loadAsset("drawCubeMapToRect_f.glsl"));
	outputShader->uniform("uCubeMap", mAppTextureBind);

	mOutputBatch = gl::Batch::create(outputMesh, outputShader);

	mSyphonServer = ciSyphon::Server::create();
	mSyphonServer->setName("DigitalLifeServer");

	mCamera.lookAt(vec3(0, 0, 4), vec3(0), vec3(0, 1, 0));
	mCameraUi = CameraUi(& mCamera, getWindow());
	mRenderTexAsSphereShader = gl::GlslProg::create(loadAsset("renderOutputTexAsSphere_v.glsl"), loadAsset("renderOutputTexAsSphere_f.glsl"));

	mReactionDiffusionApp.setup();
	mFlockingApp.setup();
}

void DigitalLifeApp::keyDown(KeyEvent evt) {
	if (evt.getCode() == KeyEvent::KEY_ESCAPE) {
		quit();
	}
}

void DigitalLifeApp::update() {
	mReactionDiffusionApp.update();
	// mFlockingApp.update();
}

void DigitalLifeApp::draw() {
	gl::TextureCubeMapRef appInstanceCubeMapFrame = mReactionDiffusionApp.draw();
	// gl::TextureCubeMapRef appInstanceCubeMapFrame = mFlockingApp.draw();

	// Draw the cubemap to the wide FBO
	{
		gl::ScopedFramebuffer scpFbo(mOutputFbo);

		gl::ScopedMatrices scpMat;
		gl::setMatricesWindow(6 * OUTPUT_CUBE_MAP_SIDE, OUTPUT_CUBE_MAP_SIDE);
		gl::ScopedViewport scpView(0, 0, 6 * OUTPUT_CUBE_MAP_SIDE, OUTPUT_CUBE_MAP_SIDE);

		gl::clear(Color(0, 0, 0));

		gl::ScopedTextureBind scpTex(appInstanceCubeMapFrame, mAppTextureBind);

		mOutputBatch->draw();
	}

	gl::clear();

	// Debug zone
	{
		// {
		// 	gl::ScopedDepth scpDepth(true);
		// 	gl::ScopedFaceCulling scpCull(true, GL_BACK);

		// 	gl::ScopedMatrices scpMat;
		// 	gl::setMatrices(mCamera);

		// 	gl::ScopedTextureBind scpTex(appInstanceCubeMapFrame);
		// 	gl::ScopedGlslProg scpShader(mRenderTexAsSphereShader);

		// 	gl::draw(geom::Sphere().center(vec3(0)).radius(1.0f).subdivisions(50));
		// }

		// gl::drawEquirectangular(appInstanceCubeMapFrame, Rectf(0, 0, getWindowWidth(), getWindowHeight()));
		gl::drawHorizontalCross(appInstanceCubeMapFrame, Rectf(0, 0, getWindowWidth(), getWindowHeight()));

		// gl::draw(mOutputFbo->getColorTexture(), Rectf(0, 0, getWindowWidth(), getWindowHeight() / 3));

		gl::drawString(std::to_string(getAverageFps()), vec2(10.0f, 20.0f), ColorA(1.0f, 1.0f, 1.0f, 1.0f));
	}

	// Publish to Syphon

	// This works with gaborpapp's version but not reza's
	mSyphonServer->publishTexture(mOutputFbo->getColorTexture());

	// This doesn't work on gaborpapp's version but does work on reza's *AND* on the current Syphon master branch
	// mSyphonServer->bind(vec2(getWindowWidth(), getWindowHeight()));
	// gl::draw(randomFbo->getColorTexture());
	// mSyphonServer->unbind();

	// This works, with occasional glitches on gaborpapp's version but not reza's
	// mSyphonServer->publishScreen();
}

CINDER_APP(DigitalLifeApp, RendererGl, & DigitalLifeApp::prepareSettings)
