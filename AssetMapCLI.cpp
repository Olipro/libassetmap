#include "Hashers.h"
#include "MemMappedArchive.h"
#include "MemMapper.h"
#include "ZSTDComp.h"

#include <CLI11.hpp>

#include <algorithm>
#include <filesystem>
#include <map>

// Workaround for https://gcc.gnu.org/bugzilla/show_bug.cgi?id=95833
// Also defined in ZSTDComp.cpp
#ifdef __GLIBCXX__
#	define stdReduce std::accumulate
#else
#	define stdReduce std::reduce
#endif

using namespace AssetMap;
using namespace std::string_literals;
using namespace std::string_view_literals;
namespace fs = std::filesystem;

enum class Mode
{
	COMPRESS,
	DECOMPRESS,
	INFO,
};

class AssetMapCLI {
	CLI::App app{"LibAssetMap Archive Builder/Extractor"};
	int compressionLevel = ZSTD::DefaultCompressLevel();
	int strategy				 = 0;
	float loadFactor		 = 0.8f;
	float dictSizeRatio	 = 0.01f;
	Mode mode						 = Mode::COMPRESS;
	bool overwrite			 = false;
	bool skip						 = false;
	bool rebuildDict		 = false;
	std::string oneFile;
	fs::directory_entry dir;
	fs::directory_entry file;
	fs::directory_entry dict;
	int exitCode = 0;

	void SetupDictionary(ZSTD& zstd) {
		if ((rebuildDict || !dict.exists()) && zstd.CreateDictionary(dir)) {
			auto&& [ptr, len] = zstd.Dictionary();
			std::ofstream{dict.path(), std::ios::binary | std::ios::trunc}.write(
					reinterpret_cast<const char*>(ptr),
					len);
		} else
			zstd.UseDictionary(dict);
	}

	void Compress(ICompress& comp, const IHasher& hash) {
		if (file.exists()) {
			fs::remove(file.path());
			file.refresh();
		}
		MemMapper out{file};
		MemMappedArchive{dir, hash, out, comp};
	}

	void Decompress(IDecompress& zstd, const IHasher& hash) const {
		MemMapper in{file};
		MemMappedArchive archive{in, zstd, hash};
		if (!oneFile.empty()) {
			auto item = archive[oneFile];
			if (!item)
				throw std::runtime_error{oneFile + " not found in the archive"};
			auto path =
					fs::directory_entry{dir.path() / fs::path{item.Name()}.filename()};
			if (path.exists() && !overwrite)
				throw std::runtime_error{oneFile +
																 " already exists. specify -f or delete it."};
			fs::remove(path);
			MemMapper out{path};
			out.Resize(item.DecompressedSize());
			out.Resize(item.Retrieve(out.Get(), out.Size()));
			return;
		}
		for (auto&& bucket : archive) {
			for (auto&& item : bucket) {
				auto loc = dir.path() / item.Name();
				fs::create_directories(loc.parent_path());
				auto ent = fs::directory_entry{loc};
				if (ent.exists()) {
					if (skip)
						continue;
					if (!overwrite)
						throw std::runtime_error{
								ent.path().generic_u8string() +
								" already exists and neither overwrite (-f) nor  skip "
								"existing (-e) specified. Aborted"};
					fs::remove(ent);
				}
				MemMapper out{ent};
				out.Resize(item.DecompressedSize());
				out.Resize(item.Retrieve(out.Get(), out.Size()));
			}
		}
	}

	void Info(IDecompress& comp, const IHasher& hash) {
		MemMapper in{file};
		MemMappedArchive archive{in, comp, hash};
		auto totalBuckets		= archive.BucketCount(),
				 emptyBuckets		= archive.EmptyBuckets();
		auto usedBuckets		= totalBuckets - emptyBuckets;
		auto smallestBucket = std::numeric_limits<ptrdiff_t>::max();
		auto largestBucket	= ptrdiff_t{0};
		std::map<uint32_t, uint32_t> distribution;
		auto pred = [&](auto acc, auto&& bucket) {
			auto size = std::distance(bucket.begin(), bucket.end());
			if (size > 0) {
				smallestBucket = std::min(size, smallestBucket);
				largestBucket	 = std::max(size, largestBucket);
				++distribution[size];
			}
			return acc + size;
		};
		auto locale = std::getenv("LANG");
		std::cout.imbue(std::locale{locale ? locale : "en_US"});
		std::cout << "Total Buckets: " << totalBuckets << '\n'
							<< "Total Unused: " << emptyBuckets << '\n'
							<< "Total Used: " << usedBuckets << '\n'
							<< "Dictionary Bytes: " << archive.DictionarySize() << std::endl;
		auto totalFiles = stdReduce(archive.begin(), archive.end(), 0, pred);
		std::cout << "Total Files: " << totalFiles << '\n'
							<< "Smallest Bucket: " << smallestBucket << '\n'
							<< "Largest Bucket: " << largestBucket << '\n'
							<< "Usage Ratio: "
							<< 100 * (usedBuckets / static_cast<float>(totalBuckets)) << "%\n"
							<< "Bytes Wasted: " << emptyBuckets * sizeof(lam_size_t) << '\n'
							<< "Average (Mean) Load: "
							<< totalFiles / static_cast<float>(usedBuckets) << '\n'
							<< "Distribution:\n";
		for (auto&& [size, count] : distribution) {
			constexpr auto s1 = " bucket with ";
			constexpr auto s2 = " element\n";
			constexpr auto p1 = " buckets with ";
			constexpr auto p2 = " elements\n";
			std::cout << "  " << count << (count > 1 ? p1 : s1) << size
								<< (size > 1 ? p2 : s2);
		}
		std::cout << "Total Archive Bytes: " << file.file_size() << '\n';
		if (dir.exists() && dir.is_directory()) {
			auto it = fs::recursive_directory_iterator{dir};
			auto totalSize =
					stdReduce(fs::begin(it), fs::end(it), 0, [](auto acc, auto& i) {
						return i.is_regular_file() ? acc + i.file_size() : acc;
					});
			std::cout << "Total Dir Bytes: " << totalSize << '\n'
								<< "Size Reduction: "
								<< 100 *
											 (1 - (file.file_size() / static_cast<float>(totalSize)))
								<< "%\n";
		}
	}

	void Execute() {
		CityHash hash{loadFactor};
		if (mode == Mode::COMPRESS) {
			ZSTD zstd{ZSTD::compress, dictSizeRatio};
			zstd.SetCompressLevel(compressionLevel);
			zstd.SetStrategyLevel(strategy);
			if (dict != fs::directory_entry{})
				SetupDictionary(zstd);
			Compress(zstd, hash);
		} else if (mode == Mode::DECOMPRESS) {
			ZSTD zstd{ZSTD::decompress};
			Decompress(zstd, hash);
		} else if (mode == Mode::INFO) {
			ZSTD zstd{ZSTD::decompress};
			Info(zstd, hash);
		}
	}

public:
	AssetMapCLI(int argc,
							const char* argv[],
							std::ostream& out = std::cout,
							std::ostream& err = std::cerr) {
		constexpr auto bucketFactorArg	= "-b,--bucket-factor";
		constexpr auto dictArg					= "-d,--dictionary";
		constexpr auto skipArg					= "-e,--skip-existing";
		constexpr auto forceArg					= "-f,--force";
		constexpr auto infoArg					= "-i,--info";
		constexpr auto oneFileArg				= "-o,--onefile";
		constexpr auto compLevelArg			= "-l,--level";
		constexpr auto rebuildDictArg		= "-r,--rebuild-dictionary";
		constexpr auto strategyArg			= "-s,--strategy";
		constexpr auto dictSizeRatioArg = "-t,--dictionary-ratio";
		constexpr auto decompArg				= "-x,--decompress";

		// Positionals, these aren't true args.
		constexpr auto fileArg = "file";
		constexpr auto dirArg	 = "dir";
		app.get_formatter()->column_width(40);
		auto* decomp = app.add_flag(
				decompArg,
				[this](auto count) {
					if (count)
						mode = Mode::DECOMPRESS;
				},
				"Decompress. If this option is absent, compress.");
		auto* infoOpt = app.add_flag(
				infoArg,
				[this](auto count) {
					if (count)
						mode = Mode::INFO;
				},
				"Prints information about an archive. No other operations will be\n"
				"performed.");
		app.add_option(strategyArg, strategy, ZSTD::StrategyInfo(), true)
				->check(CLI::Range(ZSTD::MinStrategyLevel(), ZSTD::MaxStrategyLevel()));
		auto* dictOpt =
				app.add_option(
							 dictArg,
							 dict,
							 "Use/create a dictionary. If the file does not exist, it will\n"
							 "be created. The dictionary is ALSO embedded in the archive.\n"
							 "It is "
							 "output separately since creating a dictionary is EXPENSIVE\n"
							 "and you most likely will want to re-use it. The dictionary\n"
							 "will be created with whatever compression level you define.\n"
							 "Future use of the dictionary will enforce that compression\n"
							 "level.")
						->excludes(decomp)
						->check([this](const auto& s) {
							return !file.exists() || file.is_regular_file()
												 ? ""
												 : "Error: dictionary path must be a file or any name\n"
													 "that doesn't exist on the filesystem.";
						});
		app.add_flag(rebuildDictArg,
								 rebuildDict,
								 "Delete and re-create the dictionary.")
				->needs(dictOpt);
		app.add_option(compLevelArg,
									 compressionLevel,
									 "Compression Level. When re-using a dictionary, the dict\n"
									 "overrides this. Negative compression levels are aimed at\n"
									 "speed over size.",
									 true)
				->excludes(decomp)
				->check(CLI::Range(ZSTD::MinCompressLevel(), ZSTD::MaxCompressLevel()))
				->check([this](auto&) {
					return dict.exists() && !rebuildDict
										 ? "Cannot specify compression level when reusing a "
											 "dictionary. You would have to regenerate it. (-r)"
										 : "";
				});
		app.add_flag(
				forceArg,
				overwrite,
				"When compressing, overwrite the target archive if it exists.\n"
				"When decompressing, overwrite any files that already exist");
		app.add_flag(skipArg,
								 skip,
								 "When decompressing, skip extraction of any files that\n"
								 "already exist. Otherwise, abort immediately.")
				->needs(decomp);
		app.add_option(dictSizeRatioArg,
									 dictSizeRatio,
									 "Desired dictionary size. 0.01, the default, represents a "
									 "dictionary that will be 1% of the total file size",
									 true);
		app.add_option(oneFileArg,
									 oneFile,
									 "Extract a single file by name into [dir]",
									 false)
				->needs(decomp);
		auto* fileOpt =
				app.add_option(
							 fileArg,
							 file,
							 "File to create/extract from. Specify -f/--force to allow\n"
							 "overwriting when compressing.")
						->check([&mode = this->mode, &overwrite = this->overwrite](
												const auto& s) -> std::string {
							fs::directory_entry file{s};
							if (mode == Mode::DECOMPRESS || mode == Mode::INFO) {
								if (!file.exists())
									return s + " does not exist.";
								else if (!file.is_regular_file())
									return s + " is not a regular file. Don't try to pass unix\n"
														 "sockets, block devices, etc.";
							} else if (file.exists() && !overwrite)
								return s + " already exists. use -f to force overwriting\n"
													 "(or delete it yourself)";
							return "";
						})
						->required();
		app.add_option(
					 dirArg,
					 dir,
					 "Directory to compress from or decompress into or, if\n"
					 "using -i, will be used to calculate compression ratio.\n"
					 "This will traverse the entire directory and does not check\n"
					 "if the archive was compressed from this dir.")
				->check([&mode = this->mode](const auto& s) -> std::string {
					fs::directory_entry dir{s};
					if (mode == Mode::COMPRESS && !dir.is_directory())
						return s + " is not a valid directory";
					return "";
				});
		app.add_option(bucketFactorArg,
									 loadFactor,
									 "Load Factor. This specifies how many buckets you want "
									 "relative to the\n"
									 "number of files. For example, a load factor of 0.5 will "
									 "create twice\n"
									 "as many buckets as there are files. A load factor of 1 "
									 "would produce\n"
									 "a 1:1 ratio. The lower the value, the less items go into "
									 "each bucket\n"
									 "(good) but at the expense of more space used for the "
									 "bucket table.\n"
									 "Depending on file names, you may find a ratio > 1 "
									 "acceptable - check\n"
									 "the distribution with -i after creating your archive.",
									 true)
				->excludes(decomp);
		infoOpt->excludes(decomp)->needs(fileOpt);

		try {
			app.parse(argc, argv);
			Execute();
		} catch (const CLI::ParseError& e) {
			std::cout << app.help("", CLI::AppFormatMode::All) << e.what() << '\n';
			exitCode = 1;
		} catch (const std::exception& e) {
			std::cerr << e.what() << '\n';
			exitCode = 2;
		}
	}

	[[nodiscard]] int ExitCode() const noexcept {
		return exitCode;
	}
};

int main(int argc, const char* argv[]) {
	AssetMapCLI builder{argc, argv};

	return builder.ExitCode();
}
