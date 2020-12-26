#ifndef LIBASSETMAP_MEMMAPPER_H
#define LIBASSETMAP_MEMMAPPER_H

#include "IMemMapper.h"
#include "MemOps.h"

#include <filesystem>

namespace AssetMap {
	class FileDescriptor {
		void* fd;

	public:
		explicit FileDescriptor(void* fd);

		FileDescriptor(FileDescriptor&&) noexcept;

		FileDescriptor& operator=(FileDescriptor&&) noexcept;

		operator void*() const noexcept;

		~FileDescriptor() noexcept;
	};
	class MemMapper : public IMemMapper {
		FileDescriptor fd;
		uintmax_t len;
		void* fMap = nullptr;
		void* mMap = nullptr;

		void Close() noexcept;

	public:
		explicit MemMapper(const std::filesystem::directory_entry& file);
		MemMapper(const MemMapper&) = delete;
		MemMapper(MemMapper&& rhs) noexcept;

		MemMapper& operator=(MemMapper&& rhs) noexcept;

		[[nodiscard]] size_t Size() const noexcept override;

		IMemMapper& Resize(size_t size) override;

		[[nodiscard]] uint8_t* Get() noexcept override;

		~MemMapper() noexcept override;
	};
} // namespace AssetMap

#endif // LIBASSETMAP_MEMMAPPER_H
