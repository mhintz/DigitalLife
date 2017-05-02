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
		.internalFormat(GL_RGB8);

	auto cubeMapFboFmt = FboCubeMapLayered::Format().colorFormat(cubeMapFormat);

	mOutputCubeFbo = FboCubeMapLayered::create(OUTPUT_CUBE_MAP_SIDE, OUTPUT_CUBE_MAP_SIDE, cubeMapFboFmt);

	int const matrixBindingPoint = 1;

	mMatrixBuffer = mOutputCubeFbo->generateCameraMatrixBuffer();
	mMatrixBuffer->bindBufferBase(matrixBindingPoint);

	mRenderLinesToCubeMap = gl::GlslProg::create(app::loadAsset("DLRenderIntoCubeMap_v.glsl"), app::loadAsset("DLRenderIntoCubeMap_f.glsl"), app::loadAsset("DLRenderIntoCubeMap_lines_g.glsl"));
	mRenderLinesToCubeMap->uniformBlock("uMatrices", matrixBindingPoint);

	mRenderPointsToCubeMap = gl::GlslProg::create(app::loadAsset("DLRenderIntoCubeMap_v.glsl"), app::loadAsset("DLRenderIntoCubeMap_f.glsl"), app::loadAsset("DLRenderIntoCubeMap_points_g.glsl"));
	mRenderPointsToCubeMap->uniformBlock("uMatrices", matrixBindingPoint);

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
		// Take the nearest six which don't already have six links
		// (knowing the the first one will be the node itself)
		for (int i = 1; i < nodePointers.size(); i++) {
			if (node.mLinks.size() >= mNumLinksPerNode) { break; }

			if (nodePointers[i]->mLinks.size() < mNumLinksPerNode) {
				node.mLinks.insert(nodePointers[i]->mId);
				nodePointers[i]->mLinks.insert(node.mId);
				mNetworkLinks.push_back(std::make_pair(nodePointers[i]->mId, node.mId));
			}
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

gl::TextureCubeMapRef NetworkApp::draw()
{
	gl::clear(Color(0, 0, 0));

	gl::ScopedDepth scpDepth(true);
	gl::pointSize(5.0);

	{
		gl::ScopedViewport scpView(0, 0, mOutputCubeFbo->getWidth(), mOutputCubeFbo->getHeight());

		gl::ScopedMatrices scpMat;

		gl::ScopedFramebuffer scpFbo(GL_FRAMEBUFFER, mOutputCubeFbo->getId());

		gl::clear(Color(0, 0, 0));

		{
			gl::ScopedGlslProg scpShader(mRenderLinesToCubeMap);

			gl::draw(mLinksMesh);
		}

		{
			gl::ScopedGlslProg scpShader(mRenderPointsToCubeMap);

			gl::draw(mNodesMesh);
		}
	}

	return mOutputCubeFbo->getColorTex();
}
