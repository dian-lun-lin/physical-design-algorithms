#pragma once
#include <declarations.h>

namespace fp { // begin of namespace ====================================


class Net {
  friend class SP;

  public:

    Net(
      const std::vector<Block*>& blocks,
      const std::vector<Terminal*>& terminals,
      size_t degree
    );

    Net(const Net& net) = default;
    Net(Net&& net) = default;

    Net& operator= (const Net& net) = default;
    Net& operator= (Net&& net) = default;
  
    ~Net() = default;

    void set_length(size_t length);

  private:

    std::vector<Block*> _blocks;
    std::vector<Terminal*> _terminals;
    size_t _degree;
    float _length;
};

Net::Net(
  const std::vector<Block*>& blocks,
  const std::vector<Terminal*>& terminals,
  size_t degree
): _blocks{blocks}, _terminals{terminals}, _degree{degree} {
}

void Net::set_length(size_t length) {
  _length = length;
}

} // end of namespace ===================================================
