#include <catch.hpp>

#include "Hashers.h"
#include "MemMappedArchive.h"
#include "MemMapper.h"
#include "ZSTDComp.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <random>
#include <string>
#include <string_view>

using namespace AssetMap;
using namespace std::string_literals;
using namespace std::string_view_literals;

namespace fs = std::filesystem;

static std::string_view ToSV(const uint8_t* p, size_t len) {
	return {reinterpret_cast<const char*>(p), len};
}

class FSCleanup {
protected:
	fs::path dir = fs::current_path() / "testArchive";
	fs::path arc = fs::current_path() / "testme.lam";

public:
	FSCleanup() {
		fs::remove_all(dir);
		fs::remove(arc);
		REQUIRE(fs::create_directory(dir));
	}
};

SCENARIO_METHOD(FSCleanup,
								"An archive can be successfully compressed and decompressed") {
	GIVEN("A Directory with some random files in it") {
		auto data1 = "This is a test string123"sv;
		auto data2 = "This is \xBD binary321"sv;
		auto data3 = "Another string231"sv;
		std::ofstream{dir / "file1.txt"} << data1;
		std::ofstream{dir / "file2.txt", std::ios::binary} << data2;
		std::ofstream{dir / "file3.txt"} << data3;
		WHEN("We compress it into an archive") {
			ZSTD comp{ZSTD::both};
			MemMapper out{fs::directory_entry{arc}};
			CityHash hash;
			MemMappedArchive archive{fs::directory_entry{dir},
															 hash,
															 out,
															 comp,
															 &comp};
			THEN("We should be able to retrieve the contents") {
				auto file1 = archive["file1.txt"];
				auto file2 = archive["file2.txt"];
				auto file3 = archive["file3.txt"];
				REQUIRE(file1);
				REQUIRE(file2);
				REQUIRE(file3);
				REQUIRE(file1.FileSize() != 0);
				REQUIRE(file2.FileSize() != 0);
				REQUIRE(file3.FileSize() != 0);
				AND_THEN("The first file should work") {
					auto&& [ptr, len] = file1.Retrieve();
					REQUIRE(len == data1.size());
					REQUIRE(ToSV(ptr.get(), len) == data1);
				}
				AND_THEN("The second file should work") {
					auto&& [ptr, len] = file2.Retrieve();
					REQUIRE(len == data2.size());
					REQUIRE(ToSV(ptr.get(), len) == data2);
				}
				AND_THEN("The third file should work") {
					auto&& [ptr, len] = file3.Retrieve();
					REQUIRE(len == data3.size());
					REQUIRE(ToSV(ptr.get(), len) == data3);
				}
			}
		}
	}
}

SCENARIO_METHOD(FSCleanup, "An archive can be written to disk and re-read") {
	GIVEN("A Directory with some random files in it") {
		auto data1 = "This is a test string"sv;
		auto data2 = "This is \xBD binary"sv;
		auto data3 = "Another string"sv;
		std::ofstream{dir / "file1.txt"} << data1;
		std::ofstream{dir / "file2.txt", std::ios::binary} << data2;
		std::ofstream{dir / "file3.txt"} << data3;
		CityHash hash;
		ZSTD comp{ZSTD::both};
		WHEN("We compress it into an archive") {
			{
				MemMapper out{fs::directory_entry{arc}};
				MemMappedArchive{fs::directory_entry{dir}, hash, out, comp};
			}
			AND_WHEN("We re-open the file") {
				MemMapper in{fs::directory_entry{arc}};
				const MemMappedArchive archive{in, comp, hash};
				THEN("We should be able to retrieve the contents after re-opening") {
					auto file1 = archive["file1.txt"];
					auto file2 = archive["file2.txt"];
					auto file3 = archive["file3.txt"];
					REQUIRE(file1);
					REQUIRE(file2);
					REQUIRE(file3);
					REQUIRE(file1.FileSize() != 0);
					REQUIRE(file2.FileSize() != 0);
					REQUIRE(file3.FileSize() != 0);
					{
						auto&& [ptr, len] = file1.Retrieve();
						REQUIRE(len == data1.size());
						REQUIRE(ToSV(ptr.get(), len) == data1);
					}
					{
						auto&& [ptr, len] = file2.Retrieve();
						REQUIRE(len == data2.size());
						REQUIRE(ToSV(ptr.get(), len) == data2);
					}
					{
						auto&& [ptr, len] = file3.Retrieve();
						REQUIRE(len == data3.size());
						REQUIRE(ToSV(ptr.get(), len) == data3);
					}
				}
			}
		}
	}
}

SCENARIO_METHOD(
		FSCleanup,
		"An archive with a dictionary can be created and decompressed") {
	GIVEN("A set of files with repetitive data") {
		constexpr auto repeat			 = "repeated string";
		constexpr auto repeatCount = 10000;
		constexpr auto fileCount	 = 100;
		std::string data{repeat};
		data.reserve(data.size() * (repeatCount - 1));
		for (auto i = 0; i < repeatCount - 1; ++i)
			data.append(repeat);
		std::minstd_rand rng; // ...and some random data.
		rng.seed(std::random_device{}());
		for (auto i = 0; i < fileCount; ++i) {
			auto file = "file"s + std::to_string(i) + ".txt";
			std::ofstream f{dir / file, std::ios::binary};
			f << data;
			for (auto j = 0; j < 100; ++j)
				f << rng();
		}
		CityHash hash;
		WHEN("We create a dictionary and compress the files") {
			{
				ZSTD comp{ZSTD::both};
				REQUIRE(comp.CreateDictionary(fs::directory_entry{dir}));
				MemMapper in{fs::directory_entry{arc}};
				MemMappedArchive{fs::directory_entry{dir}, hash, in, comp};
			}
			THEN("Attempting to read the archive should succeed") {
				ZSTD comp{ZSTD::both};
				MemMapper in{fs::directory_entry{arc}};
				MemMappedArchive archive{in, comp, hash};
				REQUIRE(archive.BucketCount() > 0);
				for (auto i = 0; i < fileCount; ++i) {
					auto file = "file"s + std::to_string(i) + ".txt";
					auto item = archive[file];
					REQUIRE(item);
					auto&& [ptr, len] = item.Retrieve();
					auto f						= fs::directory_entry{dir / file};
					auto fileSize			= f.file_size();
					MemMapper onDisk{f};
					REQUIRE(len == fileSize);
					REQUIRE(ToSV(ptr.get(), len) == ToSV(onDisk.Get(), onDisk.Size()));
				}
			}
		}
	}
}

SCENARIO_METHOD(FSCleanup,
								"An archive with a dictionary and a few MB random data file "
								"can be created and decompressed") {
	GIVEN("A set of files with repetitive data") {
		constexpr auto repeat			 = "repeated string";
		constexpr auto repeatCount = 10000;
		constexpr auto fileCount	 = 101;
		std::string data{repeat};
		data.reserve(data.size() * (repeatCount - 1));
		for (auto i = 0; i < repeatCount - 1; ++i)
			data.append(repeat);
		std::minstd_rand rng; // ...and some random data.
		rng.seed(std::random_device{}());
		for (auto i = 0; i < fileCount - 1; ++i) {
			auto file = "file"s + std::to_string(i) + ".txt";
			std::ofstream f{dir / file, std::ios::binary};
			f << data;
			for (auto j = 0; j < 100; ++j)
				f << rng();
		}
		{
			std::ofstream f{dir / ("file"s + std::to_string(fileCount - 1) + ".txt")};
			for (auto j = 0; j < (4'194'304) / sizeof(std::minstd_rand::result_type);
					 ++j)
				f << rng();
		}
		CityHash hash{1.2};
		WHEN("We create a dictionary and compress the files") {
			{
				ZSTD comp{ZSTD::compress, 0.00001};
				REQUIRE(comp.CreateDictionary(fs::directory_entry{dir}));
				MemMapper in{fs::directory_entry{arc}};
				MemMappedArchive{fs::directory_entry{dir}, hash, in, comp};
			}
			THEN("Attempting to read the archive should succeed") {
				ZSTD comp{ZSTD::decompress};
				MemMapper in{fs::directory_entry{arc}};
				MemMappedArchive archive{in, comp, hash};
				REQUIRE(archive.BucketCount() > 0);
				for (auto i = 0; i < fileCount; ++i) {
					auto file = "file"s + std::to_string(i) + ".txt";
					auto item = archive[file];
					REQUIRE(item);
					auto&& [ptr, len] = item.Retrieve();
					auto f						= fs::directory_entry{dir / file};
					auto fileSize			= f.file_size();
					MemMapper onDisk{f};
					REQUIRE(len == fileSize);
					REQUIRE(ToSV(ptr.get(), len) == ToSV(onDisk.Get(), onDisk.Size()));
				}
			}
		}
	}
}
