#ifndef LIBASSETMAP_HASHERS_H
#define LIBASSETMAP_HASHERS_H

#include "IHasher.h"

namespace AssetMap {
	class CityHash : public IHasher {
		float loadFactor;

	public:
		explicit CityHash(float loadFactor = 0.75f);
		[[nodiscard]] uint64_t Hash(std::string_view data) const noexcept override;
		[[nodiscard]] size_t CalcBucket(uint64_t hash,
																		size_t bucketCount) const noexcept override;
		[[nodiscard]] size_t
				CalcBucketsForItemCount(size_t count) const noexcept override;
	};
} // namespace AssetMap

#endif // LIBASSETMAP_HASHERS_H
