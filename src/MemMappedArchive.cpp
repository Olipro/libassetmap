#include "MemMappedArchive.h"

#include "DirectoryMetadata.h"
#include "MemMappedBucket.h"
#include "MemMapper.h"
#include "MemOps.h"

#include <algorithm>
#include <cassert>

using namespace AssetMap;

namespace fs = std::filesystem;

class ArchiveBuilder {
	uint8_t* const begin;
	uint8_t* const bucketsTbl;
	uint8_t* nextBucket;
	const fs::directory_entry& ent;
	IMemMapper& file;
	ICompress& comp;
	ptrdiff_t totalSize;

	void AddLength(size_t len) noexcept {
		totalSize += len;
		nextBucket += len;
	}

public:
	ArchiveBuilder(const DirectoryMetadata& meta,
								 const fs::directory_entry& ent,
								 IMemMapper&& file,
								 ICompress& comp) noexcept :
			begin{file.Resize(meta.TotalRequiredSpace()).Get()},
			bucketsTbl{begin + sizeof(lam_size_t)},
			nextBucket{begin + meta.DataStart()},
			ent{ent},
			file{file},
			comp{comp},
			totalSize{meta.DataStart()} {
		PutLamSizeT(begin, meta.Buckets().size());
	}

	void Add(const std::vector<fs::directory_entry>& bucket, lam_size_t id) {
		if (bucket.empty())
			return;
		MemMappedBucket mmBucket{begin, bucketsTbl, nextBucket - begin, id, comp};
		for (auto& bEntry : bucket) {
			fs::directory_entry fullPath{ent.path() / bEntry};
			MemMapper src{fullPath};
			AddLength(mmBucket.Append().Populate(bEntry.path().generic_u8string(),
																					 src.Get(),
																					 src.Size()));
		}
		AddLength(mmBucket.Append().MakeNull());
	}

	~ArchiveBuilder() noexcept(false) {
		auto [dict, len]			= comp.Dictionary();
		uint8_t hasDictionary = dict == nullptr ? 0 : 1;
		if (hasDictionary) {
			std::copy(dict, dict + len, begin + totalSize);
			totalSize += len;
			PutLamSizeT(begin + totalSize, len);
			totalSize += sizeof(lam_size_t);
		}
		begin[totalSize++] = hasDictionary;
		file.Resize(totalSize);
	}
};

static uint8_t HasDictionary(const uint8_t* buf, lam_size_t len) noexcept {
	return buf[len - 1];
}

static std::pair<const uint8_t*, size_t>
		DictionaryInfo(const uint8_t* buf, lam_size_t len) noexcept {
	auto* dictEnd = buf + len - (sizeof(lam_size_t) + sizeof(uint8_t));
	auto dictLen	= GetLamSizeT(dictEnd);
	return {dictEnd - dictLen, dictLen};
}

MemMappedArchive::MemMappedArchive(IMemMapper& file,
																	 IDecompress& decomp,
																	 const IHasher& hasher) :
		file{file}, decomp{&decomp}, hasher{hasher} {
	if (file.Size() == 0)
		throw std::runtime_error{"Attempt to open an empty file as an archive. "
														 "Did you call the wrong constructor?"};
	auto hasDictionary = HasDictionary(file.Get(), file.Size());
	if (hasDictionary == 1)
		LoadDictionary(decomp);
	else if (hasDictionary > 1)
		throw std::runtime_error{"Attempt to open a file with a future version"};
}

MemMappedArchive::MemMappedArchive(const fs::directory_entry& ent,
																	 const IHasher& hasher,
																	 IMemMapper& file,
																	 ICompress& comp,
																	 IDecompress* decomp) :
		file{file}, decomp{decomp}, hasher{hasher} {
	DirectoryMetadata meta{hasher, comp, ent};
	ArchiveBuilder builder{meta, ent, std::move(file), comp};
	auto& buckets = meta.Buckets();
	for (auto i = 0; i < buckets.size(); ++i)
		builder.Add(buckets[i], i);
}

template <class Comp>
void MemMappedArchive::LoadDictionary(Comp& comp) noexcept {
	auto&& [dictBegin, dictLen] = DictionaryInfo(file.Get(), file.Size());
	comp.UseDictionary(dictBegin, dictLen);
}

lam_size_t MemMappedArchive::BucketCount() const noexcept {
	auto* data = file.Get();
	return GetLamSizeT(data);
}

lam_size_t MemMappedArchive::EmptyBuckets() const noexcept {
	return std::count_if(begin(), end(), [](auto&& bucket) {
		return bucket.begin() == bucket.end();
	});
}

lam_size_t MemMappedArchive::DictionarySize() const noexcept {
	return HasDictionary(file.Get(), file.Size()) == 1
						 ? DictionaryInfo(file.Get(), file.Size()).second
						 : 0;
}

MemMappedBucketEntry
		MemMappedArchive::operator[](std::string_view name) const noexcept {
	auto bucketId = hasher.CalcBucket(hasher.Hash(name), BucketCount());
	return (*this)[bucketId][name];
}

MemMappedBucket MemMappedArchive::operator[](lam_size_t idx) const noexcept {
	assert(decomp != nullptr);
	auto* begin = file.Get();
	MemMappedBucket bucket{file.Get(), begin + sizeof(lam_size_t), idx, *decomp};
	return bucket;
}

MemMappedArchive::Iterator MemMappedArchive::begin() const noexcept {
	return {*this, 0};
}

MemMappedArchive::Iterator MemMappedArchive::end() const noexcept {
	return {*this, BucketCount()};
}

MemMappedArchive::Iterator::Iterator(const MemMappedArchive& archive,
																		 lam_size_t i) noexcept :
		archive{&archive}, i{i} {}

bool MemMappedArchive::Iterator::operator!=(
		const Iterator& rhs) const noexcept {
	return archive != rhs.archive || i != rhs.i;
}

MemMappedBucket MemMappedArchive::Iterator::operator->() noexcept {
	return (*archive)[i];
}

MemMappedBucket MemMappedArchive::Iterator::operator*() noexcept {
	return (*archive)[i];
}

MemMappedArchive::Iterator& MemMappedArchive::Iterator::operator++() noexcept {
	++i;
	return *this;
}
