#include "posix/MemMapper.h"

#include <cassert>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

using namespace AssetMap;
using namespace std::string_literals;

namespace fs = std::filesystem;

[[nodiscard]] static int OpenFile(const fs::directory_entry& file) {
	int fd;
	if (file.exists())
		fd = open(file.path().c_str(), O_RDONLY);
	else
		fd = open(file.path().c_str(),
							O_RDWR | O_CREAT,
							S_IRWXU | S_IRGRP | S_IROTH);
	if (fd == -1)
		throw std::runtime_error{"Unable to open "s +
														 file.path().generic_u8string()};
	return fd;
}

FileDescriptor::FileDescriptor(int fd) : fd{fd} {}

FileDescriptor::FileDescriptor(FileDescriptor&& rhs) noexcept : fd{rhs.fd} {
	rhs.fd = -1;
}

FileDescriptor& FileDescriptor::operator=(FileDescriptor&& rhs) noexcept {
	if (fd != -1)
		close(fd);
	fd		 = rhs.fd;
	rhs.fd = -1;
	return *this;
}

FileDescriptor::operator int() const noexcept {
	return fd;
}

FileDescriptor::~FileDescriptor() noexcept {
	if (fd != -1)
		close(fd);
}

MemMapper::MemMapper(const fs::directory_entry& file) :
		fd{OpenFile(file)}, len{file.exists() ? file.file_size() : 0} {
	if (len > 0)
		if (mMap =
						mmap(nullptr, len, PROT_READ, MAP_PRIVATE | MAP_NORESERVE, fd, 0);
				mMap == MAP_FAILED)
			throw std::runtime_error{"Unable to mmap "s +
															 file.path().generic_u8string()};
}

MemMapper::MemMapper(MemMapper&& rhs) noexcept :
		fd{std::move(rhs.fd)}, len{rhs.len}, mMap{rhs.mMap} {
	rhs.mMap = MAP_FAILED;
}

MemMapper& MemMapper::operator=(MemMapper&& rhs) noexcept {
	Close();
	fd			 = std::move(rhs.fd);
	len			 = rhs.len;
	mMap		 = rhs.mMap;
	rhs.mMap = MAP_FAILED;
	return *this;
}

size_t MemMapper::Size() const noexcept {
	return len;
}

IMemMapper& MemMapper::Resize(size_t size) {
	if (mMap != MAP_FAILED)
		munmap(mMap, len);
	mMap = MAP_FAILED;
	if (ftruncate(fd, size) == -1)
		throw std::runtime_error{"Unable to resize file"};
	auto oldSize = len;
	len					 = size;
	if (mMap = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
			mMap == MAP_FAILED) {
		throw std::runtime_error{"Unable to mmap after resize"};
	}
	if (size > oldSize)
		std::fill(Get() + oldSize, Get() + size, 0);
	return *this;
}

const uint8_t* MemMapper::Get() const noexcept {
	assert(mMap);
	return static_cast<uint8_t*>(mMap);
}

uint8_t* MemMapper::Get() noexcept {
	assert(mMap);
	return static_cast<uint8_t*>(mMap);
}

void MemMapper::Close() noexcept {
	if (mMap != MAP_FAILED)
		munmap(mMap, len);
}

MemMapper::~MemMapper() noexcept {
	Close();
}