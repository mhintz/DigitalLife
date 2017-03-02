#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class DigitalLifeApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
};

void DigitalLifeApp::setup()
{
}

void DigitalLifeApp::mouseDown( MouseEvent event )
{
}

void DigitalLifeApp::update()
{
}

void DigitalLifeApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( DigitalLifeApp, RendererGl )
