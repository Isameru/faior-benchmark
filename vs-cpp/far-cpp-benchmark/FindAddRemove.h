
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

template<typename T> struct less : public std::less<T> { };
template<typename T> struct hash : public std::hash<T> { };

template<typename ElementType>
struct less<std::unique_ptr<ElementType>>
{
	bool operator()(const std::unique_ptr<ElementType>& a, const std::unique_ptr<ElementType>& b) const
	{
		return *a < *b;
	}
};

template<typename ElementType>
struct less<std::shared_ptr<ElementType>>
{
	bool operator()(const std::shared_ptr<ElementType>& a, const std::shared_ptr<ElementType>& b) const
	{
		return *a < *b;
	}
};

template<typename ElementType>
bool operator==(const std::unique_ptr<ElementType>& a, const std::unique_ptr<ElementType>& b)
{
	return *a == *b;
}

template<typename ElementType>
bool operator!=(const std::unique_ptr<ElementType>& a, const std::unique_ptr<ElementType>& b)
{
	return *a != *b;
}

template<typename ElementType>
bool operator==(const std::shared_ptr<ElementType>& a, const std::shared_ptr<ElementType>& b)
{
	return *a == *b;
}

template<typename ElementType>
bool operator!=(const std::shared_ptr<ElementType>& a, const std::shared_ptr<ElementType>& b)
{
	return *a != *b;
}

template<class ElementType>
struct hash<std::unique_ptr<ElementType>>
{
	std::size_t operator()(const std::unique_ptr<ElementType>& p) const
	{
		return std::hash<ElementType>{}(*p);
	}
};

template<class ElementType>
struct hash<std::shared_ptr<ElementType>>
{
	std::size_t operator()(const std::shared_ptr<ElementType>& p) const
	{
		return std::hash<ElementType>{}(*p);
	}
};

template<typename RandomGenerator, typename UnknownAlgorithm>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, UnknownAlgorithm unknownAlgorithm) = delete;

// Test the pseudo-random generator itself. Just generate {turns} values. Report sumOfSizes of 0.
//
template<typename RandomGenerator>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, Tag<void>)
{
	int64_t sumOfSizes { 0 };

	for (int turn = 0; turn < turns; ++turn)
	{
		sumOfSizes += randomGenerator();
	}

	return { sumOfSizes };
}

// The collection is a fixed-sized, pre-allocated array.
//
template<typename RandomGenerator, typename ElementType>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, PositionalTag<std::unique_ptr<ElementType[]>>)
{
	auto collection = std::make_unique<ElementType[]>(slots);
	int64_t sumOfSizes { 0 };
	int64_t size { 0 };

	for (int turn = 0; turn < turns; ++turn)
	{
		auto& item = collection[static_cast<size_t>(randomGenerator())];

		size += item ? -1 : 1;
		//size += 1 - 2 * item; <- Alternative, but performs similarily.
		item ^= 1;

		// This one is up to 35% slower:
		//	if (item != 0)
		//	{
		//		item = 0;
		//		--size;
		//	}
		//	else
		//	{
		//		item = 1;
		//		++size;
		//	}

		sumOfSizes += size;
	}

	return { sumOfSizes };
}

template<typename RandomGenerator, typename BitMaskType>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, PositionalTag<std::unique_ptr<bool[]>, BitMaskType>)
{
	constexpr auto maskBitSize = 8 * sizeof(BitMaskType);

	auto collection = std::make_unique<BitMaskType[]>((slots + maskBitSize - 1) / maskBitSize);
	int64_t sumOfSizes { 0 };
	int64_t size { 0 };

	for (int turn = 0; turn < turns; ++turn)
	{
		const auto slot = randomGenerator();

		const auto index = slot / maskBitSize;
		const BitMaskType mask = static_cast<BitMaskType>(1) << (slot - index * maskBitSize);

		auto& item = collection[static_cast<size_t>(index)];
		size += ((item & mask) != 0) ? -1 : 1;
		item ^= mask;

		sumOfSizes += size;
	}

	return { sumOfSizes };
}

template<typename RandomGenerator>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, PositionalTag<std::vector<bool>>)
{
	auto collection = std::vector<bool>(slots);
	int64_t sumOfSizes { 0 };
	int64_t size { 0 };

	for (int turn = 0; turn < turns; ++turn)
	{
		auto item = collection[static_cast<size_t>(randomGenerator())];

		size += item ? -1 : 1;
		item = item ^ 1;

		sumOfSizes += size;
	}

	return { sumOfSizes };
}

template<typename RandomGenerator, int Slots>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, PositionalTag<std::bitset<Slots>>)
{
	auto collection = std::bitset<Slots>();
	int64_t sumOfSizes { 0 };
	int64_t size { 0 };

	for (int turn = 0; turn < turns; ++turn)
	{
		auto item = collection[static_cast<size_t>(randomGenerator())];

		size += item ? -1 : 1;
		item = item ^ 1;

		sumOfSizes += size;
	}

	return { sumOfSizes };
}

template<typename RandomGenerator, typename SequenceType>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, SequenceUnsortedTag<SequenceType>)
{
	auto collection = SequenceType{ };
	int64_t sumOfSizes { 0 };

	for (int turn = 0; turn < turns; ++turn)
	{
		auto slot = randomGenerator();

		auto finding = std::find(std::begin(collection), std::end(collection), slot);
		if (finding != std::end(collection))
		{
			collection.erase(finding);
		}
		else
		{
			collection.push_back(std::move(slot));
		}

		sumOfSizes += collection.size();
	}

	return { sumOfSizes };
}

// The collection has vector-conformant API, i.e. lower_bound(), push_back(), insert(), erase(), size().
//
template<typename RandomGenerator, typename SequenceType>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, SequenceSortedTag<SequenceType>)
{
	auto collection = SequenceType{ };
	int64_t sumOfSizes { 0 };

	for (int turn = 0; turn < turns; ++turn)
	{
		auto slot = static_cast<SequenceType::value_type>(randomGenerator());

		auto finding = std::lower_bound(std::begin(collection), std::end(collection), slot, less<SequenceType::value_type>{});
		if (finding == std::end(collection))
		{
			collection.push_back(std::move(slot));
		}
		else if (*finding == slot)
		{
			collection.erase(finding);
		}
		else
		{
			collection.insert(finding, std::move(slot));
		}

		sumOfSizes += collection.size();
	}

	return { sumOfSizes };
}

template<typename RandomGenerator, typename ElementType>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, SequenceOtherTag<plf::colony<ElementType>>)
{
	auto collection = plf::colony<ElementType>{ };
	int64_t sumOfSizes { 0 };

	for (int turn = 0; turn < turns; ++turn)
	{
		auto slot = randomGenerator();

		auto finding = std::find(std::begin(collection), std::end(collection), slot);
		if (finding != std::end(collection))
		{
			collection.erase(finding);
		}
		else
		{
			collection.insert(std::move(slot));
		}

		sumOfSizes += collection.size();
	}

	return { sumOfSizes };
}

// The collection has set-conformant API, i.e. find(), insert(), erase(), size().
//
template<typename RandomGenerator, typename SetCollection>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, SetTag<SetCollection>)
{
	auto collection = SetCollection{ };
	int64_t sumOfSizes { 0 };

	for (int turn = 0; turn < turns; ++turn)
	{
		auto insertion = collection.insert(randomGenerator());
		if (!insertion.second)
		{
			collection.erase(insertion.first);
		}

		sumOfSizes += collection.size();
	}

	return { sumOfSizes };
}
