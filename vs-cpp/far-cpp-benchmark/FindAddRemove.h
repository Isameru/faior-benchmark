
#pragma once

struct GameResult
{
	int64_t SumOfSizes;
};

// Algorithms
template<typename... T> struct PositionalTag : public Tag<T...> { };
template<typename... T> struct SequenceUnsortedTag : public Tag<T...> { };
template<typename... T> struct SequenceUnsortedPtrTag : public Tag<T...> { };
template<typename... T> struct SequenceUnsortedPlainPtrTag : public Tag<T...> { };
template<typename... T> struct SequenceSortedTag : public Tag<T...> { };
template<typename... T> struct SequenceSortedPtrTag : public Tag<T...> { };
template<typename... T> struct SequenceSortedPlainPtrTag : public Tag<T...> { };
template<typename... T> struct SequenceOtherTag : public Tag<T...> { };
template<typename... T> struct SetTag : public Tag<T...> { };
template<typename... T> struct SetPtrTag : public Tag<T...> { };
template<typename... T> struct SetPlainPtrTag : public Tag<T...> { };

/*
	The following code is support for unique_ptr and shared_ptr:

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
*/

template<typename RandomGenerator, typename UnknownAlgorithm, typename UnknownAllocator>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, UnknownAlgorithm unknownAlgorithm, Tag<UnknownAllocator>) = delete;

// Test the pseudo-random generator itself. Just generate {turns} values. Report sumOfSizes as a sum of all the numbers generated.
//
template<typename RandomGenerator>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, Tag<void>, Tag<void>)
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
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, PositionalTag<std::unique_ptr<ElementType[]>>, Tag<void>)
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
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, PositionalTag<std::unique_ptr<bool[]>, BitMaskType>, Tag<void>)
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
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, PositionalTag<std::vector<bool>>, Tag<void>)
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
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, PositionalTag<std::bitset<Slots>>, Tag<void>)
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
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, SequenceUnsortedTag<SequenceType>, Tag<void>)
{
	auto collection = SequenceType{ };
	int64_t sumOfSizes { 0 };

	for (int turn = 0; turn < turns; ++turn)
	{
		auto slot = static_cast<SequenceType::value_type>(randomGenerator());

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

template<typename RandomGenerator, typename SequenceType, typename AllocatorType>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, SequenceUnsortedPtrTag<SequenceType>, Tag<AllocatorType>)
{
	using ElementType = SequenceType::value_type;
	using PrimitiveType = AllocatorType::value_type;

	auto allocator = AllocatorType{ };
	auto collection = SequenceType{ };
	int64_t sumOfSizes { 0 };

	for (int turn = 0; turn < turns; ++turn)
	{
		const auto slot = static_cast<PrimitiveType>(randomGenerator());

		auto finding = std::find_if(std::begin(collection), std::end(collection), [=](const auto elem){ return *elem == slot; });
		if (finding != std::end(collection))
		{
			allocator.deallocate(*finding, 1);
			collection.erase(finding);
		}
		else
		{
			auto p = allocator.allocate(1);
			*p = slot;
			collection.push_back(std::move(p));
		}

		sumOfSizes += collection.size();
	}

	return { sumOfSizes };
}

template<typename RandomGenerator, typename SequenceType>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, SequenceUnsortedPlainPtrTag<SequenceType>, Tag<void>)
{
	using ElementType = SequenceType::value_type;
	using PrimitiveType = std::remove_pointer<ElementType>::type;

	auto collection = SequenceType{ };
	int64_t sumOfSizes { 0 };

	for (int turn = 0; turn < turns; ++turn)
	{
		const auto slot = static_cast<PrimitiveType>(randomGenerator());

		auto finding = std::find_if(std::begin(collection), std::end(collection), [=](const auto* elem){ return *elem == slot; });
		if (finding != std::end(collection))
		{
			delete *finding;
			collection.erase(finding);
		}
		else
		{
			collection.push_back(new PrimitiveType{slot});
		}

		sumOfSizes += collection.size();
	}

	return { sumOfSizes };
}

// The collection has vector-conformant API, i.e. lower_bound(), push_back(), insert(), erase(), size().
//
template<typename RandomGenerator, typename SequenceType>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, SequenceSortedTag<SequenceType>, Tag<void>)
{
	auto collection = SequenceType{ };
	int64_t sumOfSizes { 0 };

	for (int turn = 0; turn < turns; ++turn)
	{
		const auto slot = static_cast<SequenceType::value_type>(randomGenerator());

		auto finding = std::lower_bound(std::begin(collection), std::end(collection), slot /*, less<SequenceType::value_type>{}*/);
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

template<typename RandomGenerator, typename SequenceType, typename AllocatorType>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, SequenceSortedPtrTag<SequenceType>, Tag<AllocatorType>)
{
	using ElementType = SequenceType::value_type;
	using PrimitiveType = AllocatorType::value_type;

	auto allocator = AllocatorType{ };
	auto collection = SequenceType{ };
	int64_t sumOfSizes { 0 };

	for (int turn = 0; turn < turns; ++turn)
	{
		const auto slot = static_cast<PrimitiveType>(randomGenerator());

		auto finding = std::lower_bound(std::begin(collection), std::end(collection), &slot, [](const auto e1, const auto e2) { return *e1 < *e2; });
		if (finding == std::end(collection))
		{
			auto p = allocator.allocate(1);
			*p = slot;
			collection.push_back(p);
		}
		else
		{
			auto stored = *finding;

			if (*stored == slot)
			{
				allocator.deallocate(stored, 1);
				collection.erase(finding);
			}
			else
			{
				auto p = allocator.allocate(1);
				*p = slot;
				collection.insert(finding, std::move(p));
			}
		}

		sumOfSizes += collection.size();
	}

	return { sumOfSizes };
}

template<typename RandomGenerator, typename SequenceType>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, SequenceSortedPlainPtrTag<SequenceType>, Tag<void>)
{
	using ElementType = SequenceType::value_type;
	using PrimitiveType = std::remove_pointer<ElementType>::type;

	auto collection = SequenceType{ };
	int64_t sumOfSizes { 0 };

	for (int turn = 0; turn < turns; ++turn)
	{
		const auto slot = static_cast<PrimitiveType>(randomGenerator());

		auto finding = std::lower_bound(std::begin(collection), std::end(collection), &slot, [](const auto* e1, const auto* e2) { return *e1 < *e2; });
		if (finding == std::end(collection))
		{
			collection.push_back(new PrimitiveType{slot});
		}
		else
		{
			auto* stored = *finding;

			if (*stored == slot)
			{
				delete *finding;
				collection.erase(finding);
			}
			else
			{
				collection.insert(finding, new PrimitiveType{slot});
			}
		}

		sumOfSizes += collection.size();
	}

	return { sumOfSizes };
}

/*
	The following implementation was meant to be working with plf::colony, but this case is not valuable.

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
*/

// The collection has set-conformant API, i.e. find(), insert(), erase(), size().
//
template<typename RandomGenerator, typename SetCollection>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, SetTag<SetCollection>, Tag<void>)
{
	auto collection = SetCollection{ };
	int64_t sumOfSizes { 0 };

	for (int turn = 0; turn < turns; ++turn)
	{
		auto insertion = collection.insert(static_cast<SetCollection::value_type>(randomGenerator()));
		if (!insertion.second)
		{
			collection.erase(insertion.first);
		}

		sumOfSizes += collection.size();
	}

	return { sumOfSizes };
}

template<typename RandomGenerator, typename SetCollection>
GameResult PlayFindAddRemove(int turns, int slots, RandomGenerator randomGenerator, SetPlainPtrTag<SetCollection>, Tag<void>)
{
    using ElementType = SetCollection::value_type;
    using PrimitiveType = std::remove_pointer<ElementType>::type;

    auto collection = SetCollection{ };
    int64_t sumOfSizes { 0 };
    PrimitiveType* slotAllocation = new PrimitiveType{};

    for (int turn = 0; turn < turns; ++turn)
    {
        *slotAllocation = static_cast<PrimitiveType>(randomGenerator());

        auto insertion = collection.insert(slotAllocation);
        if (!insertion.second)
        {
            auto* deletedPtr = *insertion.first;
            collection.erase(insertion.first);
            delete deletedPtr;
            // slotAllocation shall be reused in the next iteration.
        }
        else
        {
            slotAllocation = new PrimitiveType{};
        }

        sumOfSizes += collection.size();
    }

    delete slotAllocation;

    return { sumOfSizes };
}
