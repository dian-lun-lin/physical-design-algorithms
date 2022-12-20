#pragma once
#include <src/utility.hpp>
#include <src/pin.hpp>
#include <src/line.hpp>
#include <declarations.h>
#include <flute/flute.h>


namespace st { // begin of namespace ============================

class Stree {

  public:

    Stree(const std::filesystem::path& input);

    void apply();

    void dump(std::ostream& os);

  private:

    void _parse();


    std::filesystem::path _input;
    int _num_pins;
    std::vector<Pin> _pins;
    std::vector<Line> _lines;
    size_t _wirelength{0};
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
  Flute::readLUT();

  auto tree = Flute::flute(_num_pins, xs.data(), ys.data(), 100);
  //Flute::plottree(tree);
  //Flute::printtree(tree);
  //Flute::write_svg(tree, "tmp.svg");

  // store
  for (auto i = 0; i < 2 * tree.deg - 2; i++) {
    int x1 = tree.branch[i].x;
    int y1 = tree.branch[i].y;
    int x2 = tree.branch[tree.branch[i].n].x;
    int y2 = tree.branch[tree.branch[i].n].y;

    if(x1 == x2 && y1 == y2) {
      // do nothing
    }
    else if(x1 == x2 && y1 != y2) {
      if(y1 > y2) { 
        _lines.emplace_back(
          LineType::VLINE,
          x1,
          y2,
          x1,
          y1
        );
        _wirelength += y1 - y2;
      }
      else {
        _lines.emplace_back(
          LineType::VLINE,
          x1,
          y1,
          x1,
          y2
        );
        _wirelength += y2 - y1;
      }
    }
    else if(x1 != x2 && y1 == y2) {
      if(x1 > x2) {
        _lines.emplace_back(
          LineType::HLINE,
          x2,
          y1,
          x1,
          y1
        );
        _wirelength += x1 - x2;
      }
      else {
        _lines.emplace_back(
          LineType::HLINE,
          x1,
          y1,
          x2,
          y1
        );
        _wirelength += x2 - x1;
      }

    }
    else {
      // split into two lines
      if(y1 > y2) { 
        _lines.emplace_back(
          LineType::VLINE,
          x1,
          y2,
          x1,
          y1
        );
        _wirelength += y1 - y2;
      }
      else {
        _lines.emplace_back(
          LineType::VLINE,
          x1,
          y1,
          x1,
          y2
        );
        _wirelength += y2 - y1;
      }

      if(x1 > x2) {
        _lines.emplace_back(
          LineType::HLINE,
          x2,
          y1,
          x1,
          y1
        );
        _wirelength += x1 - x2;
      }
      else {
        _lines.emplace_back(
          LineType::HLINE,
          x1,
          y1,
          x2,
          y1
        );
        _wirelength += x2 - x1;
      }
    }

  }
}

void Stree::dump(std::ostream& os) {
  os << "NumRoutedPins = " << _num_pins << "\n";
  os << "WireLength = " << _wirelength << "\n";

  for(auto&& line: _lines) {
    auto lt = line._type;
    switch(lt) {
      case LineType::VLINE:
        os << "V-line (" << line._x1 << "," << line._y1 << ") " << "(" << line._x1 << "," << line._y2 << ")\n";
        break;
      case LineType::HLINE:
        os << "H-line (" << line._x1 << "," << line._y1 << ") " << "(" << line._x2 << "," << line._y1 << ")\n";
        break;
      default:
        assert(false);
    }
  }
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
