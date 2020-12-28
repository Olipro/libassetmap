#ifndef LIBASSETMAP_MEMMAPPEDBUCKET_H
#define LIBASSETMAP_MEMMAPPEDBUCKET_H

#include <cstdint>

#include "MemMappedBucketEntry.h"
#include "MemOps.h"

namespace AssetMap {
	class MemMappedBucket {
		uint8_t* data;
		MemMappedBucketEntry next;
		ICompress* comp			= nullptr;
		IDecompress* decomp = nullptr;

		class Iterator {
			MemMappedBucketEntry entry;

		public:
			using difference_type		= ptrdiff_t;
			using value_type				= MemMappedBucketEntry;
			using pointer						= MemMappedBucketEntry*;
			using reference					= MemMappedBucketEntry&;
			using iterator_category = std::forward_iterator_tag;

			explicit Iterator(MemMappedBucketEntry&& entry);

			MemMappedBucketEntry& operator*() noexcept;

			MemMappedBucketEntry& operator->() noexcept;

			Iterator& operator++() noexcept;

			bool operator==(const Iterator& rhs) const noexcept;

			bool operator!=(const Iterator& rhs) const noexcept;
		};

	public:
		//! \brief            Constructs a bucket over existing data
		//! \post							Attempting to Append() new entries after using this
		//!										constructor results in undefined behaviour. The
		//!										IDecompress instance and data must live as long as
		//!									  this instance and any objects obtained through it.
		//! \param begin 			A pointer to the start of the Bucket
		//! \param bucketsTbl A pointer to the start of the bucket offset table
		//! \param id 				The valid index of the bucket in the buckets table.
		//! \param decomp 		An IDecompress instance to use for decompression.
		MemMappedBucket(uint8_t* begin,
										uint8_t* bucketsTbl,
										lam_size_t id,
										IDecompress& decomp) noexcept;

		//! \brief            Initialises an empty bucket at the given location.
		//! \post							The ICompress instance and data must live as long as
		//!									  this instance and any objects obtained through it.
		//! \param begin 			A pointer to the start of the Bucket
		//! \param bucketsTbl A pointer to the start of the bucket offset table
		//! \param id 				The valid index of the bucket in the buckets table.
		//! \param decomp 		An ICompress instance to use for decompression.
		MemMappedBucket(uint8_t* begin,
										uint8_t* bucketsTbl,
										ptrdiff_t offset,
										lam_size_t bucketId,
										ICompress& comp) noexcept;

		//! \brief  Returns an empty entry instance intended to be populated.
		//! \return An entry object with zero size and no name.
		[[nodiscard]] MemMappedBucketEntry Append() noexcept;

		//! \brief      Finds an entry in this bucket with the given name
		//! \post				If the entry does not exist, the implementation will return
		//!							either an instance explicitly convertible to \c bool
		//!             \c false or an entry with the wrong name. You must check for
		//!							both cases if you do not know whether the name exists.
		//! \param name The name of the entry to return.
		//! \return     Either the found entry or one of the post-condition cases.
		[[nodiscard]] MemMappedBucketEntry operator[](std::string_view name) const;

		//! \brief  Returns an iterator to the beginning of the bucket.
		//! \return an iterator to the beginning or end() if empty.
		[[nodiscard]] Iterator begin() const noexcept;

		//! \brief Returns an iterator to one-past-the-end as for C++ iterators.
		//! \return The one-past-the-end iterator.
		[[nodiscard]] Iterator end() const noexcept;
	};
} // namespace AssetMap
#endif // LIBASSETMAP_MEMMAPPEDBUCKET_H
