#pragma once

#include <vector>
#include <unordered_set>
#include <utility>
#include <algorithm>
#include <numeric>

#include "cinder/Rand.h"

#include "CoreMath.h"
#include "FboCubeMapLayered.h"

class NetworkNode {
public:
	bool mInfected = false;
	uint mId;
	vec3 mPos;
	std::unordered_set<uint> mLinks;

	NetworkNode(uint id, vec3 pos) : mId(id), mPos(pos) {}
};

typedef std::shared_ptr<NetworkNode> NetworkNodeRef;

class NetworkApp {
public:
	NetworkApp() {}

	void setup();
	void update();
	ci::gl::TextureCubeMapRef draw();

	std::vector<NetworkNode> mNetworkNodes;
	std::vector<std::pair<uint, uint>> mNetworkLinks;

	ci::gl::VboMeshRef mNodesMesh;
	ci::gl::VboMeshRef mLinksMesh;

	ci::gl::GlslProgRef mRenderLinesToCubeMap;
	ci::gl::GlslProgRef mRenderPointsToCubeMap;
	FboCubeMapLayeredRef mOutputCubeFbo;
	ci::gl::UboRef mMatrixBuffer;
};