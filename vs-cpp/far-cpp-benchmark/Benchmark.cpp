
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

	class UniformGenerator
	{
		std::mt19937_64 engine{};
		std::uniform_int_distribution<> distribution;
	public:
		explicit UniformGenerator(int slots) :
			distribution{ 1, slots - 1 }
		{ }

		int64_t operator()()
		{
			return distribution(engine);
		}
	};

	template<typename T>
	class plf_colony_allocator
	{
	private:
		plf::colony<T> _colony;
	public:
		using value_type = T;
		using pointer = typename plf::colony<T>::iterator;
		using const_pointer = typename plf::colony<T>::const_iterator;
		using reference = T&;
		using const_reference = const T&;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using propagate_on_container_move_assignment = std::true_type;
		template<typename U> struct rebind { using other = plf_colony_allocator<U>; };
		using is_always_equal = std::false_type;

		pointer allocate(std::size_t n) { assert(n == 1); return _colony.insert(0); }
		void deallocate(pointer p, std::size_t n) { assert(n == 1); _colony.erase(p); }
	};

    struct less_dereference
    {
        template<typename PtrType>
        bool operator()(const PtrType a, const PtrType b) const
        {
            return *a < *b;
        }
    };

    struct equal_dereference
    {
        using is_transparent = std::true_type;

        template<typename PtrType>
        bool operator()(const PtrType a, const PtrType b) const
        {
            return *a == *b;
        }
    };

    struct hash_dereference
    {
        template<typename PtrType>
        std::size_t operator()(const PtrType p) const
        {
            return std::hash<std::remove_reference_t<decltype(*p)>>{}(*p);
        }
    };


	template<typename AlgorithmTag, typename AllocatorTag>
	void Benchmark(int turns, int slots, AlgorithmTag algorithmTag, AllocatorTag allocatorTag)
	{
		namespace chrono = std::chrono;

		std::cout << '.';
		std::flush(std::cout);

		// Warm up the code.
		//
		if (doWarmup)
			PlayFindAddRemove(turns, slots, UniformGenerator{std::max(slots / 8, 1)}, algorithmTag, allocatorTag);

		// Run the workload (measuring the time).
		//
		auto clock = chrono::system_clock{ };
		const auto time0 = clock.now();
		const auto result = PlayFindAddRemove(turns, slots, UniformGenerator{slots}, algorithmTag, allocatorTag);
		const auto time1 = clock.now();

		const auto distribution = "uniform";
		const auto algorithm = typeid(typename algorithmTag).name();
		const volatile auto averageFillRatio = GetRatioOf(result.SumOfSizes, { turns }) / slots;
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

		if (finding == std::end(benchmarkRecords)) {
			BenchmarkRecord br { turns, std::move(distribution), std::move(algorithm), { std::make_pair(slots, timePerTurnNs) }};
			benchmarkRecords.push_back(std::move(br));
		} else {
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
		//1,
		4,
		//16,
		64,
		//256,
		//1 * 1024,
		4 * 1024,
		//16 * 1024,
		//64 * 1024,
		256 * 1024
		//1 * 1024 * 1024,
		//4 * 1024 * 1024,
		//16 * 1024 * 1024,
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
		Benchmark(turns, slots.value(), Tag<void>{}, Tag<void>{});

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
				Benchmark(turns, slots.value(), AlgorithmTag{}, Tag<void>{});
			});

			if (slots.value() <= maxSlotsForBitset.value())
			{
				Benchmark(turns, slots.value(), PositionalTag<std::bitset<slots.value()>>{}, Tag<void>{});
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
					std::allocator<PrimitiveType>
				>{}, [=](auto allocatorTag)
				{
					using AllocatorType = typename decltype(allocatorTag)::value_type;

					// Every slot must be able to be addressed with available integer bits.
					//
					if (static_cast<uint64_t>(slots.value()) > static_cast<uint64_t>(std::numeric_limits<PrimitiveType>::max()))
						return;

					// Sequence-based algorithms
					// When there are many slots, these work too slow to be included in benchmark.
					//
					if (slots.value() <= maxSlotsForSequence.value())
					{
						ForEachTag(Tag<
							SequenceUnsortedTag<std::vector<PrimitiveType, AllocatorType>>,
							SequenceSortedTag<std::vector<PrimitiveType, AllocatorType>>,
							SequenceUnsortedTag<std::deque<PrimitiveType, AllocatorType>>,
							SequenceSortedTag<std::deque<PrimitiveType, AllocatorType>>,
							SequenceUnsortedTag<std::list<PrimitiveType, AllocatorType>>,
							SequenceSortedTag<std::list<PrimitiveType, AllocatorType>>
						>{},
							[=](auto algorithmTagTag)
						{
							using AlgorithmTag = decltype(algorithmTagTag)::value_type;
							Benchmark(turns, slots.value(), AlgorithmTag{}, Tag<void>{});
						});

						ForEachTag(Tag<
							SequenceUnsortedPlainPtrTag<std::vector<PrimitiveType*>>,
							SequenceSortedPlainPtrTag<std::vector<PrimitiveType*>>,
							SequenceUnsortedPlainPtrTag<std::deque<PrimitiveType*>>,
							SequenceSortedPlainPtrTag<std::deque<PrimitiveType*>>,
							SequenceUnsortedPlainPtrTag<std::list<PrimitiveType*>>,
							SequenceSortedPlainPtrTag<std::list<PrimitiveType*>>
						>{},
							[=](auto algorithmTagTag)
						{
							using AlgorithmTag = decltype(algorithmTagTag)::value_type;
							Benchmark(turns, slots.value(), AlgorithmTag{}, Tag<void>{});
						});

						ForEachTag(Tag<
							SequenceUnsortedPtrTag<std::vector<PrimitiveType*>>,
							SequenceSortedPtrTag<std::vector<PrimitiveType*>>,
							SequenceUnsortedPtrTag<std::deque<PrimitiveType*>>,
							SequenceSortedPtrTag<std::deque<PrimitiveType*>>,
							SequenceUnsortedPtrTag<std::list<PrimitiveType*>>,
							SequenceSortedPtrTag<std::list<PrimitiveType*>>
						>{},
							[=](auto algorithmTagTag)
						{
							using AlgorithmTag = decltype(algorithmTagTag)::value_type;
							Benchmark(turns, slots.value(), AlgorithmTag{}, Tag<std::allocator<PrimitiveType>>{});
						});

						using ColonyPointer = plf_colony_allocator<PrimitiveType>::pointer;
						ForEachTag(Tag<
							SequenceUnsortedPtrTag<std::vector<ColonyPointer>>,
							SequenceSortedPtrTag<std::vector<ColonyPointer>>,
							SequenceUnsortedPtrTag<std::deque<ColonyPointer>>,
							SequenceSortedPtrTag<std::deque<ColonyPointer>>,
							SequenceUnsortedPtrTag<std::list<ColonyPointer>>,
							SequenceSortedPtrTag<std::list<ColonyPointer>>
						>{},
							[=](auto algorithmTagTag)
						{
							using AlgorithmTag = decltype(algorithmTagTag)::value_type;
							Benchmark(turns, slots.value(), AlgorithmTag{}, Tag<plf_colony_allocator<PrimitiveType>>{});
						});

						//Benchmark(turns, slots.value(), Tag<SequenceOtherTag<plf::colony<PrimitiveType>>>{}, Tag<void>{});
					}

					// Set-based algorithms
					//
					ForEachTag(Tag<
						SetTag<std::set<PrimitiveType>>,
						SetTag<std::unordered_set<PrimitiveType>>,
						SetTag<boost::container::flat_set<PrimitiveType>>,
						SetTag<stx::btree_set<PrimitiveType>>,
						SetTag<btree::btree_set<PrimitiveType>>,
						SetTag<google::sparse_hash_set<PrimitiveType>>,
						SetTag<google::dense_hash_set<PrimitiveType>>,
                        SetTag<tsl::hopscotch_set<PrimitiveType>>
					>{},
						[=](auto algorithmTagTag)
					{
						using AlgorithmTag = decltype(algorithmTagTag)::value_type;
						Benchmark(turns, slots.value(), AlgorithmTag{}, Tag<void>{});
					});

                    ForEachTag(Tag<
                        SetPlainPtrTag<std::set<PrimitiveType*, less_dereference>>,
                        SetPlainPtrTag<std::unordered_set<PrimitiveType*, hash_dereference, equal_dereference>>,
                        SetPlainPtrTag<boost::container::flat_set<PrimitiveType*, less_dereference>>,
                        SetPlainPtrTag<stx::btree_set<PrimitiveType*, less_dereference>>,
                        SetPlainPtrTag<btree::btree_set<PrimitiveType*, less_dereference>>,
                        SetPlainPtrTag<google::sparse_hash_set<PrimitiveType*, hash_dereference, equal_dereference>>,
                        SetPlainPtrTag<google::dense_hash_set<PrimitiveType*, hash_dereference, equal_dereference>>,
                        SetPlainPtrTag<tsl::hopscotch_set<PrimitiveType*, hash_dereference, equal_dereference>>
                    >{},
                        [=](auto algorithmTagTag)
                    {
                        using AlgorithmTag = decltype(algorithmTagTag)::value_type;
                        Benchmark(turns, slots.value(), AlgorithmTag{}, Tag<void>{});
                    });
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
			std::cout << sep << "time:s" << (slots.value() - 1);
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
