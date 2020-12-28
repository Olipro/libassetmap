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
		IDecompress* decomp = nullptr;

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
		//! \brief        Constructs a MemMappedArchive for reading from an archive
		//!	You should construct a separate instance of this class
		//! along with a separate IDecompress for each thread.
		//! \post         Anything that causes a change to the archive may result in
		//!               undefined behaviour. This should usually fail at the point
		//!               of trying to write to the IMemMapper as it should have
		//!               been opened in read-only mode.
		//! \param file   an instance of an existing, readable IMemMapper instance
		//!               consisting of an entire complete archive.
		//! \param comp   an instance of the compatible decompressor for the
		//!               implementation used to compress the instance
		//! \param hasher an instance of the hash implementation used to create the
		//!               archive.
		explicit MemMappedArchive(IMemMapper& file,
															IDecompress& comp,
															const IHasher& hasher);

		//! \brief				Constructs an instance for creating (optionally, reading)
		//! 							an archive.
		//! \post					If no decompressor or a decompressor incompatible with the
		//!               compressor is provided, the behaviour of any archive read
		//!               functions is undefined. All interface refs must remain
		//!               valid for the duration of this instance's lifetime and any
		//!               objects retrieved through this instance.
		//!               \c file will be altered. Existing data will be destroyed.
		//! \param ent 		A directory which will be recursed to create the archive.
		//! \param hasher A hasher which will be used to determine bucketing.
		//! \param file 	An IMemMapper which can (and will) be resized.
		//! \param comp 	An implementation used to compress files.
		//! \param decomp An optional decompressor should you wish to immediately
		//! 						  read data back.
		MemMappedArchive(const std::filesystem::directory_entry& ent,
										 const IHasher& hasher,
										 IMemMapper& file,
										 ICompress& comp,
										 IDecompress* decomp = nullptr);

		//! \brief Obtains the total number of buckets in the archive.
		//! \return The number of buckets in the archive.
		[[nodiscard]] lam_size_t BucketCount() const noexcept;

		//! \brief Obtains the number of empty (unused) buckets in the archive.
		//! \return the number of empty (unused) buckets.
		[[nodiscard]] lam_size_t EmptyBuckets() const noexcept;

		//! \brief  Obtains the size (in bytes) of the ICompressor's dictionary
		//! \return The size of the dictionary or 0 if no dictionary is in use.
		//!         A 0-size dictionary should never be present, but it will also
		//!         return 0 in such a situation.
		[[nodiscard]] lam_size_t DictionarySize() const noexcept;

		//! \brief      Obtains the entry matching the specified name
		//! \pre        the instance must have been constructed with a valid and
		//!             compatible IDecompress, IHasher and IMemMapper.
		//! \post				If the entry does not exist, the implementation may return
		//!							either an object explicitly convertible to \c bool false or
		//!             a valid object with a non-matching name. You must test both
		//!             cases if you do not know in advance that the entry exists.
		//! \param name The name of the item to retrieve
		//! \return     a MemMappedBucketEntry object.
		MemMappedBucketEntry operator[](std::string_view name) const noexcept;

		//! \brief		 Obtains the bucket for the given index.
		//! \pre			 \c idx must be in the range 0 <= \c idx < BucketCount(). If
		//!            there are no buckets, the behaviour is undefined.
		//! \param idx The bucket index to retrieve.
		//! \return    A bucket object
		MemMappedBucket operator[](lam_size_t idx) const noexcept;

		//! \brief  Obtains an iterator to the first bucket in the archive.
		//! \return Iterator to the first bucket. Will compare equal to end() if no
		//!         buckets are present.
		[[nodiscard]] Iterator begin() const noexcept;

		//! \brief  Obtains an iterator to one-past-the-end as for C++ iterators.
		//! \return The one-past-the-end iterator.
		[[nodiscard]] Iterator end() const noexcept;
	};
} // namespace AssetMap

#endif // LIBASSETMAP_MEMMAPPEDARCHIVE_H
