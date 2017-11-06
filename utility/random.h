#pragma once
#include <cstdint>
#include <random>

namespace pug {
namespace utility {

	struct RandomState
	{
		std::mt19937 mersenneTwister;
		std::uniform_real_distribution<float> floatDistribution;
		std::uniform_int_distribution<int> integerDistribution;
	};

	RandomState CreateRandomState();
	RandomState CreateRandomState(uint32_t seed);

	float RandomFloat( RandomState& randomState, float min = -1.0f, float max = 1.0f);
	int32_t RandomInt(RandomState& randomState, int32_t min = -1, int32_t max = 1);

	void Seed(RandomState& randomState, uint32_t seed);

}//pug::utility
}//pug