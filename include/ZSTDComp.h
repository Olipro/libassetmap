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

		ZSTD(both_t, float dictRatio = 0.01f);
		ZSTD(compress_t, float dictRatio = 0.01f);
		ZSTD(decompress_t);
		ZSTD(const ZSTD&) = delete;
		ZSTD(ZSTD&& rhs) noexcept;

		ZSTD& operator=(ZSTD&& rhs) noexcept;

		[[nodiscard]] size_t Compress(const uint8_t* src,
																	size_t srcLen,
																	uint8_t* dst,
																	size_t dstLen) const override;

		[[nodiscard]] size_t Decompress(const uint8_t* src,
																		size_t srcLen,
																		uint8_t* dst,
																		size_t dstLen) const override;

		[[nodiscard]] size_t CalcCompressSize(size_t len) const noexcept override;

		[[nodiscard]] size_t CalcDecompressSize(const uint8_t* src,
																						size_t len) const noexcept override;

		[[nodiscard]] bool
				CreateDictionary(std::filesystem::directory_entry samplesDir) override;

		void UseDictionary(const uint8_t* dict, size_t len) noexcept override;

		void UseDictionary(std::filesystem::directory_entry ent);

		void SetCompressLevel(int level) noexcept;

		void SetStrategyLevel(int level) noexcept;

		static int MinCompressLevel() noexcept;

		static int MaxCompressLevel() noexcept;

		static int DefaultCompressLevel() noexcept;

		static int MinStrategyLevel() noexcept;

		static int MaxStrategyLevel() noexcept;

		static const char* StrategyInfo() noexcept;

		[[nodiscard]] std::pair<const uint8_t*, size_t>
				Dictionary() const noexcept override;

		~ZSTD() noexcept override;
	};
} // namespace AssetMap

#endif // LIBASSETMAP_ZSTDCOMP_H
