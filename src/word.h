#pragma once

#include <array>
#include <map>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>


using Information = unsigned int;

inline constexpr std::size_t CountPotentialSolutions = 2315;
inline constexpr std::size_t CountPermittedGuesses = 12972;


class Word
{
    public:
        Word(const std::string& word, int unique_id);

        std::string Text() const { return _text; };
        int GetUniqueId() const { return _uniqueId; };

        static Information ComputeInformation(const Word* guess, const Word* target);

        bool operator==(const Word& other) const { return _uniqueId == other._uniqueId; }

    private:
        std::map<char, int> commonLettersWithMultiplicity(const Word* other) const;
        std::vector<std::tuple<char, int>> commonPositions(const Word* other) const;

        std::string _text;
        int _uniqueId;
        std::vector<char> _letters;
        std::map<char, int> _letter_counts;

        static std::array<Information, CountPotentialSolutions * CountPermittedGuesses> _informationCache;
        static class _init
        {
            public:
                _init();
        } _informationCacheInitializer;
};


namespace std
{
    template<>
    struct hash<std::tuple<int, int>>
    {
        std::size_t operator()(const std::tuple<int, int>& tup) const
        {
            auto [t0, t1] = tup;
            return (t0 * 377) ^ t1;
        }
    };
}
