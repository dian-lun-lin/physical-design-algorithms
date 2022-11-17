#pragma once
#include <declarations.h>

namespace fp { // begin of namespace ====================================


class Block {
  friend class SP;

  public:
    Block(
      const std::string& name,
      const size_t id,
      const size_t height,
      const size_t width
    );

    Block(const Block& bk) = default;
    Block(Block&& bk) = default;

    Block& operator= (const Block& bk) = default;
    Block& operator= (Block&& bk) = default;
  
    ~Block() = default;

    void set_coordinate(
      size_t x1,
      size_t y1,
      size_t x2,
      size_t y2
    );

  private:

    std::vector<Block*> _hconnects;
    std::vector<Block*> _vconnects;
    std::string _name;
    size_t _id;
    size_t _height;
    size_t _width;

    size_t _x1;
    size_t _y1;
    size_t _x2;
    size_t _y2;
    bool _locked{false};

    
};

Block::Block(
  const std::string& name,
  const size_t id,
  const size_t height,
  const size_t width
):_name{name}, _id{id}, _height{height}, _width{width} {
}

void Block::set_coordinate(
  size_t x1,
  size_t y1,
  size_t x2,
  size_t y2
) {
  _x1 = x1;
  _y1 = y1;
  _x2 = x2;
  _y2 = y2;
}


} // end of namespace ===================================================
