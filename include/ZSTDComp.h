#ifndef LIBASSETMAP_ZSTDCOMP_H
#define LIBASSETMAP_ZSTDCOMP_H

#include "ICompress.h"
#include "IDecompress.h"

#include <vector>

using ZSTD_CCtx	 = struct ZSTD_CCtx_s;
using ZSTD_DCtx	 = struct ZSTD_DCtx_s;
using ZSTD_CDict = struct ZSTD_CDict_s;
using ZSTD_DDict = struct ZSTD_DDict_s;

namespace AssetMap {

	//! \private
	template <class Ctx>
	class ZCtx {
		Ctx* ctx = nullptr;

		void Dispose();

	public:
		constexpr ZCtx() = default;
		constexpr ZCtx(ZCtx&&) noexcept;
		constexpr ZCtx& operator=(ZCtx&&) noexcept;
		constexpr ZCtx& operator=(Ctx* ctx) noexcept;
		constexpr explicit operator bool() const noexcept;
		operator Ctx*() const noexcept;
		constexpr void Create();
		~ZCtx();
	};

	class ZSTD : public ICompress, public IDecompress {
		ZCtx<ZSTD_CCtx> cCtx;
		ZCtx<ZSTD_DCtx> dCtx;
		const uint8_t* dictionary = nullptr;
		size_t dictLen						= 0;
		int compressionLevel			= 0;
		float dictRatio						= 0.01f;
		std::vector<uint8_t> generatedDictionary;

		struct compress_t {};
		struct decompress_t {};
		struct both_t {};

		ZSTD(float dictRatio) noexcept;

	public:
		static constexpr compress_t compress{};
		static constexpr decompress_t decompress{};
		static constexpr both_t both{};

		//! \brief           Construct an instance of ZSTD for compression and
		//! 							   decompression.
		//! \param dictRatio optionally specifies the desired dictionary size.
		//! 								 defaults to 1% (0.01)
		explicit ZSTD(both_t, float dictRatio = 0.01f);

		//! \brief           Construct an instance of ZSTD for compression.
		//! \post						 Attempting to decompress data results in undefined
		//!                  behaviour.
		//! \param dictRatio optionally specifies the desired dictionary size.
		//! 								 defaults to 1% (0.01)
		explicit ZSTD(compress_t, float dictRatio = 0.01f);

		//! \brief           Construct an instance of ZSTD for decompression.
		//! \post						 Attempting to compress data results in undefined
		//!                  behaviour.
		//! \param dictRatio optionally specifies the desired dictionary size.
		//! 								 defaults to 1% (0.01)
		explicit ZSTD(decompress_t);

		ZSTD(const ZSTD&) = delete;

		ZSTD(ZSTD&& rhs) noexcept;

		ZSTD& operator=(ZSTD&& rhs) noexcept;

		//! \brief        Compresses data from src into dst.
		//! \param src    The source buffer to compress data from.
		//! \param srcLen The length in bytes to compress.
		//! \param dst 		The destination buffer to place compressed data in.
		//! \param dstLen The total space available. Should be at least
		//! 							\c CalcCompressSize()
		//! \return 			The number of bytes written into the destination.
		[[nodiscard]] size_t Compress(const uint8_t* src,
																	size_t srcLen,
																	uint8_t* dst,
																	size_t dstLen) override;

		//! \brief        Decompresses data from src into dst.
		//! \pre					If the data was compressed with a dictionary, it must have
		//!               been loaded.
		//! \param src    The source buffer to decompress data from.
		//! \param srcLen The length in bytes to decompress.
		//! \param dst 		The destination buffer to place decompressed data in.
		//! \param dstLen The total space available. Ideally at least
		//! 							\c CalcDecompressSize()
		//! \return 			The number of bytes written into the destination.
		[[nodiscard]] size_t Decompress(const uint8_t* src,
																		size_t srcLen,
																		uint8_t* dst,
																		size_t dstLen) override;

		//! \brief     Calculates the worst-case size needed to compress the data.
		//! \param len The size of the input data
		//! \return 	 The worst-case space requirement to compress the data.
		[[nodiscard]] size_t CalcCompressSize(size_t len) const noexcept override;

		//! \brief		 Calculates the number of bytes needed to decompress the data.
		//! \pre			 If a dictionary was used to compress the data, it must have
		//!            been loaded.
		//! \param src A pointer to the input data.
		//! \param len the size (in bytes) of the input data.
		//! \return The number of bytes required to fully decompress the data.
		[[nodiscard]] size_t CalcDecompressSize(const uint8_t* src,
																						size_t len) const noexcept override;

		//! \brief            Creates a dictionary from a directory of samples
		//!
		//! A non-trivial amount of memory is typically required to generate a
		//! dictionary. All samples are copied into memory.
		//! \param samplesDir A valid directory that will be iterated recursively.
		//! \return           Whether or not a dictionary was created. It is not an
		//!                   error if dictionary creation fails; dive into zdict's
		//!                   implementation if you feel it shouldn't have failed.
		[[nodiscard]] bool
				CreateDictionary(std::filesystem::directory_entry samplesDir) override;

		//! \brief      References a dictionary for (de)compression purposes.
		//! \post       The dictionary must remain valid for the lifetime of this
		//!             instance.
		//! \param dict A buffer containing the dictionary to load.
		//! \param len  The size of the dictionary (in bytes)
		void UseDictionary(const uint8_t* dict, size_t len) noexcept override;

		//! \brief		 Reads a file into memory and uses it as a dictionary.
		//!
		//! This function mainly exists for convenience; it is recommended to mmap
		//! the dictionary and use the non-owning overload.
		//! \param ent A valid, regular file to read the dictionary from.
		void UseDictionary(std::filesystem::directory_entry ent);

		//! \brief 			 Sets the desired compression level.
		//! \pre				 If a dictionary has been loaded, has no effect. Otherwise,
		//!              all future compression and dictionary creation uses this.
		//! \param level The desired compression level. Must be in the range
		//!              \c MinCompressionLevel() \<= level \<= \c
		//!              MaxCompressLevel()
		void SetCompressLevel(int level) noexcept;

		//!              Sets the desired compression strategy.
		//! \param level the strategy to use. Must be in the range
		//! 						 \c MinStrategyLevel() \<= level \<= MaxStrategyLevel()
		void SetStrategyLevel(int level) noexcept;

		//! \return The minimum compression level. Usually a large negative value.
		static int MinCompressLevel() noexcept;

		//! \return The maximum compression level.
		static int MaxCompressLevel() noexcept;

		//! \return The default library's compression level.
		static int DefaultCompressLevel() noexcept;

		//! \return The minimum strategy level.
		static int MinStrategyLevel() noexcept;

		//! \return The maximum strategy level
		static int MaxStrategyLevel() noexcept;

		//! \return a human-readable string containing all strategies
		static const char* StrategyInfo() noexcept;

		//! \return a pair containing a pointer to the dictionary and its size.
		//!         the pointer will be nullptr if no dictionary was loaded.
		[[nodiscard]] std::pair<const uint8_t*, size_t>
				Dictionary() const noexcept override;

		~ZSTD() noexcept override;
	};
} // namespace AssetMap

#endif // LIBASSETMAP_ZSTDCOMP_H
