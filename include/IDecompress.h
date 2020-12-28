#ifndef LIBASSETMAP_IDECOMPRESS_H
#define LIBASSETMAP_IDECOMPRESS_H

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <utility>

namespace AssetMap {
	class IDecompress {
	public:
		//! \brief Decompress a previously compressed sequence of data.
		//!
		//! Typically, when decompressing data, a call to CalcDecompressSize() will
		//! first be made to determine the destination buffer space needed.
		//! \pre The data must have been compressed by a compatible ICompress
		//!      implementation. It is implementation-defined as to whether src and
		//!      \c dst may overlap and whether any size is permitted to be zero.
		//! \param src The buffer containing compressed data.
		//! \param srcLen The length of the source data.
		//! \param dst Destination buffer for the output data.
		//! \param dstLen Maximum number of bytes that can be written to \c dst
		//! \return The number of bytes written into \c dst
		[[nodiscard]] virtual size_t Decompress(const uint8_t* src,
																						size_t srcLen,
																						uint8_t* dst,
																						size_t dstLen) = 0;

		//! \brief Calculate the number of bytes required to decompress some data.
		//! \pre \c src must point to a sequence of data previously created by a
		//!      compatible ICompress implementation. If the input data was
		//!      compressed with a dictionary, \c UseDictionary() must have been
		//!      called.
		//! \param src A pointer to the compressed data
		//! \param len The length of the input data, in bytes.
		//! \return The size, in bytes, required to fully decompress the input.
		[[nodiscard]] virtual size_t
				CalcDecompressSize(const uint8_t* src, size_t len) const noexcept = 0;

		//! \brief  Obtains a pointer to the dictionary and its size, if loaded.
		//! \see    ICompress::Dictionary()
		//! \return a pair containing the pointer and its size. or nullptr & \<any\>
		[[nodiscard]] virtual std::pair<const uint8_t*, size_t>
				Dictionary() const noexcept = 0;

		//! \brief Loads a dictionary to be used for decompression.
		//! \pre   The dictionary must have been created with a compatible ICompress
		//!        implementation.
		//! \post  \c dict must remain valid until all calls to \c Decompress() have
		//!        completed and no further will be issued to this instance.
		//! \param dict The dictionary to load.
		//! \param len The size of the dictionary, in bytes.
		virtual void UseDictionary(const uint8_t* dict, size_t len) noexcept = 0;

		virtual ~IDecompress() noexcept = default;
	};
} // namespace AssetMap

#endif // LIBASSETMAP_IDECOMPRESS_H
