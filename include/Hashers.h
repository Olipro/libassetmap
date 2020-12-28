#ifndef LIBASSETMAP_HASHERS_H
#define LIBASSETMAP_HASHERS_H

#include "IHasher.h"

namespace AssetMap {
	class CityHash : public IHasher {
		float bucketRatio;

	public:
		//! \brief Constructs an instance of the CityHash class.
		//!
		//! This class does not allocate anything. An optional bucket ratio can be
		//! specified which is used by \c CalcBucketsForItemCount.
		//! \param bucketRatio Specifies the desired ratio of items:buckets.
		explicit CityHash(float bucketRatio = 0.75f);

		//! \brief Compute the hash of a given string of bytes.
		//! \param data A string of data to digest.
		//! \return A 64-bit hash computed by CityHash.
		[[nodiscard]] uint64_t Hash(std::string_view data) const noexcept override;

		//! \brief Calculates the bucket to index given a hash and total bucket
		//! size.
		//!
		//! The exact method of calculation uses floating point operations as this
		//! produced mildly better distribution of buckets.
		//! \param hash A hash previously obtained through a call to CityHash::Hash
		//! \param bucketCount The total number of buckets that exist.
		//! \return A value in the range 0 to \c bucketCount-1
		[[nodiscard]] size_t CalcBucket(uint64_t hash,
																		size_t bucketCount) const noexcept override;

		//! \brief Calculates the total number of buckets that should be created for
		//!				 total file \c count.
		//!
		//! This implementation simply takes \c count and divides it by \c
		//! bucketRatio
		//! \param count The total number of entries that will be hashed.
		//! \return A value indicating the preferred number of buckets.
		[[nodiscard]] size_t
				CalcBucketsForItemCount(size_t count) const noexcept override;
	};
} // namespace AssetMap

#endif // LIBASSETMAP_HASHERS_H
