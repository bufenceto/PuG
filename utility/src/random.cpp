#include "random.h"

#define INT_RESOLUTION 1024

pug::utility::RandomState pug::utility::CreateRandomState()
{
	RandomState rs;
	rs.mersenneTwister = std::mt19937();
	rs.floatDistribution = std::uniform_real_distribution<float>(0.0f, 1.0f);
	rs.integerDistribution = std::uniform_int_distribution<int>(0, INT_RESOLUTION);
	return rs;
}

pug::utility::RandomState pug::utility::CreateRandomState(uint32_t seed)
{
	RandomState rs;
	rs.mersenneTwister = std::mt19937(seed);
	rs.floatDistribution = std::uniform_real_distribution<float>(0.0f, 1.0f);
	rs.integerDistribution = std::uniform_int_distribution<int>(0, INT_RESOLUTION);
	return rs;
}

float pug::utility::RandomFloat(RandomState& randomState, float min /* = -1.0f */, float max /* = 1.0f */)
{
	float value = randomState.floatDistribution(randomState.mersenneTwister);
	return min + value * (max - min);
}

int32_t pug::utility::RandomInt(RandomState& randomState, int32_t min /* = -1 */, int32_t max /* = 1 */)
{
	int32_t value = randomState.integerDistribution(randomState.mersenneTwister);
	return (min + value * (max - min)) >> 10;//shift should be equal to log(INT_RESOLUTION) / log(2)
}

void pug::utility::Seed(RandomState& randomState, uint32_t seed)
{
	randomState.mersenneTwister.seed(seed);
}