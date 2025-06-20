#pragma once
#include <string>
#include <functional>
#include <sstream>
#include <iomanip>
inline std::string sha1(const std::string& data) {
  size_t h = std::hash<std::string>{}(data);
  std::stringstream ss;
  ss << "I6" << std::hex << std::setw(8) << std::setfill('0') << (h & 0xFFFFFFFF);
  return ss.str();
}
