#include "MemMappedBucket.h"
#include "MemOps.h"

using namespace AssetMap;

MemMappedBucket::Iterator::Iterator(MemMappedBucketEntry&& entry) :
		entry{std::move(entry)} {}

MemMappedBucketEntry& MemMappedBucket::Iterator::operator*() noexcept {
	return entry;
}

MemMappedBucketEntry& MemMappedBucket::Iterator::operator->() noexcept {
	return entry;
}

MemMappedBucket::Iterator& MemMappedBucket::Iterator::operator++() noexcept {
	++entry;
	if (entry.Name().empty())
		entry = MemMappedBucketEntry{nullptr};
	return *this;
}

bool MemMappedBucket::Iterator::operator==(const Iterator& rhs) const noexcept {
	return entry == rhs.entry;
}

bool MemMappedBucket::Iterator::operator!=(const Iterator& rhs) const noexcept {
	return !(*this == rhs);
}

MemMappedBucket::MemMappedBucket(uint8_t* begin,
																 uint8_t* bucketsTbl,
																 lam_size_t id,
																 IDecompress& decomp) noexcept :
		data{begin + GetLamSizeT(bucketsTbl + (id * sizeof(lam_size_t)))},
		next{data, decomp},
		decomp{&decomp} {
	if (data == begin)
		data = nullptr;
}

MemMappedBucket::MemMappedBucket(uint8_t* begin,
																 uint8_t* bucketsTbl,
																 ptrdiff_t offset,
																 lam_size_t bucketId,
																 ICompress& comp) noexcept :
		data{begin + offset}, next{data, comp}, comp{&comp} {
	PutLamSizeT(bucketsTbl + (bucketId * sizeof(lam_size_t)), offset);
	static_cast<void>(MemMappedBucketEntry{data, comp}.MakeNull());
}

MemMappedBucketEntry MemMappedBucket::Append() noexcept {
	return next ? ++next : next;
}

MemMappedBucketEntry MemMappedBucket::operator[](std::string_view name) const {
	for (auto i = begin(); i != end();) {
		auto entry = *i;
		if (++i == end() || entry.Name() == name)
			return entry;
	}
	return MemMappedBucketEntry{nullptr};
}

MemMappedBucket::Iterator MemMappedBucket::begin() const noexcept {
	if (data == nullptr)
		return end();
	return Iterator{MemMappedBucketEntry{data, *decomp}};
}

MemMappedBucket::Iterator MemMappedBucket::end() const noexcept {
	return Iterator{MemMappedBucketEntry{nullptr}};
}
