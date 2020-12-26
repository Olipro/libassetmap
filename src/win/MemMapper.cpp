#include "win/MemMapper.h"

#include <cassert>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

using namespace AssetMap;
using namespace std::string_literals;

namespace fs = std::filesystem;

[[nodiscard]] static void* OpenFile(const fs::directory_entry& file) {
	void* fd;
	if (file.exists())
		fd = CreateFileW(file.path().c_str(),
										 GENERIC_READ,
										 FILE_SHARE_READ,
										 nullptr,
										 OPEN_EXISTING,
										 FILE_ATTRIBUTE_NORMAL,
										 nullptr);
	else
		fd = CreateFileW(file.path().c_str(),
										 GENERIC_READ | GENERIC_WRITE,
										 FILE_SHARE_READ,
										 nullptr,
										 CREATE_NEW,
										 FILE_ATTRIBUTE_NORMAL,
										 nullptr);
	if (fd == INVALID_HANDLE_VALUE)
		throw std::runtime_error{"Unable to open "s +
														 file.path().generic_u8string()};
	return fd;
}

FileDescriptor::FileDescriptor(void* fd) : fd{fd} {}

FileDescriptor::FileDescriptor(FileDescriptor&& rhs) noexcept : fd{rhs.fd} {
	rhs.fd = INVALID_HANDLE_VALUE;
}

FileDescriptor& FileDescriptor::operator=(FileDescriptor&& rhs) noexcept {
	if (fd != INVALID_HANDLE_VALUE)
		CloseHandle(fd);
	fd		 = rhs.fd;
	rhs.fd = INVALID_HANDLE_VALUE;
	return *this;
}

FileDescriptor::operator void*() const noexcept {
	return fd;
}

FileDescriptor::~FileDescriptor() noexcept {
	if (fd != INVALID_HANDLE_VALUE)
		CloseHandle(fd);
}

MemMapper::MemMapper(const fs::directory_entry& file) :
		fd{OpenFile(file)}, len{file.exists() ? file.file_size() : 0} {
	if (len > 0) {
		if (fMap = CreateFileMappingW(fd, nullptr, PAGE_READONLY, 0, 0, nullptr);
				fMap == nullptr)
			throw std::runtime_error{"Unable to mmap "s +
															 file.path().generic_u8string()};
		if (mMap = MapViewOfFileEx(fMap,
															 FILE_MAP_READ,
															 0,
															 0,
															 file.file_size(),
															 nullptr);
				mMap == nullptr)
			throw std::runtime_error{"Unable to map view of "s +
															 file.path().generic_u8string()};
	}
}

MemMapper::MemMapper(MemMapper&& rhs) noexcept :
		fd{std::move(rhs.fd)}, len{rhs.len}, fMap{rhs.fMap}, mMap{rhs.mMap} {
	rhs.fMap = nullptr;
	rhs.mMap = nullptr;
}

MemMapper& MemMapper::operator=(MemMapper&& rhs) noexcept {
	Close();
	fd			 = std::move(rhs.fd);
	len			 = rhs.len;
	fMap		 = rhs.fMap;
	mMap		 = rhs.mMap;
	rhs.fMap = nullptr;
	rhs.mMap = nullptr;
	return *this;
}

size_t MemMapper::Size() const noexcept {
	return len;
}

IMemMapper& MemMapper::Resize(size_t size) {
	if (mMap)
		UnmapViewOfFile(mMap);
	if (fMap)
		CloseHandle(fMap);
	mMap = fMap = nullptr;
	LARGE_INTEGER lSize;
	lSize.QuadPart = size;
	if (!SetFilePointerEx(fd, lSize, nullptr, FILE_BEGIN) || !SetEndOfFile(fd))
		throw std::runtime_error{"Unable to resize file"};
	auto oldSize = len;
	len					 = size;
	if (fMap = CreateFileMappingW(fd, nullptr, PAGE_READWRITE, 0, 0, nullptr);
			fMap == nullptr)
		throw std::runtime_error{"Unable to mmap after resize"};
	if (mMap = MapViewOfFileEx(fMap,
														 FILE_MAP_READ | FILE_MAP_WRITE,
														 0,
														 0,
														 size,
														 nullptr);
			mMap == nullptr) {
		throw std::runtime_error{"Unable to map view after resize"};
	}
	if (size > oldSize)
		std::fill(Get() + oldSize, Get() + size, 0);
	return *this;
}

uint8_t* MemMapper::Get() noexcept {
	assert(mMap);
	return static_cast<uint8_t*>(mMap);
}

void MemMapper::Close() noexcept {
	if (mMap != nullptr)
		UnmapViewOfFile(mMap);
	if (fMap != nullptr)
		CloseHandle(fMap);
}

MemMapper::~MemMapper() noexcept {
	Close();
}