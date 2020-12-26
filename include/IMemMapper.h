#ifndef LIBASSETMAP_IMEMMAPPER_H
#define LIBASSETMAP_IMEMMAPPER_H

#include <cstdint>
#include <cstdlib>

namespace AssetMap {
	class IMemMapper {
	public:
		virtual IMemMapper& Resize(size_t len)						 = 0;
		[[nodiscard]] virtual size_t Size() const noexcept = 0;
		[[nodiscard]] virtual uint8_t* Get() noexcept			 = 0;
		virtual ~IMemMapper() noexcept										 = default;
	};
} // namespace AssetMap

#endif // LIBASSETMAP_IMEMMAPPER_H
