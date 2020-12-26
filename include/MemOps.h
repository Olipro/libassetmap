#ifndef LIBASSETMAP_MEMOPS_H
#define LIBASSETMAP_MEMOPS_H

#include <cstdint>
#include <type_traits>

namespace AssetMap {
	using lam_size_t = LIBARCHIVEMAP_SIZE_TYPE;
	template <typename T>
	constexpr inline void PutValue(uint8_t* buf, T value) noexcept {
		static_assert(std::is_same_v<uint8_t, unsigned char>);
		for (auto i = 0; i < sizeof(T); ++i)
			buf[i] = (value >> (8 * i)) & 0xFF;
	}

	template <typename T>
	constexpr inline T GetValue(const uint8_t* buf) noexcept {
		T ret = 0;
		for (auto i = 0; i < sizeof(T); ++i)
			ret |= buf[i] << (8 * i);
		return ret;
	}

	constexpr auto PutLamSizeT = PutValue<lam_size_t>;
	constexpr auto GetLamSizeT = GetValue<lam_size_t>;
} // namespace AssetMap

#endif // LIBASSETMAP_MEMOPS_H
