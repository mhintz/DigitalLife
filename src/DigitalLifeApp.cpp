#include <vector>

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "Syphon.h"

#include "FlockingApp.hpp"

using namespace ci;
using namespace ci::app;
using std::vector;

uint32_t OUTPUT_CUBE_MAP_SIDE = 1600;

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

	FlockingApp mFlockingApp;
};

void DigitalLifeApp::prepareSettings(Settings * settings) {
	settings->setTitle("Digital Life");
}

void genFace(vec2 ul, vec2 lr, vec3 tu, vec3 tv, vec3 tw, vector<vec2> * positions, vector<vec3> * texCoords) {
	positions->emplace_back(lr.x, ul.y); // upper-right
	texCoords->emplace_back(normalize(tw + tu + tv));
	positions->emplace_back(ul.x, ul.y); // upper-left
	texCoords->emplace_back(normalize(tw + tv));
	positions->emplace_back(lr.x, lr.y); // lower-right
	texCoords->emplace_back(normalize(tw + tu));

	positions->emplace_back(ul.x, lr.y); // lower-left
	texCoords->emplace_back(normalize(tw));
	positions->emplace_back(lr.x, lr.y); // lower-right
	texCoords->emplace_back(normalize(tw + tu));
	positions->emplace_back(ul.x, ul.y); // upper-left
	texCoords->emplace_back(normalize(tw + tv));
}

void DigitalLifeApp::setup() {
	mOutputFbo = gl::Fbo::create(6 * OUTPUT_CUBE_MAP_SIDE, OUTPUT_CUBE_MAP_SIDE);

	vector<vec2> positions;
	vector<vec3> texCoords;
	uint32_t side = OUTPUT_CUBE_MAP_SIDE;

	// Generate six sets of positions and texcoords for drawing the six faces of the cube map
	// + X
	genFace(vec2(side * 0, 0), vec2(side * 1, side), vec3(0, 0, -2), vec3(0, 2, 0), vec3(1, -1, 1), & positions, & texCoords);
	// - X
	genFace(vec2(side * 1, 0), vec2(side * 2, side), vec3(0, 0, 2), vec3(0, 2, 0), vec3(-1, -1, -1), & positions, & texCoords);
	// + Y
	genFace(vec2(side * 2, 0), vec2(side * 3, side), vec3(2, 0, 0), vec3(0, 0, -2), vec3(-1, 1, 1), & positions, & texCoords);
	// - Y
	genFace(vec2(side * 3, 0), vec2(side * 4, side), vec3(2, 0, 0), vec3(0, 0, 2), vec3( -1, -1, -1 ), & positions, & texCoords);
	// + Z
	genFace(vec2(side * 4, 0), vec2(side * 5, side), vec3(2, 0, 0), vec3(0, 2, 0), vec3(-1, -1, 1), & positions, & texCoords);
	// - Z
	genFace(vec2(side * 5, 0), vec2(side * 6, side), vec3(-2, 0, 0), vec3(0, 2, 0), vec3(1, -1, -1), & positions, & texCoords);

	auto posBuf = gl::Vbo::create(GL_ARRAY_BUFFER, positions, GL_STREAM_DRAW);
	auto posBufLayout = geom::BufferLayout({ geom::AttribInfo(geom::POSITION, 2, 0, 0) });
	auto texBuf = gl::Vbo::create(GL_ARRAY_BUFFER, texCoords, GL_STREAM_DRAW);
	auto texBufLayout = geom::BufferLayout({ geom::AttribInfo(geom::TEX_COORD_0, 3, 0, 0) });

	auto outputMesh = gl::VboMesh::create(positions.size(), GL_TRIANGLES, { { posBufLayout, posBuf }, { texBufLayout, texBuf } });
	auto outputShader = gl::GlslProg::create(loadAsset("drawCubeMapToRect_v.glsl"), loadAsset("drawCubeMapToRect_f.glsl"));
	outputShader->uniform("uCubeMap", mAppTextureBind);

	mOutputBatch = gl::Batch::create(outputMesh, outputShader);

	mSyphonServer = ciSyphon::Server::create();
	mSyphonServer->setName("DigitalLifeServer");

	mFlockingApp.setup();
}

void DigitalLifeApp::keyDown(KeyEvent evt) {
	if (evt.getCode() == KeyEvent::KEY_ESCAPE) {
		quit();
	}
}

void DigitalLifeApp::update() {
	mFlockingApp.update();
}

void DigitalLifeApp::draw() {
	gl::TextureCubeMapRef appInstanceCubeMapFrame = mFlockingApp.draw();

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
		// gl::drawEquirectangular(appInstanceCubeMapFrame, toPixels(Rectf(0, 0, getWindowWidth(), getWindowHeight())));
		gl::drawHorizontalCross(appInstanceCubeMapFrame, toPixels(Rectf(0, 0, getWindowWidth(), getWindowHeight())));

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
