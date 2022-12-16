#pragma once
#include <src/utility.hpp>
#include <src/pin.hpp>
#include <declarations.h>
#include <flute/flute.h>

namespace st { // begin of namespace ============================

class Stree {

  public:

    Stree(const std::filesystem::path& input);

    void apply();

  private:

    void _parse();

    std::filesystem::path _input;
    int _num_pins;
    std::vector<Pin> _pins;
};

Stree::Stree(const std::filesystem::path& input): _input{input} {
  _parse();
}

void Stree::apply() {
  std::vector<Flute::DTYPE> xs;
  std::vector<Flute::DTYPE> ys;
  xs.reserve(_num_pins);
  ys.reserve(_num_pins);

  for(auto&& p: _pins) {
    xs.push_back(p._x);
    ys.push_back(p._y);
  }

  Flute::flute(10, xs.data(), ys.data(), 10);
}

void Stree::_parse() {
  auto sstream = read_file_to_sstream(_input);
  std::string line;

  // boundary
  std::getline(sstream, line);

  // number of pins
  {
    std::getline(sstream, line);
    std::stringstream line_stream(line);
    std::string token;
    std::getline(line_stream, token, ' ');
    std::getline(line_stream, token, ' ');
    std::getline(line_stream, token, ' ');
    _num_pins = std::stoi(token);
    _pins.reserve(_num_pins);
  }

  while(std::getline(sstream, line)) {
    std::stringstream line_stream(line);
    std::string token;
    // name
    std::getline(line_stream, token, ' ');
    std::getline(line_stream, token, ' ');
    std::string pname{token};

    // coordinate
    std::getline(line_stream, token, ' ');
    std::stringstream token_stream(token);
    char tmp;
    int x, y;
    token_stream >> tmp >> x >> tmp >> y >> tmp;
    _pins.emplace_back(pname, x, y);

    //std::cerr << "pin: " << pname << " (x, y): " << x << ", " << y << "\n";

  }
}

} // end of namespace ===================================
