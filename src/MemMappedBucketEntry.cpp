#include "MemMappedBucketEntry.h"

#include "MemOps.h"

using namespace AssetMap;

MemMappedBucketEntry::MemMappedBucketEntry(uint8_t* data,
																					 const ICompress& comp) :
		data{data}, comp{&comp} {}

MemMappedBucketEntry::MemMappedBucketEntry(uint8_t* data,
																					 const IDecompress& decomp) :
		data{data}, decomp{&decomp} {}

MemMappedBucketEntry::MemMappedBucketEntry(std::nullptr_t) {}

lam_size_t MemMappedBucketEntry::FileSize() const noexcept {
	return GetLamSizeT(data);
}

size_t MemMappedBucketEntry::InMemorySize() const noexcept {
	auto len = sizeof(lam_size_t) + Name().size() + 1 + FileSize();
	auto mod = len % sizeof(lam_size_t);
	len += mod ? sizeof(lam_size_t) - mod : 0;
	return len;
}

void MemMappedBucketEntry::FileSize(lam_size_t size) {
	PutLamSizeT(data, size);
}

lam_size_t MemMappedBucketEntry::DecompressedSize() const noexcept {
	return decomp->CalcDecompressSize(FileData(), FileSize());
}

std::string_view MemMappedBucketEntry::Name() const noexcept {
	return {reinterpret_cast<const char*>(data + sizeof(lam_size_t))};
}

void MemMappedBucketEntry::Name(std::string_view name) noexcept {
	auto* str = name.data();
	auto len	= name.size();
	std::copy(str, str + len, data + sizeof(lam_size_t));
	data[sizeof(lam_size_t) + len] = '\0';
}

size_t MemMappedBucketEntry::Populate(std::string_view name,
																			const uint8_t* ptr,
																			size_t len) noexcept {
	Name(name);
	auto compBound = comp->CalcCompressSize(len);
	len						 = comp->Compress(ptr, len, FileData(), compBound);
	FileSize(len);
	auto minFill =
			std::min(sizeof(lam_size_t) + sizeof(uint8_t), compBound - len);
	auto zeroBegin = data + InMemorySize();
	auto zeroEnd	 = zeroBegin + minFill;
	std::fill(zeroBegin, zeroEnd, 0);
	return InMemorySize();
}

std::pair<std::unique_ptr<uint8_t[]>, size_t>
		MemMappedBucketEntry::Retrieve() const {
	auto len	= decomp->CalcDecompressSize(FileData(), FileSize());
	auto ret	= std::make_unique<uint8_t[]>(len);
	auto* buf = ret.get();
	return {std::move(ret), Retrieve(buf, len)};
}

size_t MemMappedBucketEntry::Retrieve(uint8_t* buf, size_t len) const {
	return decomp->Decompress(FileData(), FileSize(), buf, len);
}

size_t MemMappedBucketEntry::MakeNull() noexcept {
	Name({});
	FileSize(0);
	return InMemorySize();
}

uint8_t* MemMappedBucketEntry::FileData() noexcept {
	return data + sizeof(lam_size_t) + Name().size() + 1;
}

const uint8_t* MemMappedBucketEntry::FileData() const noexcept {
	return data + sizeof(lam_size_t) + Name().size() + 1;
}

MemMappedBucketEntry& MemMappedBucketEntry::operator++() noexcept {
	data += InMemorySize();
	return *this;
}

bool MemMappedBucketEntry::operator==(
		const MemMappedBucketEntry& rhs) const noexcept {
	return data == rhs.data;
}

bool MemMappedBucketEntry::operator!=(
		const MemMappedBucketEntry& rhs) const noexcept {
	return !(*this == rhs);
}

MemMappedBucketEntry::operator bool() const noexcept {
	return data != nullptr && !Name().empty();
}
