#pragma once

#include <filesystem>
#include <fstream>


inline
std::stringstream read_file_to_sstream(const std::filesystem::path& input_path) {
  using namespace std::literals::string_literals;

  std::ifstream ifs{input_path};

  if(!ifs) {
    throw std::runtime_error("cannot open the file"s + input_path.c_str());
  }

  std::stringstream sstream;

  sstream << ifs.rdbuf();
  return sstream;
}
