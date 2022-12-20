#pragma once

#include <declarations.h>

namespace st { // begin of namespace ============================

class Line {

  friend class Stree;

  public:

    Line(const LineType lt, const int x1, const int y1, const int x2, const int y2);

    Line(const Line& line) = default;
    Line(Line&& line) = default;

  private:

    LineType _type;
    int _x1;
    int _x2;
    int _y1;
    int _y2;
    
};

Line::Line(const LineType lt, const int x1, const int y1, const int x2, const int y2)
:_type{lt}, _x1{x1}, _y1{y1}, _x2{x2}, _y2{y2}
 {
}

} // end of namespace ===================================
