#ifndef LIBASSETMAP_ICOMPRESS_H
#define LIBASSETMAP_ICOMPRESS_H

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <utility>

namespace AssetMap {
	class ICompress {
	public:
		//! \brief Compresses a range of data into an output buffer.
		//!
		//! This function will compress a full sequence of data such that
		//! the output can be passed to an instance of \c IDecompress::Decompress
		//! and result in the same original input.
		//! \pre It is implementation-defined whether \c src and \c dst may overlap.
		//!      This is also true of passing any 0 lengths or a \c dstLen that is
		//!			 less than the result of \c CalcCompressSize. If dstLen is less than
		//!      \c CalcCompressSize() there is no guarantee the compressed result
		//!      contains the complete input data.
		//! \param src A pointer to data at least \c srcLen in length.
		//! \param srcLen The length of \c data to compress.
		//! \param dst A pointer to output
		//! \param dstLen
		//! \return The number of bytes written to the destination.
		[[nodiscard]] virtual size_t Compress(const uint8_t* src,
																					size_t srcLen,
																					uint8_t* dst,
																					size_t dstLen) = 0;

		//! \brief Calculates the worst-case size (in bytes) required to compress
		//!        data of length \c len
		//! \param len The size (in bytes) to compress.
		//! \return The worst-case amount of space needed as a dest buffer to
		//! 				Compress()
		[[nodiscard]] virtual size_t
				CalcCompressSize(size_t len) const noexcept = 0;

		//! \brief Creates a dictionary from a valid, readable directory, if it can.
		//!
		//! The directory is iterated recursively. How the dictionary is created is
		//! implementation defined. Typically a constructor will have been told a
		//! desired dictionary size.
		//! It is not an error for a dictionary to fail creation. This simply means
		//! the provided samples are not suitable for generating a dictionary.
		//! \param samplesDir A valid directory entry to use as samples.
		//! \return a \c bool indicating whether dictionary creation succeeded.
		[[nodiscard]] virtual bool
				CreateDictionary(std::filesystem::directory_entry samplesDir) = 0;

		//! \brief Obtains the dictionary pointer and size.
		//! \pre \c CreateDictionary() must have been called and returned true.
		//! \post The pointer is guaranteed to remain valid until another call to
		//!       CreateDictionary() or destruction of the instance.
		//! \return a pair consisting of a pointer to the data and its length.
		[[nodiscard]] virtual std::pair<const uint8_t*, size_t>
				Dictionary() const noexcept = 0;

		//! \brief References a dictionary to use for compression.
		//! \pre \c dict must point to a valid dictionary of size \c len previously
		//!      created by an instance of the same or compatible implementation.
		//! \post \c dict must remain valid until all calls to \c Compress() have
		//!       completed and no further calls are made.
		//! \param dict A pointer to the dictionary.
		//! \param len
		virtual void UseDictionary(const uint8_t* dict, size_t len) noexcept = 0;

		virtual ~ICompress() noexcept = default;
	};
} // namespace AssetMap

#endif // LIBASSETMAP_ICOMPRESS_H
