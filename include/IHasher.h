#ifndef LIBASSETMAP_IHASHER_H
#define LIBASSETMAP_IHASHER_H

#include <cstdint>
#include <string_view>

namespace AssetMap {
	class IHasher {
	public:
		//! \brief Computes an unsigned 64-bit value of an arbitrary string of
		//! bytes. \pre   It is implementation defined whether \c data can be empty.
		//! \param data a sequence of bytes to hash.
		//! \return a 64-bit hash of \c data
		[[nodiscard]] virtual uint64_t
				Hash(std::string_view data) const noexcept = 0;

		//! \brief Calculates the bucket index from a given hash and bucket count
		//! \param hash a hash previously computed with \c Hash()
		//! \param bucketCount The total number of buckets that exist.
		//! \return The bucket that this entry should be found/placed in.
		[[nodiscard]] virtual size_t
				CalcBucket(uint64_t hash, size_t bucketCount) const noexcept = 0;

		//! \brief Computes the preferred number of buckets the implementation wants
		//!        for a given number of entries that will be inserted.
		//! \param count The total number of items that will be stored.
		//! \return The number of buckets that are preferred for \c count input
		//!         items.
		[[nodiscard]] virtual size_t
				CalcBucketsForItemCount(size_t count) const noexcept = 0;

		virtual ~IHasher() noexcept = default;
	};
} // namespace AssetMap

#endif // LIBASSETMAP_IHASHER_H
