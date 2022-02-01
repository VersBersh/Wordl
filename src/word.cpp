#include <array>

#include "word.h"


std::array<Information, CountPotentialSolutions * CountPermittedGuesses> Word::_informationCache {};

// a constant to represent an uninitialized element in the cache
const Information Uninitialized = 1u << 15;

Word::_init::_init()
{
    Word::_informationCache.fill(Uninitialized);
}

Word::_init Word::_informationCacheInitializer {};


Word::Word(const std::string& word, int uniqueId)
    : _text(word), _uniqueId(uniqueId)
{
    _letters = std::vector<char>(word.cbegin(), word.cend());

    _letter_counts = std::map<char, int> {};
    for (auto letter : _letters)
    {
        auto pair = _letter_counts.emplace(letter, 0);
        pair.first->second++;
    }
}


/**
 *  Encode the Information returned from the given guess when the
 *  hidden word is target
 *
 *  The information is encoded in the bits of an unsigned integer.
 *  Take pairs of bits and encode Grey = 00, Yellow = 01, Green = 10.
 *  E.g {Green, Yellow, Grey, Grey, Yellow} -> 0b1001000001
 */
Information
Word::ComputeInformation(const Word* guess, const Word* target)
{
    auto index = CountPotentialSolutions * guess->_uniqueId + target->_uniqueId;
    auto cached = _informationCache[index];
    if (cached != Uninitialized)
        return cached;

    Information info = 0;

    auto commonLetters = guess->commonLettersWithMultiplicity(target);
    auto commonPositions = guess->commonPositions(target);

    // note: the bit encoding is actually reversed (like little
    // endian, but it's still a unique mapping)
    for (const auto& pos : commonPositions)
    {
        auto [letter, k] = pos;
        info |= 1u << (2*k + 1);
        commonLetters[letter] -= 1;
    }

    for (auto i = 0; i < 5; i++)
    {
        auto letter = guess->_letters[i];
        auto pair = commonLetters.find(letter);
        auto isCommonPosition = (info & (1u << (2*i+1))) != 0;
        if (pair != commonLetters.end() && pair->second != 0 && !isCommonPosition)
        {
            info |= 1u << (2*i);
            pair->second--;
        }
    }

    _informationCache[index] = info;
    return info;
}


/**
 *  Return the letters that are common between both words with the
 *  multiplicity (i.e. the smallest multiplicity between the two
 *  words).
 */
std::map<char, int>
Word::commonLettersWithMultiplicity(const Word* other) const
{
    std::map<char, int> result {};

    for (const auto& kv : other->_letter_counts)
    {
        auto letter = kv.first;

        auto it = _letter_counts.find(letter);
        if (it == _letter_counts.end())
            continue;

        result[letter] = std::min(kv.second, it->second);
    }

    return result;
}


/**
 *  Return the letters that occur in the same position in both words
 */
std::vector<std::tuple<char, int>>
Word::commonPositions(const Word* other) const
{
    std::vector<std::tuple<char, int>> result;

    auto index = 0;
    for (auto letter : other->_letters)
    {
        if (letter == _letters[index])
            result.emplace_back(std::forward_as_tuple(letter, index));
        index++;
    }

    return result;
}
