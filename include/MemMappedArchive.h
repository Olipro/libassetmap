#ifndef LIBASSETMAP_MEMMAPPEDARCHIVE_H
#define LIBASSETMAP_MEMMAPPEDARCHIVE_H

#include "ICompress.h"
#include "IDecompress.h"
#include "IHasher.h"
#include "IMemMapper.h"

#include "MemMappedBucket.h"
#include "MemMappedBucketEntry.h"
#include "MemOps.h"

#include <cstdint>
#include <filesystem>
#include <string_view>

namespace AssetMap {
	class MemMappedArchive {
		IMemMapper& file;
		const IHasher& hasher;
		const IDecompress* decomp = nullptr;

		template <class Comp>
		void LoadDictionary(Comp& comp) noexcept;

		class Iterator {
			const MemMappedArchive* archive;
			lam_size_t i;

		public:
			using difference_type		= ptrdiff_t;
			using value_type				= MemMappedBucket;
			using pointer						= MemMappedBucket*;
			using reference					= MemMappedBucket&;
			using iterator_category = std::forward_iterator_tag;

			Iterator(const MemMappedArchive& archive, lam_size_t i) noexcept;

			bool operator!=(const Iterator& rhs) const noexcept;

			MemMappedBucket operator->() noexcept;

			MemMappedBucket operator*() noexcept;

			Iterator& operator++() noexcept;
		};

	public:
		explicit MemMappedArchive(IMemMapper& file,
															IDecompress& comp,
															const IHasher& hasher);

		MemMappedArchive(const std::filesystem::directory_entry& ent,
										 const IHasher& hasher,
										 IMemMapper& file,
										 ICompress& comp,
										 IDecompress* decomp = nullptr);

		[[nodiscard]] lam_size_t BucketCount() const noexcept;

		[[nodiscard]] lam_size_t EmptyBuckets() const noexcept;

		[[nodiscard]] lam_size_t DictionarySize() const noexcept;

		MemMappedBucketEntry operator[](std::string_view name) const noexcept;

		MemMappedBucket operator[](lam_size_t idx) const noexcept;

		[[nodiscard]] Iterator begin() const noexcept;

		[[nodiscard]] Iterator end() const noexcept;
	};
} // namespace AssetMap

#endif // LIBASSETMAP_MEMMAPPEDARCHIVE_H
