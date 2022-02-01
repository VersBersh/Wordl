#pragma once

#include <vector>

#include "word.h"


struct Node;

using WordSet = std::vector<const Word*>;
using Children = std::unordered_map<Information, Node>;


/**
 *  A node of the solution tree
 */
struct Node
{
    WordSet Words;
    const Word* BestGuess;
    float AverageDepth;
    std::size_t MaxDepth;
    Children Children;
};
