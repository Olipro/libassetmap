#include "Hashers.h"
#include "city.h"

#include <cmath>

using namespace AssetMap;

CityHash::CityHash(float bucketRatio) : bucketRatio{bucketRatio} {}

uint64_t CityHash::Hash(std::string_view data) const noexcept {
	return CityHash64(data.data(), data.size());
}

size_t CityHash::CalcBucket(uint64_t hash, size_t bucketCount) const noexcept {
	double val = hash, dLen = bucketCount;
	val = val / std::pow(2., 64.);
	return std::min(std::round(val * dLen), dLen - 1);
}

[[nodiscard]] size_t
		CityHash::CalcBucketsForItemCount(size_t count) const noexcept {
	return std::max(1.f, count / bucketRatio);
}