#include <string>
#include <vector>

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Camera.h"
#include "cinder/CameraUi.h"
#include "cinder/ObjLoader.h"
#include "cinder/Serial.h"
#include "cinder/Log.h"
#include "cinder/audio/audio.h"

#include "Syphon.h"

#include "MeshHelpers.h"

#include "ReactionDiffusionApp.h"
#include "FlockingApp.h"
#include "NetworkApp.h"

using namespace ci;
using namespace ci::app;
using std::vector;

// uint32_t OUTPUT_CUBE_MAP_SIDE = 512;
uint32_t OUTPUT_CUBE_MAP_SIDE = 1024;

enum class AppType {
	REACTION_DIFFUSION,
	FLOCKING,
	NETWORK,
	CUBE_DEBUG
};

enum class AppMode {
	DEVELOPMENT,
	DISPLAY
};

class DigitalLifeApp : public App {
	public:
	static void prepareSettings(Settings * settings);

	void setup() override;
	void update() override;
	void draw() override;

	void keyDown(KeyEvent evt) override;

	SerialRef attemptArduinoCxn();
	vec3 getDisruptionVector(uint8_t dir);

	// App variables
	gl::FboRef mOutputFbo;
	uint8_t mAppTextureBind = 0;
	gl::BatchRef mOutputBatch;
	ciSyphon::ServerRef mSyphonServer;

	// App state stuff
	AppType mActiveAppType = AppType::REACTION_DIFFUSION;
	AppMode mActiveAppMode = AppMode::DEVELOPMENT;

	// Arduino connection stuff
	SerialRef mArduinoCxn;
	bool mArduinoNoCxnLogged = false;

	// The simulations themselves
	ReactionDiffusionApp mReactionDiffusionApp;
	FlockingApp mFlockingApp;
	NetworkApp mNetworkApp;

	// Narration playback and coordination stuff
	audio::VoiceSamplePlayerNodeRef mNarrationPlayer;

	// Debug and config stuff
	CameraPersp mCamera;
	CameraUi mCameraUi;
	gl::GlslProgRef mRenderTexAsSphereShader;

	gl::BatchRef mSparckConfigCube;
	FboCubeMapLayeredRef mSparckConfigDrawFbo;
	gl::UboRef mSparckConfigDrawMatrices;
	gl::TextureCubeMapRef drawDebugCube();
};

void DigitalLifeApp::prepareSettings(Settings * settings) {
	settings->setTitle("Digital Life");
	settings->setHighDensityDisplayEnabled();
	// settings->setWindowSize(800, 650);
//	settings->setFullScreen();
}

void DigitalLifeApp::setup() {
	mOutputFbo = gl::Fbo::create(6 * OUTPUT_CUBE_MAP_SIDE, OUTPUT_CUBE_MAP_SIDE);

	auto outputMesh = makeCubeMapToRowLayoutMesh_SPARCK(OUTPUT_CUBE_MAP_SIDE);
	auto outputShader = gl::GlslProg::create(loadAsset("DLOutputCubeMapToRect_v.glsl"), loadAsset("DLOutputCubeMapToRect_f.glsl"));
	outputShader->uniform("uCubeMap", mAppTextureBind);

	mOutputBatch = gl::Batch::create(outputMesh, outputShader);

	mSyphonServer = ciSyphon::Server::create();
	mSyphonServer->setName("DigitalLifeServer");

	auto cubeObj = ObjLoader(loadAsset("BoxSides.obj"));
	auto cubeVboMesh = gl::VboMesh::create(cubeObj, {
		{gl::VboMesh::Layout().attrib(geom::Attrib::POSITION, 3), nullptr},
		{gl::VboMesh::Layout().attrib(geom::Attrib::TEX_COORD_0, 2), nullptr}
	});
	auto cubeShader = gl::GlslProg::create(loadAsset("DLRenderIntoCubeMap_v.glsl"), loadAsset("DLRenderIntoCubeMap_f.glsl"), loadAsset("DLRenderIntoCubeMap_triangles_g.glsl"));
	mSparckConfigCube = gl::Batch::create(cubeVboMesh, cubeShader);
	mSparckConfigDrawFbo = FboCubeMapLayered::create(OUTPUT_CUBE_MAP_SIDE, OUTPUT_CUBE_MAP_SIDE);
	mSparckConfigDrawMatrices = mSparckConfigDrawFbo->generateCameraMatrixBuffer();
	mSparckConfigDrawMatrices->bindBufferBase(1);
	cubeShader->uniformBlock("uMatrices", 1);

	mReactionDiffusionApp.setup();
	mFlockingApp.setup();
	mNetworkApp.setup();

	// Setup audio track

	audio::SourceFileRef narrationTrackFile = audio::load(loadAsset("AudioNarration.mp3"));
	mNarrationPlayer = audio::Voice::create(narrationTrackFile);

	mNarrationPlayer->getSamplePlayerNode()->setLoopEnabled();

	// Setup viewing camera
	mCamera.lookAt(vec3(0, 0, 3.5), vec3(0), vec3(0, 1, 0));
	mCameraUi = CameraUi(& mCamera, getWindow());
	mRenderTexAsSphereShader = gl::GlslProg::create(loadAsset("DLRenderOutputTexAsSphere_v.glsl"), loadAsset("DLRenderOutputTexAsSphere_f.glsl"));
}

void DigitalLifeApp::keyDown(KeyEvent evt) {
	if (evt.getCode() == KeyEvent::KEY_ESCAPE) {
		quit();
	} else if (evt.getCode() == KeyEvent::KEY_1) {
		mActiveAppType = AppType::REACTION_DIFFUSION;
	} else if (evt.getCode() == KeyEvent::KEY_2) {
		mActiveAppType = AppType::FLOCKING;
	} else if (evt.getCode() == KeyEvent::KEY_3) {
		mActiveAppType = AppType::NETWORK;
	} else if (evt.getCode() == KeyEvent::KEY_4) {
		mActiveAppType = AppType::CUBE_DEBUG;
	} else if (evt.getCode() == KeyEvent::KEY_SPACE) {
		if (mActiveAppMode == AppMode::DEVELOPMENT) {
			if (mNarrationPlayer->isPlaying()) {
				mNarrationPlayer->pause();
			} else {
				mNarrationPlayer->start();
			}
		}
	}
}

vec3 DigitalLifeApp::getDisruptionVector(uint8_t dir) {
	assert(0 <= dir && dir <= 5);

	float const SLICE_INC = M_TWO_PI / 6.0f;
	float const SLICE_START = -SLICE_INC / 2.0f;

	float zxAngle = SLICE_START + dir * SLICE_INC + randFloat() * SLICE_INC; // angle in the zx plane
	float yAngle = clamp(M_PI / 8.0f + randFloat() * M_PI, 0, M_PI);

	return getPointOnSphere(yAngle, zxAngle);
}

SerialRef DigitalLifeApp::attemptArduinoCxn() {
	if (Serial::getDevices().size()) {
		try {
			auto cxnDevice = Serial::findDeviceByNameContains("cu.usbmodem", true);
			auto cxn = Serial::create(cxnDevice, 9600);

			CI_LOG_I("Successfully connected with arduino at port: " << cxnDevice.getName());
			mArduinoNoCxnLogged = false;

			return cxn;
		} catch (SerialExc exc) {
			if (!mArduinoNoCxnLogged) {
				CI_LOG_EXCEPTION("Failed to connect to arduino", exc);
				mArduinoNoCxnLogged = true;
			}

			return nullptr;
		}
	} else if (!mArduinoNoCxnLogged) {
		CI_LOG_W("No active Arduino detected!");
		mArduinoNoCxnLogged = true;
	}

	return nullptr;
}

void DigitalLifeApp::update() {
	if (!mArduinoCxn) {
		mArduinoCxn = attemptArduinoCxn();
	}

	if (mArduinoCxn && mArduinoCxn->getNumBytesAvailable() > 0) {
		uint8_t ardMessage;
		mArduinoCxn->readAvailableBytes(& ardMessage, 1);
				
		if (0 <= ardMessage && ardMessage <= 5) {
			CI_LOG_I("Disturb at: " << (int) ardMessage);
			vec3 disruptionVector = getDisruptionVector(ardMessage);
			switch (mActiveAppType) {
				case AppType::REACTION_DIFFUSION: mReactionDiffusionApp.disrupt(disruptionVector); break;
				case AppType::FLOCKING: mFlockingApp.disrupt(disruptionVector); break;
				case AppType::NETWORK: mNetworkApp.disrupt(disruptionVector); break;
				case AppType::CUBE_DEBUG: break;
			}
		} else {
			CI_LOG_W("weird value from microphones: " << (int) ardMessage);
		}
	}

	if (mActiveAppMode == AppMode::DISPLAY) {
		if (!mNarrationPlayer->isPlaying()) {
			mNarrationPlayer->start();
		}
	}

	switch (mActiveAppType) {
		case AppType::REACTION_DIFFUSION: mReactionDiffusionApp.update(); break;
		case AppType::FLOCKING: mFlockingApp.update(); break;
		case AppType::NETWORK: mNetworkApp.update(); break;
		case AppType::CUBE_DEBUG: break;
	}
}

gl::TextureCubeMapRef DigitalLifeApp::drawDebugCube() {
	gl::ScopedViewport scpView(0, 0, mSparckConfigDrawFbo->getWidth(), mSparckConfigDrawFbo->getHeight());

	gl::ScopedMatrices scpMat;

	gl::ScopedFramebuffer scpFbo(GL_FRAMEBUFFER, mSparckConfigDrawFbo->getId());

	gl::clear(Color(0, 0, 0));

	gl::ScopedColor scpColor(Color(1, 1, 1));

	mSparckConfigCube->draw();

	return mSparckConfigDrawFbo->getColorTex();
}

void DigitalLifeApp::draw() {
	gl::TextureCubeMapRef appInstanceCubeMapFrame;
	switch (mActiveAppType) {
		case AppType::REACTION_DIFFUSION: appInstanceCubeMapFrame = mReactionDiffusionApp.draw(); break;
		case AppType::FLOCKING: appInstanceCubeMapFrame = mFlockingApp.draw(); break;
		case AppType::NETWORK: appInstanceCubeMapFrame = mNetworkApp.draw(); break;
		case AppType::CUBE_DEBUG: appInstanceCubeMapFrame = drawDebugCube(); break;
	}

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

	// Draw the main sphere monitoring window
	{
		gl::clear();

		gl::ScopedDepth scpDepth(true);
		gl::ScopedFaceCulling scpCull(true, GL_BACK);

		gl::ScopedMatrices scpMat;
		gl::setMatrices(mCamera);

		gl::ScopedTextureBind scpTex(appInstanceCubeMapFrame);
		gl::ScopedGlslProg scpShader(mRenderTexAsSphereShader);

		gl::draw(geom::Sphere().center(vec3(0)).radius(1.0f).subdivisions(50));

		gl::drawString(std::to_string(getAverageFps()), vec2(10.0f, getWindowHeight() - 40.0f), ColorA(1.0f, 1.0f, 1.0f, 1.0f));
	}

	// Debug zone
	if (mActiveAppMode == AppMode::DEVELOPMENT) {
		{
			gl::ScopedDepth scpDepth(true);
			gl::ScopedMatrices scpMat;
			gl::setMatrices(mCamera);

			gl::drawCoordinateFrame(2.0f);
		}

		{
			mFlockingApp.mMenu->draw();
		}

		{
			// gl::clear();

			// gl::drawEquirectangular(appInstanceCubeMapFrame, Rectf(0, 0, getWindowWidth(), getWindowHeight()));
			// gl::drawHorizontalCross(appInstanceCubeMapFrame, Rectf(0, 0, getWindowWidth(), getWindowHeight()));

			// gl::draw(mOutputFbo->getColorTexture(), Rectf(0, 0, getWindowWidth(), getWindowHeight() / 3));
		}
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
