#pragma once
#include <declarations.h>

namespace fp { // begin of namespace ====================================

class Terminal {

  friend class SP;

  public:

    Terminal(
      const std::string& name,
      const size_t id,
      const size_t x,
      const size_t y
    );

  private:

    std::string _name;
    size_t _id;
    size_t _x;
    size_t _y;
};

Terminal::Terminal(
  const std::string& name,
  const size_t id,
  const size_t x,
  const size_t y
): _name{name}, _id{id}, _x{x}, _y{y} {
}

} // end of namespace ===================================================
