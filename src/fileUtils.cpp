#include <fstream>
#include <unordered_set>

#include "fileUtils.h"


std::vector<std::string>
read_file(const std::string& path)
{
    std::ifstream infile(path);

    std::vector<std::string> result {};
    std::string line;
    while (std::getline(infile, line))
        result.push_back(line);

    return result;
}


void
Write(const Node& node, std::ofstream& file, const std::string& cur)
{
    if (node.Children.size() == 0)
    {
        file << cur << "\n";
        return;
    }

    for (const auto& child : node.Children)
    {
        Write(child.second, file, cur + ", " + child.second.BestGuess->Text());
    }
}


void
WriteOutput(const Node& root,
            const std::string& filepath)
{
    std::ofstream file(filepath);
    std::string cur = root.BestGuess->Text();
    Write(root, file, cur);
}
