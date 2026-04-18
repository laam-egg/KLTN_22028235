#pragma once

#include <string>

std::wstring from_utf8(std::string const& s);
std::string to_utf8(std::wstring const& w);
