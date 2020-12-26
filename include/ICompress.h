#ifndef LIBASSETMAP_ICOMPRESS_H
#define LIBASSETMAP_ICOMPRESS_H

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <utility>

namespace AssetMap {
	class ICompress {
	public:
		[[nodiscard]] virtual size_t Compress(const uint8_t* src,
																					size_t srcLen,
																					uint8_t* dst,
																					size_t dstLen) const = 0;
		[[nodiscard]] virtual size_t
				CalcCompressSize(size_t len) const noexcept = 0;
		[[nodiscard]] virtual bool
				CreateDictionary(std::filesystem::directory_entry samplesDir) = 0;
		[[nodiscard]] virtual std::pair<const uint8_t*, size_t>
				Dictionary() const noexcept																			 = 0;
		virtual void UseDictionary(const uint8_t* dict, size_t len) noexcept = 0;
		virtual ~ICompress() noexcept = default;
	};
} // namespace AssetMap

#endif // LIBASSETMAP_ICOMPRESS_H
