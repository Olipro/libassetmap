#ifndef LIBASSETMAP_MEMMAPPER_H
#define LIBASSETMAP_MEMMAPPER_H

#include "IMemMapper.h"
#include "MemOps.h"

#include <filesystem>

namespace AssetMap {
	//! \private
	class FileDescriptor {
		int fd;

	public:
		explicit FileDescriptor(int fd);

		FileDescriptor(FileDescriptor&&) noexcept;

		FileDescriptor& operator=(FileDescriptor&&) noexcept;

		operator int() const noexcept;

		~FileDescriptor() noexcept;
	};
	class MemMapper : public IMemMapper {
		FileDescriptor fd;
		uintmax_t len;
		void* mMap = nullptr;

		void Close() noexcept;

	public:
		//! \brief      Constructs a MemMapper from a file path.
		//! \pre				If creating a file, the directory hierarchy must exist, it
		//!							will not create it for you.
		//! \post       If the file exists and has a length > 0, it will be opened
		//!             and mapped. Otherwise, it will be created/opened, but not
		//!             mapped until a call to Resize() occurs. Even if mapped, the
		//!             data is never writable after construction.
		//! \param file A valid path to an existing file, or location to create one.
		explicit MemMapper(const std::filesystem::directory_entry& file);

		MemMapper(const MemMapper&) = delete;

		MemMapper(MemMapper&& rhs) noexcept;

		MemMapper& operator=(MemMapper&& rhs) noexcept;

		//! \brief Obtains the size of the file.
		//! \return The file size (in bytes) - 0 if empty.
		[[nodiscard]] size_t Size() const noexcept override;

		//! \brief      Resizes the file; semantics are always the same as
		//! 					  ftruncate()
		//! \post       All existing pointers and references to the memory-mapped
		//!             area are invalidated. The data will become writable.
		//! \param size The size (in bytes) to resize the file to.
		//! \return 		A reference to *this.
		IMemMapper& Resize(size_t size) override;

		//! \brief  Obtains a \c const pointer to the memory-mapped file.
		//! \post   If the instance was constructed from an empty or non-existent
		//!         file, de-referencing the returned pointer causes
		//! 				undefined behaviour. Debug builds have an assert guard.
		//! \return the beginning of the memory-mapped data..
		[[nodiscard]] const uint8_t* Get() const noexcept override;

		//! \brief  Obtains a pointer to the memory-mapped file.
		//! \post   If the instance was constructed from an empty or non-existent
		//!         file, de-referencing the returned pointer causes
		//! 				undefined behaviour. Debug builds have an assert guard.
		//! \return the beginning of the memory-mapped data.
		[[nodiscard]] uint8_t* Get() noexcept override;

		~MemMapper() noexcept override;
	};
} // namespace AssetMap

#endif // LIBASSETMAP_MEMMAPPER_H
