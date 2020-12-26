#include "DirectoryMetadata.h"
#include "MemOps.h"

using namespace AssetMap;

namespace fs = std::filesystem;

DirectoryMetadata::DirectoryMetadata(const IHasher& hasher,
																		 ICompress& comp,
																		 const fs::directory_entry& ent) :
		dictionarySize{comp.Dictionary().second} {
	std::vector<fs::directory_entry> files;
	for (auto& file : fs::recursive_directory_iterator{ent}) {
		if (!file.is_regular_file())
			continue;
		auto compressBound = comp.CalcCompressSize(file.file_size());
		totalCompressBound += compressBound;
		auto& name				= files.emplace_back(fs::relative(file, ent));
		auto fileNameSize = name.path().generic_u8string().size() + 1;
		totalFileNameSize += fileNameSize; // inc. \0
		auto unalignedSize = sizeof(lam_size_t) + fileNameSize + compressBound;
		auto mod					 = unalignedSize % sizeof(lam_size_t);
		totalAlignmentPadding += mod ? sizeof(lam_size_t) - mod : 0;
	}
	totalNumFiles						= files.size();
	const auto bucketTarget = hasher.CalcBucketsForItemCount(totalNumFiles);
	buckets.resize(bucketTarget);
	for (auto& file : files) {
		auto bucketId =
				hasher.CalcBucket(hasher.Hash(file.path().generic_u8string()),
													bucketTarget);
		auto& bucket = buckets[bucketId];
		bucket.emplace_back(std::move(file));
	}
}

size_t DirectoryMetadata::TotalRequiredSpace() const noexcept {
	size_t ret = sizeof(lam_size_t);						// bucket count @ addr + 0
	ret += sizeof(lam_size_t) * buckets.size(); // bucket offset values @ addr + 4
	ret += sizeof(lam_size_t) * totalNumFiles;	// length prefix on each bucket
	ret += totalFileNameSize;										// space for each file name.
	ret += totalCompressBound;		// space for each compressed data payload.
																// (worst-case)
	ret += totalAlignmentPadding; // space for alignment padding. (worst-case)
	ret += (sizeof(lam_size_t) * 2) *
				 buckets.size(); // space for terminating entry of each bucket list.
	if (dictionarySize > 0) {
		ret += dictionarySize;		 // space for dictionary data
		ret += sizeof(lam_size_t); // space for dictionary size
	}
	ret += sizeof(uint8_t); // dictionary presence flag
	return ret;
}

ptrdiff_t DirectoryMetadata::DataStart() const noexcept {
	return sizeof(lam_size_t) * (buckets.size() + 1);
}

auto DirectoryMetadata::Buckets() const noexcept -> const decltype(buckets)& {
	return buckets;
}
