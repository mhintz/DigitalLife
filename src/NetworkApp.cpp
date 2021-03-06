#include "NetworkApp.h"

using namespace ci;
using std::vector;

extern uint32_t OUTPUT_CUBE_MAP_SIDE;

void NetworkApp::setup()
{
	// Set up the 360 degree cube map camera
	auto cubeMapFormat = gl::TextureCubeMap::Format()
		.magFilter(GL_LINEAR)
		.minFilter(GL_LINEAR)
		.internalFormat(GL_RGB8)
		.mipmap();

	auto cubeMapFboFmt = gl::FboCubeMap::Format()
		.textureCubeMapFormat(cubeMapFormat);

	mOutputCubeFbo = gl::FboCubeMap::create(OUTPUT_CUBE_MAP_SIDE * 2, OUTPUT_CUBE_MAP_SIDE * 2, cubeMapFboFmt);

	mRenderToCubeMap = gl::getStockShader(gl::ShaderDef().color());

	// Set up the simulation data
	for (int idx = 0; idx < mNumNetworkNodes; idx++) {
		mNetworkNodes.push_back(NetworkNode(idx, randVec3()));
		if (randFloat() < 0.1) { mNetworkNodes[idx].mInfected = true; }
	}

	vector<NetworkNode *> nodePointers;
	for (auto & node : mNetworkNodes) {
		nodePointers.push_back(& node);
	}

	for (auto & node : mNetworkNodes) {
		// Sort according to distance
		std::sort(nodePointers.begin(), nodePointers.end(), [& node] (NetworkNode * p1, NetworkNode * p2) {
			float d1 = distance(node.mPos, p1->mPos);
			float d2 = distance(node.mPos, p2->mPos);
			return d1 < d2;
		});
		// Take the nearest ones, knowing the the first one will be the node itself
		for (int i = 1; i <= mNumLinksPerNode; i++) {
			node.mLinks.insert(nodePointers[i]->mId);
			nodePointers[i]->mLinks.insert(node.mId);
			mNetworkLinks.push_back(std::make_pair(nodePointers[i]->mId, node.mId));
		}
	}

	// Set up OpenGL data structures on the GPU
	size_t numNodes = mNetworkNodes.size();

	// lol this is literally just to avoid having a shader warning all the time :/
	auto emptyTexCoordsBuf = gl::Vbo::create(GL_ARRAY_BUFFER, vector<vec2>(numNodes));
	auto emptyTexCoordsFmt = geom::BufferLayout({ geom::AttribInfo(geom::TEX_COORD_0, 2, 0, 0) });

	vector<vec3> nodePositions(numNodes);
	vector<vec3> nodeColors(numNodes);
	for (int idx = 0; idx < numNodes; idx++) {
		nodePositions[idx] = mNetworkNodes[idx].mPos;
		nodeColors[idx] = mNetworkNodes[idx].mInfected ? vec3(1, 0, 0) : vec3(0, 0, 1);
	}

	auto nodesBuf = gl::Vbo::create(GL_ARRAY_BUFFER, nodePositions);
	auto nodesFmt = geom::BufferLayout({ geom::AttribInfo(geom::POSITION, 3, 0, 0) });
	auto nodeColorsBuf = gl::Vbo::create(GL_ARRAY_BUFFER, nodeColors, GL_DYNAMIC_DRAW);
	auto nodeColorsFmt = geom::BufferLayout({ geom::AttribInfo(geom::COLOR, 3, 0, 0) });
	mNodesMesh = gl::VboMesh::create(numNodes, GL_POINTS, { { nodesFmt, nodesBuf }, { nodeColorsFmt, nodeColorsBuf }, { emptyTexCoordsFmt, emptyTexCoordsBuf } });

	size_t numLinks = mNetworkLinks.size();

	vector<vec3> linkPositions(2 * numLinks);
	vector<vec3> linkColors(2 * numLinks);
	for (int idx = 0; idx < numLinks; idx++) {
		uint id1 = mNetworkLinks[idx].first;
		uint id2 = mNetworkLinks[idx].second;
		linkPositions[2 * idx] = mNetworkNodes[id1].mPos;
		linkPositions[2 * idx + 1] = mNetworkNodes[id2].mPos;
		linkColors[2 * idx] = mNetworkNodes[id1].mInfected ? vec3(1, 0, 0) : vec3(0, 0, 1);
		linkColors[2 * idx + 1] = mNetworkNodes[id2].mInfected ? vec3(1, 0, 0) : vec3(0, 0, 1);
	}

	auto linksBuf = gl::Vbo::create(GL_ARRAY_BUFFER, linkPositions);
	auto linksFmt = geom::BufferLayout({ geom::AttribInfo(geom::POSITION, 3, 0, 0) });
	auto linkColorsBuf = gl::Vbo::create(GL_ARRAY_BUFFER, linkColors, GL_DYNAMIC_DRAW);
	auto linkColorsFmt = geom::BufferLayout({ geom::AttribInfo(geom::COLOR, 3, 0, 0) });
	mLinksMesh = gl::VboMesh::create(2 * numLinks, GL_LINES, { { linksFmt, linksBuf }, { linkColorsFmt, linkColorsBuf }, { emptyTexCoordsFmt, emptyTexCoordsBuf } });
}

void NetworkApp::update()
{
	vector<bool> willBeInfected(mNetworkNodes.size(), false);

	for (int idx = 0; idx < mNetworkNodes.size(); idx++) {
		auto & node = mNetworkNodes[idx];
		if (node.mInfected) {
			if (randFloat() < mNodeDisinfectChance) { willBeInfected[node.mId] = false; } else { willBeInfected[node.mId] = true; }
			for (uint otherId : node.mLinks) {
				if (randFloat() < mSpreadInfectionChance) { willBeInfected[otherId] = true; }
			}
		}
	}

	uint numInfected = std::accumulate(willBeInfected.begin(), willBeInfected.end(), 0, [] (uint count, bool inf) { return count + (inf ? 1 : 0); });
	if (numInfected < mMinInfected) {
		for (auto & node : mNetworkNodes) {
			if (randFloat() < (float) mMinInfected / mNumNetworkNodes) { willBeInfected[node.mId] = true; }
		}
	}

	for (int idx = 0; idx < mNetworkNodes.size(); idx++) {
		mNetworkNodes[idx].mInfected = willBeInfected[idx];
	}

	this->setColorAttribs();
}

void NetworkApp::setColorAttribs() {
	vector<vec3> nodeColors(mNetworkNodes.size());
	for (int idx = 0; idx < mNetworkNodes.size(); idx++) {
		nodeColors[idx] = mNetworkNodes[idx].mInfected ? vec3(1, 0, 0) : vec3(0, 0, 1);
	}

	mNodesMesh->findAttrib(geom::COLOR)->second->copyData(vectorByteSize(nodeColors), nodeColors.data());

	vector<vec3> linkColors(2 * mNetworkLinks.size());
	for (int idx = 0; idx < mNetworkLinks.size(); idx++) {
		uint id1 = mNetworkLinks[idx].first;
		uint id2 = mNetworkLinks[idx].second;
		linkColors[2 * idx] = mNetworkNodes[id1].mInfected ? vec3(1, 0, 0) : vec3(0, 0, 1);
		linkColors[2 * idx + 1] = mNetworkNodes[id2].mInfected ? vec3(1, 0, 0) : vec3(0, 0, 1);
	}

	mLinksMesh->findAttrib(geom::COLOR)->second->copyData(vectorByteSize(linkColors), linkColors.data());
}

void NetworkApp::disrupt(vec3 dir) {
	float const DISRUPT_RADIUS = 0.45;
	vec3 const disruptDir = normalize(dir);

	for (auto & node : mNetworkNodes) {
		if (distance(node.mPos, disruptDir) < DISRUPT_RADIUS) {
			node.mInfected = true;
		}
	}

	this->setColorAttribs();
}

gl::TextureCubeMapRef NetworkApp::draw()
{
	gl::context()->pushFramebuffer();

	gl::ScopedDepth scpDepth(true);

	gl::ScopedViewport scpView(0, 0, mOutputCubeFbo->getWidth(), mOutputCubeFbo->getHeight());
	gl::ScopedMatrices scpMat;

	gl::pointSize(5.0);

	gl::ScopedGlslProg scpShader(mRenderToCubeMap);

	for (uint8_t dir = 0; dir < 6; ++dir) {
		gl::setProjectionMatrix(ci::CameraPersp(mOutputCubeFbo->getWidth(), mOutputCubeFbo->getHeight(), 90.0f, 0.5, 5.0).getProjectionMatrix());
		gl::setViewMatrix(mOutputCubeFbo->calcViewMatrix(GL_TEXTURE_CUBE_MAP_POSITIVE_X + dir, vec3(0)));
		mOutputCubeFbo->bindFramebufferFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X + dir);

		gl::clear(ColorA(0, 0, 0, 0));
		gl::draw(mLinksMesh);
		gl::draw(mNodesMesh);
	}

	gl::pointSize(1.0);

	gl::context()->popFramebuffer();

	return mOutputCubeFbo->getTextureCubeMap();
}
