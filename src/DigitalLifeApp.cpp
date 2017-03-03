#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"

#include "Syphon.h"

using namespace ci;
using namespace ci::app;

class DigitalLifeApp : public App {
  public:
	static void prepareSettings(Settings * settings);

	void setup() override;
	void update() override;
	void draw() override;

	gl::FboRef randomFbo;
	ciSyphon::ServerRef mSyphonServer;
};

void DigitalLifeApp::prepareSettings(Settings * settings) {
	settings->setTitle("Digital Life");
//	settings->setFullScreen();
//	settings->setHighDensityDisplayEnabled();
}

void DigitalLifeApp::setup() {
	randomFbo = gl::Fbo::create(getWindowWidth(), getWindowHeight());
	mSyphonServer = ciSyphon::Server::create();
	mSyphonServer->setName("DigitalLifeServer");
}

void DigitalLifeApp::update() {
}

void DigitalLifeApp::draw() {
	randomFbo->bindFramebuffer();

	gl::clear(Color(sinf(getElapsedSeconds() * 1.0f) * 0.25f + 0.5f,
					sinf(getElapsedSeconds() * 0.6f) * 0.25f + 0.5f,
					sinf(getElapsedSeconds() * 0.3f) * 0.25f + 0.5f));

	gl::enableAlphaBlending();
	gl::drawStringCentered("Hello from Cinder!", getWindowCenter() - vec2(0, 32), Color::white(), Font("Arial", 64));
	gl::disableAlphaBlending();

	randomFbo->unbindFramebuffer();

	// Publish to Syphon

	// This works with gaborpapp's version but not reza's
	mSyphonServer->publishTexture(randomFbo->getColorTexture());

	// This doesn't work on gaborpapp's version but does work on reza's *AND* on the current Syphon master branch
	// mSyphonServer->bind(vec2(getWindowWidth(), getWindowHeight()));
	// gl::draw(randomFbo->getColorTexture());
	// mSyphonServer->unbind();

	// Draw to the Cinder window
	gl::clear();
	gl::draw(randomFbo->getColorTexture());

	// This works, with occasional glitches on gaborpapp's version but not reza's
//	mSyphonServer->publishScreen();
}

CINDER_APP(DigitalLifeApp, RendererGl, & DigitalLifeApp::prepareSettings)
