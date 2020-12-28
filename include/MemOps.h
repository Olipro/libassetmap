#ifndef LIBASSETMAP_MEMOPS_H
#define LIBASSETMAP_MEMOPS_H

#include <cstdint>
#include <type_traits>

namespace AssetMap {
	using lam_size_t = LIBARCHIVEMAP_SIZE_TYPE;

	//! \brief       Writes any integral value, byte-by-byte to a memory location.
	//!
	//!              It is left to the compiler to determine whether the value can
	//!              be written with an unaligned access or not.
	//! \todo				 When the project is upgraded to C++ 20, alignment hinting
	//!						   be supported to allow using std::assume_aligned.
	//! \pre				 \c buf must be able to hold at least \c sizeof(T) bytes.
	//! \tparam T    The integral type to write (deduced)
	//! \param buf   The buffer to write the value into.
	//! \param value The integral value to write
	template <typename T>
	constexpr inline void PutValue(uint8_t* buf, T value) noexcept {
		static_assert(std::is_same_v<uint8_t, unsigned char>);
		static_assert(std::is_integral_v<T>, "Only integral values permitted");
		for (auto i = 0; i < sizeof(T); ++i)
			buf[i] = (value >> (8 * i)) & 0xFF;
	}

	//! \brief     Reads any integral value byte-by-byte and return it.
	//! \pre       \c buf must contain at least \c sizeof(T) bytes.
	//! \tparam T  Any integral type
	//! \param buf The buffer to read the value from.
	//! \return    The integral value read from the buffer.
	template <typename T>
	constexpr inline T GetValue(const uint8_t* buf) noexcept {
		static_assert(std::is_integral_v<T>, "Only integral values permitted");
		T ret = 0;
		for (auto i = 0; i < sizeof(T); ++i)
			ret |= buf[i] << (8 * i);
		return ret;
	}

	constexpr auto PutLamSizeT = PutValue<lam_size_t>;
	constexpr auto GetLamSizeT = GetValue<lam_size_t>;
} // namespace AssetMap

#endif // LIBASSETMAP_MEMOPS_H
