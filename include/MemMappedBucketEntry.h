#ifndef LIBASSETMAP_MEMMAPPEDBUCKETENTRY_H
#define LIBASSETMAP_MEMMAPPEDBUCKETENTRY_H

#include "ICompress.h"
#include "IDecompress.h"

#include "MemOps.h"

#include <cstdint>
#include <memory>
#include <string_view>
#include <utility>

namespace AssetMap {
	class MemMappedBucketEntry {
		uint8_t* data							= nullptr;
		const ICompress* comp			= nullptr;
		const IDecompress* decomp = nullptr;

		void Name(std::string_view name) noexcept;

		[[nodiscard]] uint8_t* FileData() noexcept;

		[[nodiscard]] const uint8_t* FileData() const noexcept;

		void FileSize(lam_size_t size);

	public:
		MemMappedBucketEntry(uint8_t* data, const ICompress& comp);
		MemMappedBucketEntry(uint8_t* data, const IDecompress& decomp);

		explicit MemMappedBucketEntry(std::nullptr_t);

		[[nodiscard]] lam_size_t FileSize() const noexcept;

		[[nodiscard]] lam_size_t DecompressedSize() const noexcept;

		[[nodiscard]] size_t InMemorySize() const noexcept;

		[[nodiscard]] std::string_view Name() const noexcept;

		[[nodiscard]] size_t Populate(std::string_view name,
																	const uint8_t* ptr,
																	size_t len) noexcept;

		[[nodiscard]] size_t MakeNull() noexcept;

		[[nodiscard]] std::pair<std::unique_ptr<uint8_t[]>, size_t>
				Retrieve() const;

		[[nodiscard]] size_t Retrieve(uint8_t* buf, size_t len) const;

		MemMappedBucketEntry& operator++() noexcept;

		[[nodiscard]] bool
				operator==(const MemMappedBucketEntry& rhs) const noexcept;

		[[nodiscard]] bool
				operator!=(const MemMappedBucketEntry& rhs) const noexcept;

		explicit operator bool() const noexcept;
	};
} // namespace AssetMap
#endif // LIBASSETMAP_MEMMAPPEDBUCKETENTRY_H
