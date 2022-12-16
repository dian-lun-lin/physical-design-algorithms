#pragma once

#include <declarations.h>

namespace st { // begin of namespace ============================

class Pin {

  friend class Stree;

  public:

    Pin(const std::string& name, const int x, const int y);

  private:

    const std::string _name;
    const int _x;
    const int _y;
    
};

Pin::Pin(const std::string& name, const int x, const int y)
:_name{name}, _x{x}, _y{y}
 {
}

} // end of namespace ===================================
