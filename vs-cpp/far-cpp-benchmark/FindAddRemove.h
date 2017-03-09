
#pragma once

struct GameResult
{
	int64_t SumOfSizes;
};

// Algorithms
template<typename... T> struct PositionalTag : public Tag<T...> { };
template<typename... T> struct SequenceUnsortedTag : public Tag<T...> { };
template<typename... T> struct SequenceSortedTag : public Tag<T...> { };
template<typename... T> struct SequenceOtherTag : public Tag<T...> { };
template<typename... T> struct SetTag : public Tag<T...> { };

template<typename T>
struct PrimitiveAllocMethod
{
	using ElementType = T;
	using Less = std::less<T>;
	using Equal = std::equal_to<T>;
	using Hash = std::hash<T>;
};

template<typename F>
class Finalizer
{
	F f;
public:
	explicit Finalizer(F&& f_) : f{f_} {}
	~Finalizer() { f(); }
};

template<typename F>
Finalizer<F> Finalize(F&& f)
{
	return Finalizer<F>{std::forward<F>(f)};
}

template<typename Collection>
auto InitCollection(Collection&) { return 0; }

template<typename Collection, typename AllocatorType>
auto InitCollection(Collection&, AllocatorType&) { return 0; }

template<typename PrimitiveType, typename... Others>
auto InitCollection(google::sparse_hash_set<PrimitiveType, Others...>& c)
{
	c.set_deleted_key(PrimitiveType{});
    return 0;
}

template<typename AllocatorType, typename... CollectionParams>
auto InitCollection(google::sparse_hash_set<CollectionParams...>& c, AllocatorType& alloc)
{
	auto slot0{alloc.Alloc(typename AllocatorType::PrimitiveType{})};
    c.set_deleted_key(slot0);
	return Finalize([&alloc, slot0] () { alloc.Free(slot0); });
}

template<typename PrimitiveType, typename... Others>
auto InitCollection(google::dense_hash_set<PrimitiveType, Others...>& c)
{
	c.set_empty_key(PrimitiveType{});
	c.set_deleted_key(PrimitiveType{});
    return 0;
}

template<typename AllocatorType, typename... CollectionParams>
auto InitCollection(google::dense_hash_set<CollectionParams...>& c, AllocatorType& alloc)
{
	auto slot0{alloc.Alloc(typename AllocatorType::PrimitiveType{})};
    c.set_empty_key(slot0);
    c.set_deleted_key(slot0);
	return Finalize([&alloc, slot0] () { alloc.Free(slot0); });
}

template<typename RandomGenerator, typename UnknownAlgorithm, typename UnknownAllocator>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, UnknownAlgorithm unknownAlgorithm, Tag<UnknownAllocator>) = delete;

// Test the pseudo-random generator itself. Just generate {turns} values. Report sumOfSizes as a sum of all the numbers generated.
//
template<typename RandomGenerator>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, Tag<void>, Tag<void>)
{
	int64_t sumOfSizes{0};

	for (int turn{0}; turn < turns; ++turn) {
		sumOfSizes += randomGenerator();
	}

	return {sumOfSizes};
}

// The collection is a fixed-sized, pre-allocated array.
//
template<typename RandomGenerator, typename ElementType>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, PositionalTag<std::unique_ptr<ElementType[]>>, Tag<void>)
{
	auto collection{ std::make_unique<ElementType[]>(slots) };
	int64_t sumOfSizes{0};
	int64_t size{0};

	for (int turn{0}; turn < turns; ++turn)
	{
		auto& item{collection[static_cast<size_t>(randomGenerator())]};

		size += item ? -1 : 1;
		//size += 1 - 2 * item; <- Alternative, but performs similarily.
		item ^= 1;

		// This one is up to 35% slower:
		//	if (item != 0) {
		//		item = 0;
		//		--size;
		//	} else {
		//		item = 1;
		//		++size;
		//	}

		sumOfSizes += size;
	}

	return {sumOfSizes};
}

template<typename RandomGenerator, typename BitMaskType>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, PositionalTag<std::unique_ptr<bool[]>, BitMaskType>, Tag<void>)
{
	constexpr int64_t maskBitSize{ 8 * sizeof(BitMaskType) };
	auto collection{ std::make_unique<BitMaskType[]>((slots + maskBitSize - 1) / maskBitSize) };
	int64_t sumOfSizes{0};
	int64_t size{0};

	for (int turn{0}; turn < turns; ++turn)
	{
		const auto slot{randomGenerator()};
		const auto index{slot / maskBitSize};
		auto bitShift{static_cast<uint8_t>(slot - index * maskBitSize)};
		const auto mask{static_cast<BitMaskType>(BitMaskType{1} << bitShift)};

		auto& item{collection[static_cast<size_t>(index)]};
		size += ((item & mask) != 0) ? -1 : 1;
		item ^= mask;

		sumOfSizes += size;
	}

	return {sumOfSizes};
}

template<typename RandomGenerator>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, PositionalTag<std::vector<bool>>, Tag<void>)
{
	auto collection{std::vector<bool>(slots)};
	int64_t sumOfSizes{0};
	int64_t size{0};

	for (int turn{0}; turn < turns; ++turn)
	{
		auto item{collection[static_cast<size_t>(randomGenerator())]};

		size += item ? -1 : 1;
		item = item ^ 1;

		sumOfSizes += size;
	}

	return {sumOfSizes};
}

template<typename RandomGenerator, int Slots>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, PositionalTag<std::bitset<Slots>>, Tag<void>)
{
	auto collection{std::bitset<Slots>()};
	int64_t sumOfSizes{0};
	int64_t size{0};

	for (int turn{0}; turn < turns; ++turn)
	{
		auto item{collection[static_cast<size_t>(randomGenerator())]};

		size += item ? -1 : 1;
		item = item ^ 1;

		sumOfSizes += size;
	}

	return {sumOfSizes};
}

template<typename RandomGenerator, typename SequenceType, typename PrimitiveType>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, SequenceUnsortedTag<SequenceType>, Tag<PrimitiveAllocMethod<PrimitiveType>>)
{
	auto collection{SequenceType{}};
	int64_t sumOfSizes{0};

	for (int turn{0}; turn < turns; ++turn)
	{
		auto slot{static_cast<PrimitiveType>(randomGenerator())};
		auto finding{ std::find(std::begin(collection), std::end(collection), slot) };
		if (finding != std::end(collection)) {
			collection.erase(finding);
		} else {
			collection.push_back(std::move(slot));
		}
		sumOfSizes += collection.size();
	}

	return {sumOfSizes};
}

template<typename RandomGenerator, typename SequenceType, typename AllocatorType>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, SequenceUnsortedTag<SequenceType>, Tag<AllocatorType>)
{
	using PrimitiveType = AllocatorType::PrimitiveType;
	using ElementType = AllocatorType::ElementType;

	auto allocator{AllocatorType{}};
	auto collection{SequenceType{}};
	int64_t sumOfSizes{0};

	for (int turn{0}; turn < turns; ++turn)
	{
		auto slot{static_cast<PrimitiveType>(randomGenerator())};
		auto finding{ std::find_if(std::begin(collection), std::end(collection), [=] (const auto& elem){ return *elem == slot; }) };
		if (finding != std::end(collection)) {
			allocator.Free(*finding);
			collection.erase(finding);
		} else {
			collection.push_back(allocator.Alloc(std::move(slot)));
		}
		sumOfSizes += collection.size();
	}

	return {sumOfSizes};
}

// The collection has vector-conformant API, i.e. lower_bound(), push_back(), insert(), erase(), size().
//
template<typename RandomGenerator, typename SequenceType, typename PrimitiveType>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, SequenceSortedTag<SequenceType>, Tag<PrimitiveAllocMethod<PrimitiveType>>)
{
	auto collection{SequenceType{}};
	int64_t sumOfSizes{0};

	for (int turn{0}; turn < turns; ++turn)
	{
		auto slot{static_cast<PrimitiveType>(randomGenerator())};
		auto finding{ std::lower_bound(std::begin(collection), std::end(collection), slot) };
		if (finding == std::end(collection)) {
			collection.push_back(std::move(slot));
		} else if (*finding == slot) {
			collection.erase(finding);
		} else {
			collection.insert(finding, std::move(slot));
		}
		sumOfSizes += collection.size();
	}

	return {sumOfSizes};
}

template<typename RandomGenerator, typename SequenceType, typename AllocatorType>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, SequenceSortedTag<SequenceType>, Tag<AllocatorType>)
{
	using ElementType = AllocatorType::ElementType;
	using PrimitiveType = AllocatorType::PrimitiveType;

	auto allocator{AllocatorType{}};
	auto collection{SequenceType{}};
	int64_t sumOfSizes{0};

	for (int turn{0}; turn < turns; ++turn)
	{
		auto slot{static_cast<PrimitiveType>(randomGenerator())};
		auto finding{ std::lower_bound(std::begin(collection), std::end(collection), &slot, [] (const auto& e1, const auto& e2) { return *e1 < *e2; }) };
		if (finding == std::end(collection)) {
			collection.push_back(allocator.Alloc(std::move(slot)));
		} else {
			const auto stored{*finding};
			if (*stored == slot) {
				allocator.Free(stored);
				collection.erase(finding);
			} else {
				collection.insert(finding, allocator.Alloc(std::move(slot)));
			}
		}
		sumOfSizes += collection.size();
	}

	return {sumOfSizes};
}

// The collection has set-conformant API, i.e. insert(), erase(), size().
//
template<typename RandomGenerator, typename SetCollection, typename PrimitiveType>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, SetTag<SetCollection>, Tag<PrimitiveAllocMethod<PrimitiveType>>)
{
	auto collection{SetCollection{}};
	auto collectionAux{InitCollection(collection)};
    int64_t sumOfSizes{0};

	for (int turn{0}; turn < turns; ++turn)
	{
		auto insertion{collection.insert(static_cast<PrimitiveType>(randomGenerator()))};
		if (!insertion.second) {
			collection.erase(insertion.first);
		}
		sumOfSizes += collection.size();
	}

	return {sumOfSizes};
}

template<typename RandomGenerator, typename SetCollection, typename AllocatorType>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, SetTag<SetCollection>, Tag<AllocatorType>)
{
	using PrimitiveType = AllocatorType::PrimitiveType;
	using ElementType = AllocatorType::ElementType;

	auto allocator{AllocatorType{}};
	auto collection{SetCollection{}};
	auto collectionAux{InitCollection(collection, allocator)};
    int64_t sumOfSizes{0};

	auto slotAllocation{allocator.Alloc()};

	for (int turn{0}; turn < turns; ++turn)
	{
        *slotAllocation = static_cast<PrimitiveType>(randomGenerator());
		auto insertion{collection.insert(slotAllocation)};
        if (!insertion.second) {
            const auto deletedPtr{*insertion.first};
            collection.erase(insertion.first);
			allocator.Free(deletedPtr);
            // slotAllocation shall be reused in the next iteration.
        } else {
            slotAllocation = allocator.Alloc();
        }
        sumOfSizes += collection.size();
    }

	allocator.Free(slotAllocation);
    return {sumOfSizes};
}
