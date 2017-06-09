#pragma once

#include <vector>
#include <unordered_set>
#include <utility>
#include <algorithm>
#include <numeric>

#include "cinder/Rand.h"

#include "CoreMath.h"

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
	void disrupt(ci::vec3 dir);

	void setColorAttribs();

	int const mNumNetworkNodes = 2000;
	int const mNumLinksPerNode = 8;
	float const mNodeDisinfectChance = 0.04;
	float const mSpreadInfectionChance = 0.007;
	int const mMinInfected = 20;

	std::vector<NetworkNode> mNetworkNodes;
	std::vector<std::pair<uint, uint>> mNetworkLinks;

	ci::gl::VboMeshRef mNodesMesh;
	ci::gl::VboMeshRef mLinksMesh;

	ci::gl::GlslProgRef mRenderToCubeMap;
	ci::gl::FboCubeMapRef mOutputCubeFbo;
};