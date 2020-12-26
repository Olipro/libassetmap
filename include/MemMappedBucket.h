#ifndef LIBASSETMAP_MEMMAPPEDBUCKET_H
#define LIBASSETMAP_MEMMAPPEDBUCKET_H

#include <cstdint>

#include "MemMappedBucketEntry.h"
#include "MemOps.h"

namespace AssetMap {
	class MemMappedBucket {
		uint8_t* data;
		MemMappedBucketEntry next;
		const ICompress* comp			= nullptr;
		const IDecompress* decomp = nullptr;

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
		MemMappedBucket(uint8_t* begin,
										uint8_t* bucketsTbl,
										lam_size_t id,
										const IDecompress& decomp) noexcept;

		MemMappedBucket(uint8_t* begin,
										uint8_t* bucketsTbl,
										ptrdiff_t offset,
										lam_size_t bucketId,
										const ICompress& comp) noexcept;

		[[nodiscard]] MemMappedBucketEntry Append() noexcept;

		[[nodiscard]] MemMappedBucketEntry operator[](std::string_view name) const;

		[[nodiscard]] Iterator begin() const noexcept;

		[[nodiscard]] Iterator end() const noexcept;
	};
} // namespace AssetMap
#endif // LIBASSETMAP_MEMMAPPEDBUCKET_H
