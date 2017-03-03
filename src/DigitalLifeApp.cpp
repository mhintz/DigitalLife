#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "Syphon.h"

using namespace ci;
using namespace ci::app;

class DigitalLifeApp : public App {
  public:
	static void prepareSettings(Settings * settings);

	void setup() override;
	void update() override;
	void draw() override;

	void keyDown(KeyEvent evt) override;

	ciSyphon::ServerRef mSyphonServer;
};

void DigitalLifeApp::prepareSettings(Settings * settings) {
	settings->setTitle("Digital Life");
	// settings->setFullScreen();
	// settings->setHighDensityDisplayEnabled();
}

void DigitalLifeApp::setup() {
	mSyphonServer = ciSyphon::Server::create();
	mSyphonServer->setName("DigitalLifeServer");

}

void DigitalLifeApp::keyDown(KeyEvent evt) {
	if (evt.getCode() == KeyEvent::KEY_ESCAPE) {
		quit();
	}
}

void DigitalLifeApp::update() {
}

void DigitalLifeApp::draw() {
	gl::clear();




	// Publish to Syphon

	// This works with gaborpapp's version but not reza's
	// mSyphonServer->publishTexture(randomFbo->getColorTexture());

	// This doesn't work on gaborpapp's version but does work on reza's *AND* on the current Syphon master branch
	// mSyphonServer->bind(vec2(getWindowWidth(), getWindowHeight()));
	// gl::draw(randomFbo->getColorTexture());
	// mSyphonServer->unbind();

	// This works, with occasional glitches on gaborpapp's version but not reza's
	// mSyphonServer->publishScreen();
}

CINDER_APP(DigitalLifeApp, RendererGl, & DigitalLifeApp::prepareSettings)
