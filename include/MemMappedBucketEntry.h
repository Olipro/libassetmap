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
		uint8_t* data				= nullptr;
		ICompress* comp			= nullptr;
		IDecompress* decomp = nullptr;

		void Name(std::string_view name) noexcept;

		[[nodiscard]] uint8_t* FileData() noexcept;

		[[nodiscard]] const uint8_t* FileData() const noexcept;

		void FileSize(lam_size_t size);

	public:
		//! \brief      Constructor to reference available space for writing.
		//! \param data A pointer to the location where entry data is to be written.
		//! \param comp A valid instance of a compressor to compress the data.
		MemMappedBucketEntry(uint8_t* data, ICompress& comp);

		//! \brief			  Constructor to reference an existing entry for reading.
		//! \param data   A pointer to the location where an entry (may) exist.
		//! \param decomp A valid instance of a decompressor to extract the data.
		MemMappedBucketEntry(uint8_t* data, IDecompress& decomp);

		//! \brief Constructs an instance not pointing to any data.
		//! \post  Calling anything other than the comparison operators results in
		//!        undefined behaviour.
		//! \param nullptr_t requires you to pass \c nullptr - nothing else is valid
		explicit MemMappedBucketEntry(std::nullptr_t);

		//! \brief Obtains the (compressed) size of this entry's file.
		//! \return the size of this entry's file.
		[[nodiscard]] lam_size_t FileSize() const noexcept;

		//! \brief Obtains the size that would be required to fully decompress.
		//! \return the number of bytes needed to fully decompress the data.
		[[nodiscard]] lam_size_t DecompressedSize() const noexcept;

		//! Obtains the full size (in bytes) of this entry, including padding.
		//! \return the full size of this entry. Generally used to calculate the
		//!         appropriate address to store the next entry.
		[[nodiscard]] size_t InMemorySize() const noexcept;

		//! \brief Obtains the name of this entry
		//! \return the name of this entry.
		[[nodiscard]] std::string_view Name() const noexcept;

		//! \brief      Initialises this entry by compressing and writing the input
		//!             data to \c ptr
		//! \pre				\c name must not be empty.
		//! \param name The name of the file
		//! \param ptr  The source data to compress and write to the file
		//! \param len  The length of the source data (in bytes)
		//! \return     The total in-memory size of this entire entry.
		//! \see				InMemorySize()
		[[nodiscard]] size_t Populate(std::string_view name,
																	const uint8_t* ptr,
																	size_t len) noexcept;

		//! \brief  Initializes this entry to a size of zero and an empty name
		//! \return The total in-memory size of this entire entry.
		[[nodiscard]] size_t MakeNull() noexcept;

		//! \brief  Allocates and returns the decompressed data and its size.
		//! \return a pair containing the data and its size.
		[[nodiscard]] std::pair<std::unique_ptr<uint8_t[]>, size_t> Retrieve();

		//! \brief Decompresses the data into \c buf up to a limit of \c len
		//! \return the number of bytes written to \c buf
		[[nodiscard]] size_t Retrieve(uint8_t* buf, size_t len);

		//! \brief  Increments this entry to point to the next entry space.
		//! \pre    This instance must currently have a valid name and size.
		//! \return a reference to *this.
		MemMappedBucketEntry& operator++() noexcept;

		//! \brief     compares this instance to another instance for equality
		//!
		//! Elements compare equal if they point to the same location.
		//! \param rhs any instance.
		//! \return    true if the instances are equivalent.
		[[nodiscard]] bool
				operator==(const MemMappedBucketEntry& rhs) const noexcept;

		//! \brief		 compares this instance to another instance for inequality
		//!
		//!	Elements compare inequal if operator== would return false.
		//! \param rhs any instance.
		//! \return 	 true if the instance are not equivalent.
		[[nodiscard]] bool
				operator!=(const MemMappedBucketEntry& rhs) const noexcept;

		//! \brief	Tests if this entry is valid.
		//!
		//! Returns true if this entry points to a valid address and has a
		//! non-empty name.
		//! \return whether or not the instance is valid.
		explicit operator bool() const noexcept;
	};
} // namespace AssetMap
#endif // LIBASSETMAP_MEMMAPPEDBUCKETENTRY_H
