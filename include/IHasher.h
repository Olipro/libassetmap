#ifndef LIBASSETMAP_IHASHER_H
#define LIBASSETMAP_IHASHER_H

#include <cstdint>
#include <string_view>

class IHasher {
public:
	[[nodiscard]] virtual uint64_t Hash(std::string_view data) const noexcept = 0;
	[[nodiscard]] virtual size_t
			CalcBucket(uint64_t hash, size_t bucketCount) const noexcept = 0;
	[[nodiscard]] virtual size_t
			CalcBucketsForItemCount(size_t count) const noexcept = 0;
	virtual ~IHasher() noexcept															 = default;
};

#endif // LIBASSETMAP_IHASHER_H
