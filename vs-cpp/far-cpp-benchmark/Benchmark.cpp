
#include "Pch.h"

#include "Common.h"
#include "FindAddRemove.h"

namespace
{
	using namespace std::string_literals;

	constexpr auto sep = '|';
	constexpr auto nl = '\n';

	constexpr bool doWarmup = false;

	struct BenchmarkRecord
	{
		int Turns;
		std::string Distribution;
		std::string Algorithm;
		std::map<int, double> SlotsToTimePerTurnNs;
	};

	std::vector<BenchmarkRecord> benchmarkRecords;


	template<typename...>
	class UniformGenerator;

	template<typename ElementTag>
	auto MakeUniformGenerator(ElementTag, int slots)
	{
		return UniformGenerator<ElementTag::value_type>(slots);
	}

	template<typename... T>
	auto to_string(UniformGenerator<T...>&) { return "uniform"s; }

	template<typename ElementType>
	class UniformGenerator<ElementType>
	{
		std::mt19937_64 engine { };
		std::uniform_int_distribution<> distribution;
	public:
		explicit UniformGenerator(int slots) :
			distribution { 0, slots - 1 }
		{ }

		ElementType operator()()
		{
			return static_cast<ElementType>(distribution(engine));
		}
	};

	template<typename ElementType>
	class UniformGenerator<std::unique_ptr<ElementType>>
	{
		std::mt19937_64 engine { };
		std::uniform_int_distribution<> distribution;
	public:
		explicit UniformGenerator(int slots) :
			distribution { 0, slots - 1 }
		{ }

		auto operator()()
		{
			return std::make_unique<ElementType>(static_cast<ElementType>(distribution(engine)));
		}
	};

	template<typename ElementType>
	class UniformGenerator<std::shared_ptr<ElementType>>
	{
		std::mt19937_64 engine { };
		std::uniform_int_distribution<> distribution;
	public:
		explicit UniformGenerator(int slots) :
			distribution { 0, slots - 1 }
		{ }

		auto operator()()
		{
			return std::make_shared<ElementType>(static_cast<ElementType>(distribution(engine)));
		}
	};

	template<typename ElementTag, typename AlgorithmTag>
	void Benchmark(int turns, int slots, ElementTag elementTag, AlgorithmTag algorithmTag)
	{
		namespace chrono = std::chrono;

		std::cout << ".";
		std::flush(std::cout);

		// Warm up the code.
		//
		if (doWarmup)
			PlayFindAddRemove(turns, slots, MakeUniformGenerator(elementTag, std::max(slots / 8, 1)), algorithmTag);

		// Run the workload (measuring the time).
		//
		auto clock = chrono::system_clock{ };
		const auto time0 = clock.now();
		const auto simulationResult = PlayFindAddRemove(turns, slots, MakeUniformGenerator(elementTag, slots), algorithmTag);
		const auto time1 = clock.now();

		const auto distribution = "uniform";
		const auto algorithm = typeid(typename algorithmTag).name();
		const auto averageFillRatio = GetRatioOf(simulationResult.SumOfSizes, { turns }) / slots;
		const auto timePerTurnNs = GetRatioOf(chrono::duration_cast<chrono::nanoseconds>(time1 - time0).count(), { turns });

		//std::cout << turns
		//	<< sep << slots
		//	<< sep << randomDistributionName
		//	<< sep << collectionName
		//	<< sep << averageFillRatio
		//	<< sep << timePerTurnNs
		//	<< std::endl;

		auto finding = std::find_if(std::begin(benchmarkRecords), std::end(benchmarkRecords), [&](const BenchmarkRecord& br) {
			return br.Turns == turns &&
				br.Distribution == distribution &&
				br.Algorithm == algorithm;
		});

		if (finding == std::end(benchmarkRecords))
		{
			BenchmarkRecord br { turns, std::move(distribution), std::move(algorithm), { std::make_pair(slots, timePerTurnNs) }};
			benchmarkRecords.push_back(std::move(br));
		}
		else
		{
			finding->SlotsToTimePerTurnNs.insert(std::make_pair(slots, timePerTurnNs));
		}
	}
}

void Benchmark(int turns)
{
	std::cout << "The benchmark performs {turns}=" << std::to_string(turns) << " number of iterations. " << nl
		<< "Every turn the pseudo-random generator generates a number within the range from 0 to {space}-1 using {distribution}. " << nl
		<< "That number is placed into {dataset} collection, or it is removed from it if the number was already present. "
		<< std::endl;

	std::cout << "Processing...";

	//std::cout << "turns"
	//	<< sep << "slots"
	//	<< sep << "distribution"
	//	<< sep << "dataset"
	//	<< sep << "average_fill_ratio"
	//	<< sep << "time_per_turn_ns"
	//	<< std::endl;

	auto slotsSeries = IntegerConstants<
		1,
		4,
		16,
		64,
		256,
		1 * 1024,
		4 * 1024,
		16 * 1024,
		64 * 1024,
		256 * 1024,
		1 * 1024 * 1024,
		4 * 1024 * 1024,
		16 * 1024 * 1024
		//64 * 1024 * 1024,
		//256 * 1024 * 1024
	>{};

	auto maxSlotsForBitset = IntegerConstants<4 * 1024 * 1024>{};
	auto maxSlotsForSequence = IntegerConstants<4 * 1024>{};

	// For each number of slots...
	//
	ForEachIntegerConstant(slotsSeries, [=](auto slots)
	{
		// void is special - it only invokes the pseudo-random generator.
		//
		Benchmark(turns, slots.value(), Tag<int64_t>{}, Tag<void>{});

		// In positional algorithm, slot is determined by the position (index) in a pre-allocated space.
		//
		{
			ForEachTag(Tag<
				PositionalTag<std::unique_ptr<bool[]>, uint8_t>,
				PositionalTag<std::unique_ptr<bool[]>, int8_t>,
				PositionalTag<std::unique_ptr<bool[]>, uint16_t>,
				PositionalTag<std::unique_ptr<bool[]>, int16_t>,
				PositionalTag<std::unique_ptr<bool[]>, uint32_t>,
				PositionalTag<std::unique_ptr<bool[]>, int32_t>,
				PositionalTag<std::unique_ptr<bool[]>, uint64_t>,
				PositionalTag<std::unique_ptr<bool[]>, int64_t>,
				PositionalTag<std::unique_ptr<uint8_t[]>>,
				PositionalTag<std::unique_ptr<int8_t[]>>,
				PositionalTag<std::unique_ptr<uint16_t[]>>,
				PositionalTag<std::unique_ptr<int16_t[]>>,
				PositionalTag<std::unique_ptr<uint32_t[]>>,
				PositionalTag<std::unique_ptr<int32_t[]>>,
				PositionalTag<std::unique_ptr<uint64_t[]>>,
				PositionalTag<std::unique_ptr<int64_t[]>>,
				PositionalTag<std::unique_ptr<int128_t[]>>,
				PositionalTag<std::vector<bool>>
			>{},
				[=](auto algorithmTagTag)
			{
				using AlgorithmTag = decltype(algorithmTagTag)::value_type;
				Benchmark(turns, slots.value(), Tag<int64_t>{}, AlgorithmTag{});
				//std::cout << typeid(typename AlgorithmTag).name() << std::endl;
			});

			if (slots.value() <= maxSlotsForBitset.value())
			{
				Benchmark(turns, slots.value(), Tag<int64_t>{}, PositionalTag<std::bitset<slots.value()>>{});
			}
		}

		// In container-based algorithm, slot is an unique element in the colleciton.
		//
		{
			ForEachTag(Tag<
				int8_t,
				uint8_t,
				int16_t,
				uint16_t,
				int32_t,
				uint32_t,
				int64_t,
				uint64_t
			>{},
				[=](auto primitiveTag)
			{
				using PrimitiveType = decltype(primitiveTag)::value_type;

				ForEachTag(Tag<
					PrimitiveType,
					std::unique_ptr<PrimitiveType>,
					std::shared_ptr<PrimitiveType>
				>{}, [=](auto elementTag)
				{
					using ElementType = typename decltype(elementTag)::value_type;

					auto runBenchmark = [=](auto algorithmTagTag)
					{
						using AlgorithmTag = decltype(algorithmTagTag)::value_type;
						Benchmark(turns, slots.value(), elementTag, AlgorithmTag{});
					};

					if (static_cast<uint64_t>(slots.value()) > static_cast<uint64_t>(std::numeric_limits<PrimitiveType>::max()))
						return;

					// Sequence-based algorithms
					// When there are many slots, these work too slow to be included in benchmark.
					//
					if (slots.value() <= maxSlotsForSequence.value())
					{
						auto sequenceAlgorithms = Tag<
							SequenceUnsortedTag<std::vector<ElementType>>,
							SequenceSortedTag<std::vector<ElementType>>,
							SequenceUnsortedTag<std::deque<ElementType>>,
							SequenceSortedTag<std::deque<ElementType>>,
							SequenceUnsortedTag<std::list<ElementType>>,
							SequenceSortedTag<std::list<ElementType>>
						>{};

						ForEachTag(sequenceAlgorithms, runBenchmark);

						ConstexprIf(BoolType<std::is_integral<ElementType>::value>{}, [=]{
							runBenchmark(Tag<SequenceOtherTag<plf::colony<ElementType>>>{});
						});
					}

					// Set-based algorithms
					//
					{
						auto treeAlgorithms = Tag<
							SetTag<std::set<ElementType, less<ElementType>>>,
							SetTag<std::unordered_set<ElementType, hash<ElementType>>>,
							SetTag<boost::container::flat_set<ElementType, less<ElementType>>>
							//SetTag<stx::btree_set<ElementType, less<ElementType>>>
							//SetTag<btree::btree_set<ElementType, less<ElementType>>>
							//SetTag<google::sparse_hash_set<ElementType>>
							//SetTag<google::dense_hash_set<ElementType>> <- flawed: crashes
						>{};

						ForEachTag(treeAlgorithms, runBenchmark);
					}
				});
			});
		}
	});

	std::cout << std::endl;

	{
		std::cout << "turns"
			<< sep << "distribution"
			<< sep << "algorithm";

		ForEachIntegerConstant(slotsSeries, [=](auto slots)
		{
			std::cout << sep << "time:s" << slots.value();
		});

		std::cout << std::endl;

		for (const auto& br : benchmarkRecords)
		{
			std::cout << br.Turns
				<< sep << br.Distribution
				<< sep << br.Algorithm;

			ForEachIntegerConstant(slotsSeries, [=](auto slots)
			{
				std::cout << sep;

				auto finding = br.SlotsToTimePerTurnNs.find(slots.value());
				if (finding != std::end(br.SlotsToTimePerTurnNs))
				{
					std::cout << finding->second;
				}
			});

			std::cout << std::endl;
		}
	}

} // namespace


int main(const int argc, const char* const argv[])
{
	const auto turns = argc >= 2 ? std::stoi(argv[1]) : 1024;

	Benchmark(turns);
	return 0;
}
