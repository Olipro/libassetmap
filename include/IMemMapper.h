#ifndef LIBASSETMAP_IMEMMAPPER_H
#define LIBASSETMAP_IMEMMAPPER_H

#include <cstdint>
#include <cstdlib>

namespace AssetMap {
	class IMemMapper {
	public:
		//! \brief       Resize a mapped file.
		//! \param  size Desired size. Can be larger or smaller. Must be > 0
		//! \throws      std::runtime_error for any implementation-defined failure.
		//!              Typically errors in resizing or re-opening the memory
		//!              mapping.
		//! \post		     All pointers & references to mapped data are invalidated.
		//! 			       The memory mapped area will be writable after returning.
		//!				       If an exception is thrown, only the destructor is
		//!              guaranteed to work.
		//! \return A reference to *this
		virtual IMemMapper& Resize(size_t size) = 0;

		//! \brief Get the size of this mapping
		//! \return The size in bytes of the mapping
		[[nodiscard]] virtual size_t Size() const noexcept = 0;

		//! \brief Obtain a read-only pointer to the mapped data.
		//! \return A const pointer to the data guaranteed to be readable up to but
		//! 			  not including the address at Get() + Size()
		[[nodiscard]] virtual const uint8_t* Get() const noexcept = 0;

		//! \brief Obtain a writable pointer to the mapped data.
		//! \pre   An implementation-defined constructor that opens the mapping in
		//! 			 write mode or a call to Resize() must have occurred.
		//! \return A pointer to the data guaranteed to be readable/writable up to
		//! 			  but not including the address at Get() + Size()
		[[nodiscard]] virtual uint8_t* Get() noexcept = 0;

		virtual ~IMemMapper() noexcept = default;
	};
} // namespace AssetMap

#endif // LIBASSETMAP_IMEMMAPPER_H
