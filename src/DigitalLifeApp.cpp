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
};

void DigitalLifeApp::prepareSettings(Settings * settings) {
	settings->setTitle("Digital Life");
	settings->setFullScreen();
	settings->setHighDensityDisplayEnabled();
}

void DigitalLifeApp::setup() {
}

void DigitalLifeApp::update() {
}

void DigitalLifeApp::draw() {
	gl::clear(Color(0, 0, 0)); 
}

CINDER_APP(DigitalLifeApp, RendererGl, & DigitalLifeApp::prepareSettings)
