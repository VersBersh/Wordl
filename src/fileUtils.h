#pragma once

#include <vector>
#include <string>

#include "node.h"


std::vector<std::string> read_file(const std::string& path);

void WriteOutput(const Node& root, const std::string& path);
