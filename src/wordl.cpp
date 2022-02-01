#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <math.h>
#include <numeric>
#include <string>
#include <vector>

#include "fileUtils.h"
#include "node.h"
#include "word.h"


using Grouping = std::unordered_map<Information, WordSet>;


/**
 *  The size-metric.
 *
 *  Given a list of words, and a guess. The guess partitions the words
 *  into n groups. The size-metric asks for the average and maximum
 *  size of any of those groups.
 */
struct SizeResult
{
    const Word* Word;
    float AverageSize;
    std::size_t MaxSize;
    bool GuessIsPotentialSolution;
};


/**
 *  Get the SizeResult for a given list of words and a guess
 */
SizeResult
GetSizeResult(const WordSet& words, const Word* guess)
{
    bool guessIsPotentialSolution = false;
    auto counts = std::unordered_map<Information, std::size_t> {};
    for (auto target : words)
    {
        // if we guess the target word the game will end
        if (*target == *guess)
        {
            guessIsPotentialSolution = true;
            continue;
        }

        auto info = Word::ComputeInformation(guess, target);
        auto it = counts.emplace(info, 0);
        it.first->second++;
    }

    // the average size of the remaining groups should be adjusted to
    // account for whether the guess was one of the targets
    auto noWordsRemaining = static_cast<float>(words.size() - guessIsPotentialSolution);
    auto noGroups = counts.size() + guessIsPotentialSolution;
    auto avgSize = noWordsRemaining / noGroups;

    auto maxSize = std::max_element(counts.cbegin(),
                                    counts.cend(),
                                    [](const auto& left, const auto& right) {
                                        return left.second < right.second;
                                    })->second;

    return SizeResult{ guess, avgSize, maxSize, guessIsPotentialSolution };
}


/**
 *  Determine the topN best words according to the size-metric
 *
 *  Note: this function may return less than topN results if we can
 *  already determine that a result is optimal. Similarly we may need
 *  to return more than topN if more than this many words share the
 *  same size heuristic. It will not return more than hardMax.
 */
std::vector<SizeResult>
GetBestWordsBySize(const WordSet& potentialSolutions,
                   const WordSet& permittedGuesses,
                   std::size_t topN,
                   std::size_t hardMax)
{
    std::vector<SizeResult> results {};

    // small group optimisation: try to find a word in the group
    // itself that totally disambiguates all remaining words
    if (potentialSolutions.size() <= 10)
    {
        for (auto guess : potentialSolutions)
        {
            auto sizeRes = GetSizeResult(potentialSolutions, guess);
            if (sizeRes.MaxSize != 1)
                continue;

            results.push_back(sizeRes);
            return results;
        }
    }

    // otherwise we have to check every potential word
    for (auto guess : permittedGuesses)
    {
        auto sizeRes = GetSizeResult(potentialSolutions, guess);
        results.push_back(sizeRes);
    }

    // take the best topN + 1 guesses
    auto k = std::min(topN + 1, results.size());
    std::partial_sort(results.begin(), results.begin() + k, results.end(),
                      [](const auto& left, const auto& right) {
                          if (left.AverageSize == right.AverageSize)
                              return left.GuessIsPotentialSolution;
                          return left.AverageSize < right.AverageSize;
                      });

    if (results[0].MaxSize == 2)
    {
        // if the guess with the lowest average size also has a
        // maximum size of 2 then this solution is optimal and we
        // will only need to check that one.
        topN = 1;
    }
    else if (k >= 2 && results[k-2].AverageSize == results[k-1].AverageSize)
    {
        // if many words give the same average we need to test them
        // all, so we increase topN accordingly
        std::sort(results.begin(), results.end(),
                  [](const auto& left, const auto& right) {
                      if (left.AverageSize == right.AverageSize)
                          return left.GuessIsPotentialSolution;
                      return left.AverageSize < right.AverageSize;
                  });

        auto kth = results[k-1].AverageSize;
        auto it = std::find_if(results.crbegin(), results.crend(),
                               [kth](const auto& it) { return it.AverageSize == kth; });
        topN = std::min(hardMax, results.size() - std::distance(it, results.crend()));
    }

    // dummy required because std::vector::resize requires a default
    // constructor (but we always resize downward)
    SizeResult dummy { potentialSolutions[0], 0, 0 };
    results.resize(topN, dummy);

    return results;
}


/**
 *  Partition the words into N groups based on the information gained
 *  by the given guess (remove the guess from the target words if it's
 *  one of them)
 */
Grouping
SplitWordsByGuess(const WordSet& words, const Word* guess)
{
    Grouping groups {};
    for (auto target : words)
    {
        if (*target == *guess)
            continue;

        auto info = Word::ComputeInformation(guess, target);
        auto it = groups.emplace(info, WordSet{});
        if (it.second)
            it.first->second.reserve(words.size()/10);
        it.first->second.push_back(target);
    }

    auto sum = std::accumulate(groups.cbegin(), groups.cend(), 0,
                               [](int sum, const auto& it) {
                                   return sum + it.second.size();
                                });
    assert(sum == words.size() || sum == words.size() - 1);
    return groups;
}


/**
 *  Return the weighted average depth of a node with the given children
 */
float
AverageChildDepth(const WordSet& words, const Children& children)
{
    auto num = std::accumulate(children.cbegin(), children.cend(), 0.0f,
                               [](float sum, const auto& it){
                                   return sum + it.second.AverageDepth * it.second.Words.size();
                               });

    return 1 + num / words.size();
}


/**
 *  Return the maximum depth of a node with the given children
 */
std::size_t
MaxChildDepth(const Children& children)
{
    return 1 + std::max_element(children.cbegin(), children.cend(),
                                [](const auto& left, const auto& right) {
                                    return left.second.MaxDepth < right.second.MaxDepth;
                                })->second.MaxDepth;
}


/**
 *  Compute a quick lower bound of the average child depth for the
 *  given children.
 *
 *  A set of words X requires only one guess if |X| == 1, otherwise
 *  it requires at least 2*|X| - 1 guesses
 */
float
QuickLowerBound(const WordSet& words, const Grouping& groups)
{
    auto totalGuesses = std::accumulate(groups.cbegin(), groups.cend(), 0.0f,
                                        [](float guesses, const auto& it) {
                                            auto sz = it.second.size();
                                            return guesses + (sz == 1 ? 1 : (2*sz-1));
                                        });

    return 1 + totalGuesses / words.size();
}


/**
 *  Minimise the average depth of the node
 */
Node
Optimise(const WordSet& words,
         const WordSet& permittedGuesses,
         std::size_t topN,
         std::size_t hardMax,
         std::size_t maxTreeDepth)
{
    if (words.size() == 1)
    {
        return Node { words, words[0], 1, 1, {} };
    }
    if (words.size() == 2)
    {
        auto w0 = words[0];
        auto w1 = words[1];
        auto info0 = Word::ComputeInformation(w0, w0);
        auto info1 = Word::ComputeInformation(w0, w1);
        Children children {
            {info0, Node {{w0}, w0, 1, 1, {}} },
            {info1, Node {{w1}, w1, 1, 1, {}} },
        };
        return Node { words, w0, 1.5, 2, children };
    }

    auto bestBySize = GetBestWordsBySize(words, permittedGuesses, topN, hardMax);

    auto bestGuess = permittedGuesses[0];
    auto bestAvg = 1e6f;
    std::size_t bestMax = 1000;
    Children bestChildren {};

    for (const auto& sizeRes : bestBySize)
    {
        const Word* guess = sizeRes.Word;

        auto groups = SplitWordsByGuess(words, guess);

        // if any of the groups have more than 2 elements, but we only
        // have one more guess then we can prune this branch
        if (maxTreeDepth == 1 && sizeRes.MaxSize != 1)
            continue;

        float quickLowerBound = QuickLowerBound(words, groups);
        if (quickLowerBound > bestAvg)
            continue;

        Children children {};
        for (const auto& p : groups)
            children.insert({
                p.first,
                Optimise(p.second, permittedGuesses, topN, hardMax, maxTreeDepth - 1)});

        auto avg = AverageChildDepth(words, children);
        auto max = MaxChildDepth(children);

        if (avg < bestAvg || (avg == bestAvg && max < bestMax))
        {
            bestAvg = avg;
            bestGuess = guess;
            bestChildren = children;
            bestMax = max;
        }

        // the quick lower bound is optimal relative to the size
        if (bestAvg <= quickLowerBound)
            break;
    }

    return Node { words, bestGuess, bestAvg, bestMax, bestChildren };
}


/**
 *  Minimise the average depth of the node
 */
Node
Optimise(const Word* initialGuess,
         const WordSet& words,
         const WordSet& permittedGuesses,
         std::size_t topN,
         std::size_t hardMax,
         std::size_t maxTreeDepth)
{
    auto groups = SplitWordsByGuess(words, initialGuess);

    Children children {};
    for (const auto& p : groups)
    {
        auto node = Optimise(p.second, permittedGuesses, topN, hardMax, maxTreeDepth - 1);
        children.insert({ p.first, node });
    }

    auto avg = AverageChildDepth(words, children);
    auto max = MaxChildDepth(children);

    return Node { words, initialGuess, avg, max, children };
}


int main(int c, char* argv[])
{
    assert(c == 8);
    std::string firstGuess = argv[1];
    std::string solutionsFilePath = argv[2];
    std::string guessesFilePath = argv[3];
    auto topN = std::stoi(argv[4]);
    auto hardMax = std::stoi(argv[5]);
    auto maxTreeDepth = std::stoi(argv[6]);
    std::string outFile = argv[7];

    auto potentialSolutionsText = read_file(solutionsFilePath);
    auto permittedGuessesText = read_file(guessesFilePath);

    int unique_id = 0;
    std::vector<Word> potentialSolutions {};
    for (const auto& word : potentialSolutionsText)
        potentialSolutions.emplace_back(word, unique_id++);

    std::vector<Word> permittedGuesses(potentialSolutions);
    for (const auto& word : permittedGuessesText)
        permittedGuesses.emplace_back(word, unique_id++);

    std::vector<const Word*> pPotentialSolutions;
    pPotentialSolutions.reserve(potentialSolutions.size());
    std::transform(potentialSolutions.cbegin(),
                   potentialSolutions.cend(),
                   std::back_inserter(pPotentialSolutions),
                   [](const Word& word){ return &word; });

    std::vector<const Word*> pPermittedGuesses;
    pPermittedGuesses.reserve(permittedGuesses.size());
    std::transform(permittedGuesses.cbegin(),
                   permittedGuesses.cend(),
                   std::back_inserter(pPermittedGuesses),
                   [](const Word& word){ return &word; });

    assert(pPotentialSolutions.size() == CountPotentialSolutions);
    assert(pPermittedGuesses.size() == CountPermittedGuesses);

    auto initGuess = std::find_if(pPermittedGuesses.cbegin(), pPermittedGuesses.cend(),
                                  [&firstGuess](const auto& word) {
                                      return word->Text() == firstGuess;
                                  });

    assert(initGuess != pPermittedGuesses.end());

    using namespace std::chrono;
    time_point begin = steady_clock::now();

    auto root = Optimise(*initGuess,
                         pPotentialSolutions,
                         pPermittedGuesses,
                         topN,
                         hardMax,
                         maxTreeDepth);

    std::cout << root.BestGuess->Text() << " "
              << root.AverageDepth
              << " " << root.MaxDepth << "\n";

    time_point end = steady_clock::now();
    auto secs = duration_cast<seconds>(end - begin).count();
    std::cout << "Execution Time: "
              << secs / 60 << "m "
              << secs % 60 << "s\n";

    WriteOutput(root, outFile);
}
