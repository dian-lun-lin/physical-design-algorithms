#pragma once
#include <declarations.h>
#include <src/utility.hpp>
#include <src/block.hpp>
#include <src/terminal.hpp>
#include <src/net.hpp>

namespace fp { // begin of namespace ====================================

class SP {

  friend class ParallelSP;

  public:

    SP(
      const float alpha,
      const std::filesystem::path& blockf, 
      const std::filesystem::path& netf,
      std::mt19937& eng
    );

    SP(const SP& sp) = default;
    SP(SP&& sp) = default;

    SP& operator= (const SP& sp) = default;
    SP& operator= (SP&& sp) = default;
  
    ~SP() = default;

    void apply();
    void dump(std::ostream& os);

  private:

    void _parse_block(
      const std::filesystem::path& blockf
    );

    void _parse_net(
      const std::filesystem::path& netf
    );

    void _initialize();
    void _build_connections();
    void _spfa_h();
    void _spfa_v();
    void _move();
    void _move1(size_t idx1);
    void _move2(size_t idx1);
    void _move3(size_t idx);
    void _move(const std::vector<Block*>& out_bounds);
    void _get_results();
    void _get_final_results();
    void _get_average();
    void _set_coordinate();
    void _set_wirelength();
    void _reject();
    void _accept();
    void _update_best();
    void _update_all_to_best();
    void _reverse();
    void _compress(int step_size);
    void _detail_compress();
    bool _is_overlapped(Block* a, Block* b);
    double _get_penalty();

    std::unordered_map<std::string, Block> _blocks_map;
    std::vector<Block*> _blocks;

    std::unordered_map<std::string, Terminal> _terminals_map;
    std::vector<Net> _nets;

    std::pair<size_t, size_t> _outline;
    size_t _num_blocks;
    size_t _num_nets;
    size_t _num_terminals;

    std::vector<size_t> _first_seq;
    std::vector<size_t> _second_seq;
    std::vector<size_t> _first_seq_id_loc_map;
    std::vector<size_t> _second_seq_id_loc_map;
    double _cost;

    std::vector<size_t> _prev_first_seq;
    std::vector<size_t> _prev_second_seq;
    std::vector<size_t> _prev_first_seq_id_loc_map;
    std::vector<size_t> _prev_second_seq_id_loc_map;
    double _prev_cost;

    std::vector<size_t> _best_first_seq;
    std::vector<size_t> _best_second_seq;
    std::vector<size_t> _best_first_seq_id_loc_map;
    std::vector<size_t> _best_second_seq_id_loc_map;
    double _best_cost;

    std::vector<std::pair<size_t, size_t>> _best_block_wh;
    std::vector<std::pair<size_t, size_t>> _prev_block_wh;

    std::vector<size_t> _length_h;
    std::vector<size_t> _length_v;

    std::vector<size_t> _critical_path_h;
    std::vector<size_t> _critical_path_v;

    Block* _s;
    Block* _t;

    float _wire_length;
    float _chip_area;
    float _chip_height;
    float _chip_width;
    float _alpha;

    double _area_average{0};
    double _wire_length_average{0};
    double _penalty_average{0};

    std::mt19937& _eng;
    std::chrono::time_point<std::chrono::steady_clock> _tic;
    std::chrono::time_point<std::chrono::steady_clock> _toc;
    float _runtime;

};

SP::SP(
  const float alpha,
  const std::filesystem::path& blockf, 
  const std::filesystem::path& netf,
  std::mt19937& eng
):_alpha{alpha}, _eng{eng} {
  _tic = std::chrono::steady_clock::now();
  // source
  auto src = _blocks_map.emplace(
    std::piecewise_construct, 
    std::forward_as_tuple("src"), 
    std::forward_as_tuple("src", 0, 0, 0)
  );
  auto& src_ = (*(src.first)).second;
  _blocks.push_back(&src_);
  _s = _blocks[0];

  // target
  auto tgt = _blocks_map.emplace(
    std::piecewise_construct, 
    std::forward_as_tuple("tgt"),
    std::forward_as_tuple("tgt", 1, 0, 0)
  );
  auto& tgt_ = (*(tgt.first)).second;
  _blocks.push_back(&tgt_);
  _t = _blocks[1];

  _parse_block(blockf);
  _parse_net(netf);

  _best_block_wh.resize(_blocks.size());
  _prev_block_wh.resize(_blocks.size());



  // initialize
  _get_average();

  _initialize();
  _cost = INT_MAX;
  _update_best();
  _update_all_to_best();

}

void SP::_get_average() {
  _area_average = 0;
  _wire_length_average = 0;
  // It seems like running one time is enough
  for(size_t i = 0; i < 1; ++i) {
    _initialize();
    _area_average += _chip_area;
    _wire_length_average += _wire_length;
    _penalty_average += _get_penalty();
  }

  //_area_average /= 100;
  //_wire_length_average /= 100;
  //_penalty_average /= 100;
}

double SP::_get_penalty() {
  double penalty{0};
  for(size_t i = 2; i < _num_blocks + 2; ++i) {

    if(_outline.first < _blocks[i]->_x2) {
      penalty += std::pow(_blocks[i]->_x2 - _outline.first, 2);
  
    } 

    if(_outline.second < _blocks[i]->_y2) {
      penalty += std::pow(_blocks[i]->_y2 - _outline.second, 2);
    }
  }

  if(_chip_width > _outline.first && _chip_height < _outline.second ) {
    penalty += (_chip_width - _outline.first) * _outline.second;
   }
  else if(_chip_width < _outline.first && _chip_height > _outline.second) {
    penalty += (_chip_height - _outline.second) * _outline.first;
  }
  else if (_chip_width > _outline.first && _chip_height > _outline.second) {
    penalty += _chip_height * _chip_width - _outline.second * _outline.first;
  }
  return penalty;
}


void SP::apply() {

  std::uniform_real_distribution dist(0.f, 1.f);
  std::uniform_int_distribution<size_t> dist2(1, 10);
  std::uniform_int_distribution<size_t> dist3(2, _num_blocks - 1);

  size_t count{0};
  std::vector<Block*> out_bounds;
  std::vector<Block*> in_bounds;
  double total_temp{5000};
  bool is_legal{false}; 
  size_t nobest_count{0};

  size_t out_width{0};
  size_t out_height{0};
  double temp{total_temp};
  double beta = 300;
  _build_connections();
  _get_results();
  std::cerr << "thread\n";
  while(temp > 5) {

    for(size_t i = 0; i < 3000; ++i) {
      out_bounds.clear();
      in_bounds.clear();
      for(size_t k = 2; k < _num_blocks + 2; ++k) {
        bool is_legal = (_outline.first >= _chip_width) && (_outline.second >= _chip_height);
        if(!is_legal) {
          out_bounds.push_back(_blocks[k]);
        }
        else {
          in_bounds.push_back(_blocks[k]);
        }
      }

      //std::shuffle(in_bounds.begin(), in_bounds.end(), _eng);
      //std::shuffle(out_bounds.begin(), out_bounds.end(), _eng);

      //for(size_t j = 0; j < in_bounds.size() / 2; ++j) {
        //_move3(in_bounds[j]->_id);
      //}

      //size_t num_moves = dist2(_eng);
      //for(size_t j = 0; j < num_moves; ++j) {
        //_move(out_bounds);
      //}
      
      //for(size_t j = 0; j < num_moves; ++j) {
      _move();
      //}

      _build_connections();
      _get_results();

      double penalty = _get_penalty() / _penalty_average;
      //double penalty = _get_penalty();
      //if(_outline.first < _chip_width) {
        ////penalty += (_chip_width - _outline.first) + 0.01 * out_width;
        //penalty += (_chip_width - _outline.first);
        //out_width++;
      //}
      //if(_outline.second < _chip_height) {
        ////penalty += (_chip_height - _outline.second) + 0.01 * out_height;
        //penalty += (_chip_height - _outline.second);
        //out_height++;
      //}

      
      _cost += penalty;
      //std::cerr << "penalty: " << penalty << "\n";
      //std::cerr << "number out width: " << out_width << "\n";
      //std::cerr << "number out height: " << out_height << "\n";

      if(_cost < _prev_cost) {
        _accept();
        if(_cost < _best_cost) {
          _update_best();
        }
        //std::cerr << "accept\n";
      }
      else {
        ++nobest_count;
        auto random = dist(_eng);
        double accr = std::exp((_cost - _prev_cost) * beta * -1 / temp);
        //std::cerr << "delta cost: " << _cost - _prev_cost << "\n";
        //std::cerr << "accr rate: " << accr << "\n";

        if(accr > random) {
          _accept();
          //std::cerr << "accept\n";
        }
        else {
          _reject();
          //std::cerr << "reject\n";
        }
      }
    }
    temp *= 0.85;
  }

  _update_all_to_best();
  _build_connections();
  _get_results();

}

//void SP::_reverse() {
  //std::vector<size_t> reverse_first_seq(_first_seq.size());
  //std::vector<size_t> reverse_second_seq(_second_seq.size());
  //std::vector<size_t> reverse_first_id_loc_map(_first_seq_id_loc_map.size());
  //std::vector<size_t> reverse_second_id_loc_map(_second_seq_id_loc_map.size());

  //for(size_t i = 0; i < _first_seq.size(); ++i) {
    //reverse_first_seq[i] = _first_seq[_first_seq.size() - i - 1];
    //reverse_first_id_loc_map[reverse_first_seq[i]] = i;
  //}

  //for(size_t i = 0; i < _second_seq.size(); ++i) {
    //reverse_second_seq[i] = _second_seq[_second_seq.size() - i - 1];
    //reverse_second_id_loc_map[reverse_second_seq[i]] = i;
  //}
  //_first_seq = reverse_first_seq;
  //_second_seq = reverse_second_seq;
  //_first_seq_id_loc_map = reverse_first_id_loc_map;
  //_second_seq_id_loc_map = reverse_second_id_loc_map;

//}

void SP::_update_all_to_best() {
  _first_seq = _best_first_seq;
  _second_seq = _best_second_seq;
  _first_seq_id_loc_map = _best_first_seq_id_loc_map;
  _second_seq_id_loc_map = _best_second_seq_id_loc_map;
  _cost = _best_cost;

  _prev_first_seq = _best_first_seq;
  _prev_second_seq = _best_second_seq;
  _prev_first_seq_id_loc_map = _best_first_seq_id_loc_map;
  _prev_second_seq_id_loc_map = _best_second_seq_id_loc_map;
  _prev_cost = _best_cost;

  for(size_t i = 2; i < _num_blocks + 2; ++i) {
    _blocks[i]->_width       = _best_block_wh[i].first;
    _blocks[i]->_height      = _best_block_wh[i].second;
  }
  _prev_block_wh  = _best_block_wh;
}

void SP::_update_best() {
  _best_first_seq = _first_seq;
  _best_second_seq = _second_seq;
  _best_first_seq_id_loc_map = _first_seq_id_loc_map;
  _best_second_seq_id_loc_map = _second_seq_id_loc_map;
  _best_cost = _cost;

  for(size_t i = 2; i < _num_blocks + 2; ++i) {
    _best_block_wh[i].first  = _blocks[i]->_width;
    _best_block_wh[i].second = _blocks[i]->_height;
  }
}

void SP::_accept() {
  _prev_first_seq = _first_seq;
  _prev_second_seq = _second_seq;
  _prev_first_seq_id_loc_map = _first_seq_id_loc_map;
  _prev_second_seq_id_loc_map = _second_seq_id_loc_map;
  _prev_cost = _cost;

  for(size_t i = 2; i < _num_blocks + 2; ++i) {
    _prev_block_wh[i].first  = _blocks[i]->_width;
    _prev_block_wh[i].second = _blocks[i]->_height;
  }
}

void SP::_reject() {
  _first_seq = _prev_first_seq;
  _second_seq = _prev_second_seq;
  _first_seq_id_loc_map = _prev_first_seq_id_loc_map;
  _second_seq_id_loc_map = _prev_second_seq_id_loc_map;
  _cost = _prev_cost;

  for(size_t i = 2; i < _num_blocks + 2; ++i) {
    _blocks[i]->_width  = _prev_block_wh[i].first;
    _blocks[i]->_height = _prev_block_wh[i].second;
  }
}


void SP::dump(std::ostream& os) {
  std::cerr << "dumping...\n";

  _toc = std::chrono::steady_clock::now();
  _runtime = std::chrono::duration_cast<std::chrono::seconds>(_toc - _tic).count();

  os << _cost        << "\n";
  os << _wire_length << "\n";
  os << _chip_area   << "\n";
  os << _chip_width  << " "   << _chip_height << "\n";
  os << _runtime     << "\n";

  for(size_t i = 2; i < _num_blocks + 2; ++i) {
    os << _blocks[i]->_name << " " << _blocks[i]->_x1 << " " << _blocks[i]->_y1 << " " << _blocks[i]->_x2 << " " << _blocks[i]->_y2 << "\n";
  }
}

void SP::_initialize() {
  _first_seq.clear();
  _second_seq.clear();
  _second_seq_id_loc_map.clear();
  _first_seq_id_loc_map.clear();

  _first_seq.resize(_num_blocks);
  _second_seq.resize(_num_blocks);
  _second_seq_id_loc_map.resize(_num_blocks + 2);
  _first_seq_id_loc_map.resize(_num_blocks + 2);

  std::vector<size_t> r1(_num_blocks);
  std::vector<size_t> r2(_num_blocks);
  std::iota(r1.begin(), r1.end(), 2);
  std::iota(r2.begin(), r2.end(), 2);
  std::shuffle(r1.begin(), r1.end(), _eng);
  std::shuffle(r2.begin(), r2.end(), _eng);

  for(size_t i = 0; i < _num_blocks; ++i) {
    auto f = r1[i];
    auto s = r2[i];
    _first_seq[i] = f;
    _second_seq[i] = s;
    _first_seq_id_loc_map[f] = i;
    _second_seq_id_loc_map[s] = i;
  }

  _build_connections();
  _get_results();
}

void SP::_get_results() {
  _spfa_h();
  _spfa_v();

  _set_coordinate();
  _chip_width = _length_h[1];
  _chip_height = _length_v[1];
  _chip_area = _chip_height * _chip_width;

  _set_wirelength();
  //_chip_width = (*std::max_element(_blocks.begin() + 2, _blocks.end(), [](const Block* a, const Block* b){ return a->_x2 < b->_x2; }))->_x2;
  //_chip_height = (*std::max_element(_blocks.begin() + 2, _blocks.end(), [](const Block* a, const Block* b){ return a->_y2 < b->_y2; }))->_y2;
  //_chip_area = _chip_height * _chip_width;

  // cost
  _cost = _alpha * _chip_area / _area_average + (1-_alpha) * _wire_length / _wire_length_average;

  //std::cerr << "restriced outline: " << "width: " << _outline.first << " height: " << _outline.second << "\n";
  //std::cerr << "current outline: " << "width: " << _chip_width << " height: " << _chip_height << "\n"; 
}

//void SP::_get_results() {
  //_spfa_h();
  //_spfa_v();

  //_set_coordinate();
  //auto tmp = std::min(_outline.first, _outline.second);
  //_compress(tmp/50);
  //_set_wirelength();
  //_chip_width = (*std::max_element(_blocks.begin() + 2, _blocks.end(), [](const Block* a, const Block* b){ return a->_x2 < b->_x2; }))->_x2;
  //_chip_height = (*std::max_element(_blocks.begin() + 2, _blocks.end(), [](const Block* a, const Block* b){ return a->_y2 < b->_y2; }))->_y2;
  //_chip_area = _chip_height * _chip_width;
  //_chip_width = (*std::max_element(_blocks.begin() + 2, _blocks.end(), [](const Block* a, const Block* b){ return a->_x2 < b->_x2; }))->_x2;
  //_chip_height = (*std::max_element(_blocks.begin() + 2, _blocks.end(), [](const Block* a, const Block* b){ return a->_y2 < b->_y2; }))->_y2;
  //_chip_area = _chip_height * _chip_width;
//}

void SP::_get_final_results() {
  _spfa_h();
  _spfa_v();

  _set_coordinate();
  _compress(1);
  _set_wirelength();
  _chip_width = (*std::max_element(_blocks.begin() + 2, _blocks.end(), [](const Block* a, const Block* b){ return a->_x2 < b->_x2; }))->_x2;
  _chip_height = (*std::max_element(_blocks.begin() + 2, _blocks.end(), [](const Block* a, const Block* b){ return a->_y2 < b->_y2; }))->_y2;
  _chip_area = _chip_height * _chip_width;
  _chip_width = (*std::max_element(_blocks.begin() + 2, _blocks.end(), [](const Block* a, const Block* b){ return a->_x2 < b->_x2; }))->_x2;
  _chip_height = (*std::max_element(_blocks.begin() + 2, _blocks.end(), [](const Block* a, const Block* b){ return a->_y2 < b->_y2; }))->_y2;
  _chip_area = _chip_height * _chip_width;
}

//void SP::_relax() {
  //if(_chip_width > )

//}

void SP::_compress(int step_size) {

  //std::cerr << "id: " << sorted_idx_x[0] << " " <<  _blocks[sorted_idx_x[0]]->_x1 << ", " 
            //<< "id: " << sorted_idx_x[10] << " " << _blocks[sorted_idx_x[10]]->_x1 << "\n";

  //// horizonal
  size_t count = 5;
  while(count-- > 0) {
    std::vector<size_t> sorted_idx_x(_num_blocks);
    std::iota(sorted_idx_x.begin(), sorted_idx_x.end(), 2);
    std::stable_sort(sorted_idx_x.begin(), sorted_idx_x.end(), [this](const size_t a, const size_t b) {
      return _blocks[a]->_x1 < _blocks[b]->_x1;
    });

    std::vector<size_t> sorted_idx_y(_num_blocks);
    std::iota(sorted_idx_y.begin(), sorted_idx_y.end(), 2);
    std::stable_sort(sorted_idx_y.begin(), sorted_idx_y.end(), [this](const size_t a, const size_t b) {
      return _blocks[a]->_y1 < _blocks[b]->_y1;
    });

    for(int i = 0; i < sorted_idx_x.size(); ++i) {
      auto idx = sorted_idx_x[i];
      int from = _blocks[idx]->_x1;
      for(int x = from - step_size; x > 0; x-=step_size) {
        size_t prev_x1 = _blocks[idx]->_x1;
        size_t prev_y1 = _blocks[idx]->_y1;
        size_t prev_x2 = _blocks[idx]->_x2;
        size_t prev_y2 = _blocks[idx]->_y2;
        bool is_legal = true;
        _blocks[idx]->_x1 = x;
        _blocks[idx]->_x2 = _blocks[idx]->_x1 + _blocks[idx]->_width;
        
        for(size_t k = 2; k < _num_blocks + 2; ++k) {
          if(k != idx && _is_overlapped(_blocks[idx], _blocks[k])) {
            is_legal = false;
            break;
          }
        }
        //for(int k = i-1; k > 0; --k) {
          //size_t idx2 = sorted_idx_x[k];
          //if(_is_overlapped(_blocks[idx], _blocks[idx2])) {
            //is_legal = false;
            //break;
          //}
        //}
        if(!is_legal) {
          _blocks[idx]->set_coordinate(prev_x1, prev_y1, prev_x2, prev_y2);
          break;
        }
      }
    }

  // vertical
    for(int i = 0; i < sorted_idx_y.size(); ++i) {
      auto idx = sorted_idx_y[i];
      int from = _blocks[idx]->_y1;
      for(int y = from - step_size; y > 0; y-=step_size) {
        size_t prev_x1 = _blocks[idx]->_x1;
        size_t prev_y1 = _blocks[idx]->_y1;
        size_t prev_x2 = _blocks[idx]->_x2;
        size_t prev_y2 = _blocks[idx]->_y2;
        bool is_legal = true;
        _blocks[idx]->_y1 = y;
        _blocks[idx]->_y2 = _blocks[idx]->_y1 + _blocks[idx]->_height;
        
        //for(size_t k = 2; k < _num_blocks + 2; ++k) {
        //for(int k = i-1; k > 0; --k) {
          //size_t idx2 = sorted_idx_y[k];
          //if(_is_overlapped(_blocks[idx], _blocks[idx2])) {
            //is_legal = false;
            //break;
          //}
        //}
        for(size_t k = 2; k < _num_blocks + 2; ++k) {
          if(k != idx && _is_overlapped(_blocks[idx], _blocks[k])) {
            is_legal = false;
            break;
          }
        }
        if(!is_legal) {
          _blocks[idx]->set_coordinate(prev_x1, prev_y1, prev_x2, prev_y2);
          break;
        }
      }
    }
  }
  //for(size_t i = 2; i < _num_blocks + 2; ++i) {
    //for(size_t j = i + 1; j < _num_blocks + 2; ++j) {
      //std::cerr << "idx: " << i << ", (x1, y1, x2, y2): " 
                //<< "("<< _blocks[i]->_x1 << ", " << _blocks[i]->_y1 << ", " <<  _blocks[i]->_x2 << ", " << _blocks[i]->_y2 << ")\n";
      //std::cerr << "idx: " << j << ", (x1, y1, x2, y2): "
                //<< "("<< _blocks[j]->_x1 << ", " << _blocks[j]->_y1 << ", " <<  _blocks[j]->_x2 << ", " << _blocks[j]->_y2 << ")\n";
      //assert(!_is_overlapped(_blocks[i], _blocks[j]));
    //}
  //}

}

bool SP::_is_overlapped(Block* a, Block* b) {
  return 
    !(a->_x2 <= b->_x1 || a->_x1 >= b->_x2 || a->_y2 <= b->_y1 || a->_y1 >= b->_y2);
}

void SP::_set_coordinate() {

  for(size_t i = 0; i < _num_blocks + 2; ++i) {
    _blocks[i]->set_coordinate(_length_h[i] - _blocks[i]->_width, _length_v[i] - _blocks[i]->_height, _length_h[i], _length_v[i]);
  }
}

void SP::_set_wirelength() {
  
  _wire_length = 0.;
  for(auto& n: _nets) {
    float lg;
    size_t bx_min{INT_MAX};
    size_t bx_max{0};
    size_t by_min{INT_MAX};
    size_t by_max{0};
    for(auto b: n._blocks) {
      auto tmpx = 2 * b->_x1 + b->_width;
      auto tmpy = 2 * b->_y1 + b->_height;
      bx_min = std::min(tmpx, bx_min);
      bx_max = std::max(tmpx, bx_max);

      by_min = std::min(tmpy, by_min);
      by_max = std::max(tmpy, by_max);
    }

    if(!n._terminals.empty()) {
      size_t tx_min{INT_MAX};
      size_t tx_max{0};
      size_t ty_min{INT_MAX};
      size_t ty_max{0};
      for(auto t: n._terminals) {
        auto tmpx = 2 * t->_x;
        auto tmpy = 2 * t->_y;
        tx_min = std::min(tmpx, tx_min);
        tx_max = std::max(tmpx, tx_max);

        ty_min = std::min(tmpy, ty_min);
        ty_max = std::max(tmpy, ty_max);
      }

      size_t x_min = std::min(bx_min, tx_min);
      size_t x_max = std::max(bx_max, tx_max);
      size_t y_min = std::min(by_min, ty_min);
      size_t y_max = std::max(by_max, ty_max);
      lg = (x_max - x_min + y_max - y_min) / 2.0f;
    }
    //else {
      size_t x_min = bx_min;
      size_t x_max = bx_max;
      size_t y_min = by_min;
      size_t y_max = by_max;
      lg = (x_max - x_min + y_max - y_min) / 2.0f;
    //}

    n.set_length(lg);
    _wire_length += lg;
  }
  _wire_length;
}

void SP::_build_connections() {

  //std::cerr << "first seq: ";
  //for(size_t i = 0; i < _num_blocks; ++i) {
    //std::cerr << _first_seq[i] << " ";
  //}
  //std::cerr << "\n";
  //std::cerr << "second seq: ";
  //for(size_t i = 0; i < _num_blocks; ++i) {
    //std::cerr << _second_seq[i] << " ";
  //}
  //std::cerr << "\n";

  for(auto& b: _blocks) {
    b->_hconnects.clear();
    b->_vconnects.clear();
  }

  for(size_t i = 2; i < _num_blocks + 2; ++i) {
    auto f_loc1 = _first_seq_id_loc_map[i];
    auto f_loc2 = _second_seq_id_loc_map[i];

    for(size_t k = 2; k < _num_blocks + 2; ++k) {
      if(k != i) {
        auto sec_loc1 = _first_seq_id_loc_map[k];
        auto sec_loc2 = _second_seq_id_loc_map[k];

        if(f_loc1 < sec_loc1 && f_loc2 < sec_loc2) {
          // horizontal
          _blocks[i]->_hconnects.push_back(_blocks[k]);
        }
        else if(f_loc1 > sec_loc1 && f_loc2 < sec_loc2) {
          // vertical
          _blocks[i]->_vconnects.push_back(_blocks[k]);
        }
      }
    }

  }

  // s -> t
  _s->_hconnects.push_back(_t);
  _s->_vconnects.push_back(_t);
  for(size_t i = 2; i < _num_blocks + 2; ++i) {
    // s -> each block
    _s->_hconnects.push_back(_blocks[i]);
    _s->_vconnects.push_back(_blocks[i]);

    // each block -> t
    _blocks[i]->_hconnects.push_back(_t);
    _blocks[i]->_vconnects.push_back(_t);
  }

  //for(size_t i = 2; i < _num_blocks + 2; ++i) {
    //std::cerr << "check hconnect for " << i << " block: ";
    //for(auto& b: _blocks[i]->_hconnects) {
      //std::cerr << b->_id << " ";
    //}
    //std::cerr << "\n";

    //std::cerr << "check vconnect for " << i << " firt block: ";
    //for(auto& b: _blocks[i]->_vconnects) {
      //std::cerr << b->_id << " ";
    //}
    //std::cerr << "\n";
  //}
  
}

void SP::_spfa_h() {
  std::queue<Block*> spfa;
  spfa.push(_s);

  _length_h.clear();
  _length_h.resize(_num_blocks + 2, 0);
  std::vector<bool> in_queue(_num_blocks + 2, false);
  in_queue[0] = true;

  while(!spfa.empty()) {
    auto b = spfa.front();
    spfa.pop();
    in_queue[b->_id] = false;

    for(auto nb: b->_hconnects) {
      if(_length_h[b->_id] + nb->_width > _length_h[nb->_id]) {
        _length_h[nb->_id] = _length_h[b->_id] + nb->_width;
        if(!in_queue[nb->_id]) {
          in_queue[nb->_id] = true;
          spfa.push(nb);
        }
      }
    }
  }

}

void SP::_spfa_v() {
  std::queue<Block*> spfa;
  spfa.push(_s);

  _length_v.clear();
  _length_v.resize(_num_blocks + 2, 0);
  std::vector<bool> in_queue(_num_blocks + 2, false);
  in_queue[0] = true;

  while(!spfa.empty()) {
    auto b = spfa.front();
    spfa.pop();
    in_queue[b->_id] = false;

    for(auto nb: b->_vconnects) {
      if(_length_v[b->_id] + nb->_height > _length_v[nb->_id]) {
        _length_v[nb->_id] = _length_v[b->_id] + nb->_height;
        if(!in_queue[nb->_id]) {
          in_queue[nb->_id] = true;
          spfa.push(nb);
        }
      }
    }
  }

}

void SP::_move1(size_t idx1) {
  // move 1
  std::uniform_int_distribution<> random_idx(2, _num_blocks + 1);
  auto idx2 = random_idx(_eng);
  auto loc1 = _first_seq_id_loc_map[idx1];
  auto loc2 = _first_seq_id_loc_map[idx2];

  std::swap(_first_seq[loc1], _first_seq[loc2]);
}

void SP::_move2(size_t idx1) {
  // move 2
  std::uniform_int_distribution<> random_idx(2, _num_blocks + 1);
  auto idx2 = random_idx(_eng);

  auto first_loc1 =  _first_seq_id_loc_map[idx1];
  auto first_loc2 =  _first_seq_id_loc_map[idx2];

  auto second_loc1 =  _second_seq_id_loc_map[idx1];
  auto second_loc2 =  _second_seq_id_loc_map[idx2];

  std::swap(_first_seq[first_loc1], _first_seq[first_loc2]);
  std::swap(_second_seq[second_loc1], _second_seq[second_loc2]);
  std::swap(_first_seq_id_loc_map[idx1], _first_seq_id_loc_map[idx2]);
  std::swap(_second_seq_id_loc_map[idx1], _second_seq_id_loc_map[idx2]);
}

void SP::_move3(size_t idx) {
  // move 3
  //std::uniform_int_distribution<> random_idx(2, _num_blocks + 1);
  //auto idx = random_idx(_eng);
  auto* b = _blocks[idx];
  if(_outline.first < _chip_width && _outline.second < _chip_height) {
    auto delta_w = _chip_width - _outline.first;
    auto delta_h = _chip_height - _outline.second;
    if(
      (delta_w > delta_h && b->_height > b->_width) ||
      (delta_w < delta_h && b->_height < b->_width)
    ) {
      std::swap(b->_height, b->_width);
    }
  }
  else if(_outline.first < _chip_width) {
    if(b->_height < b->_width) {
      std::swap(b->_height, b->_width);
    }
  } 
  else if(_outline.second < _chip_height) {
    if(b->_height > b->_width) {
      std::swap(b->_height, b->_width);
    }
  }
  else {
    std::swap(b->_height, b->_width);
  }
}

void SP::_move() {

  std::uniform_int_distribution<> random_move(1, 3);
  std::uniform_int_distribution<> random_idx(2, _num_blocks + 1);
  auto random = random_move(_eng);
  
  if(random == 1) {
    // move 1
    auto idx1 = random_idx(_eng);
    auto idx2 = random_idx(_eng);
    auto loc1 = _first_seq_id_loc_map[idx1];
    auto loc2 = _first_seq_id_loc_map[idx2];

    std::swap(_first_seq[loc1], _first_seq[loc2]);
    std::swap(_first_seq_id_loc_map[idx1], _first_seq_id_loc_map[idx2]);
  }
  else if(random == 2) {
    // move 2
    auto idx1 = random_idx(_eng);
    auto idx2 = random_idx(_eng);

    auto first_loc1 =  _first_seq_id_loc_map[idx1];
    auto first_loc2 =  _first_seq_id_loc_map[idx2];

    auto second_loc1 =  _second_seq_id_loc_map[idx1];
    auto second_loc2 =  _second_seq_id_loc_map[idx2];

    std::swap(_first_seq[first_loc1], _first_seq[first_loc2]);
    std::swap(_second_seq[second_loc1], _second_seq[second_loc2]);
    std::swap(_first_seq_id_loc_map[idx1], _first_seq_id_loc_map[idx2]);
    std::swap(_second_seq_id_loc_map[idx1], _second_seq_id_loc_map[idx2]);
  }
  else {
    // move 3
    auto idx = random_idx(_eng);
    auto* b = _blocks[idx];
    if(_outline.first < _chip_width && _outline.second < _chip_height) {
      std::swap(b->_height, b->_width);
    }
    else if(_outline.first < _chip_width) {
      if(b->_height < b->_width) {
        std::swap(b->_height, b->_width);
      }
    } 
    else if(_outline.second < _chip_height) {
      if(b->_height > b->_width) {
        std::swap(b->_height, b->_width);
      }
    }
    else {
      std::swap(b->_height, b->_width);
    }
  }
}
void SP::_move(const std::vector<Block*>& out_bounds) {

  std::uniform_int_distribution<> random_move(1, 4);
  std::uniform_int_distribution<> random_idx(2, _num_blocks + 1);
  std::uniform_int_distribution<> random_out_bounds(0, out_bounds.size() - 1);
  auto random = random_move(_eng);
  
  switch(random) {
    case 1: {
      // move 1
      auto idx1 = random_idx(_eng);
      auto ob_idx = random_out_bounds(_eng);
      auto idx2 = out_bounds[ob_idx]->_id;
      
      auto loc1 = _first_seq_id_loc_map[idx1];
      auto loc2 = _first_seq_id_loc_map[idx2];

      std::swap(_first_seq[loc1], _first_seq[loc2]);
      std::swap(_first_seq_id_loc_map[idx1], _first_seq_id_loc_map[idx2]);
      
      break;
    }
    case 2: {
      // move 2
      auto idx1 = random_idx(_eng);
      auto ob_idx = random_out_bounds(_eng);
      auto idx2 = out_bounds[ob_idx]->_id;


      auto first_loc1 =  _first_seq_id_loc_map[idx1];
      auto first_loc2 =  _first_seq_id_loc_map[idx2];
      auto second_loc1 =  _second_seq_id_loc_map[idx1];
      auto second_loc2 =  _second_seq_id_loc_map[idx2];

      std::swap(_first_seq[first_loc1], _first_seq[first_loc2]);
      std::swap(_second_seq[second_loc1], _second_seq[second_loc2]);
      std::swap(_first_seq_id_loc_map[idx1], _first_seq_id_loc_map[idx2]);
      std::swap(_second_seq_id_loc_map[idx1], _second_seq_id_loc_map[idx2]);

      break;
    }

    case 3: {
      // move 3
      auto ob_idx = random_out_bounds(_eng);
      auto idx = out_bounds[ob_idx]->_id;
      auto* b = _blocks[idx];
      //if(_outline.first < b->_x2 && b->_height < b->_width) {
        //std::swap(b->_height, b->_width);
      //}
      //else if(_outline.second < b->_y2 && b->_height > b->_width) {
        //std::swap(b->_height, b->_width);
      //}
      if(_outline.first < _chip_width && _outline.second < _chip_height) {
        auto delta_w = _chip_width - _outline.first;
        auto delta_h = _chip_height - _outline.second;
        if(
          (delta_w > delta_h && b->_height > b->_width) ||
          (delta_w < delta_h && b->_height < b->_width)
        ) {
          std::swap(b->_height, b->_width);
        }
      }
      else if(_outline.first < _chip_width) {
        if(b->_height < b->_width) {
          std::swap(b->_height, b->_width);
        }
      } 
      else if(_outline.second < _chip_height) {
        if(b->_height > b->_width) {
          std::swap(b->_height, b->_width);
        }
      }
      else {
        std::swap(b->_height, b->_width);
      }

      break;
    }
    case 4: {
      // new move
      auto idx1 = random_idx(_eng);
      auto ob_idx = random_out_bounds(_eng);
      auto idx2 = out_bounds[ob_idx]->_id;

      auto loc1 = _second_seq_id_loc_map[idx1];
      auto loc2 = _second_seq_id_loc_map[idx2];

      std::swap(_second_seq_id_loc_map[idx1], _second_seq_id_loc_map[idx2]);

      std::swap(_second_seq[loc1], _second_seq[loc2]);
    }
    //case 5: {
      //// new move
      //auto loc1 = _random_idx(_eng);
      //auto loc2 = _random_idx(_eng);

      //std::swap(_second_seq_id_loc_map[_second_seq[loc1]], _second_seq_id_loc_map[_second_seq[loc2]]);
      //std::swap(_first_seq[loc1], _first_seq[loc2]);
      //std::swap(_second_seq[loc1], _second_seq[loc2]);
    //}
  }
}

void SP::_parse_block(
  const std::filesystem::path& blockf
) {
  auto sstream = read_file_to_sstream(blockf);
  std::string line;
  std::string tmp;


  // outline
  std::getline(sstream, line);
  if(line.find("Outline") != std::string::npos) {
    line = line.substr(line.find(": ") + 1);
    std::stringstream line_stream(line);

    std::getline(line_stream, tmp, ' ');
    std::getline(line_stream, tmp, ' ');
    _outline.first = std::stoi(tmp);
    std::getline(line_stream, tmp, ' ');
    _outline.second = std::stoi(tmp);

  }
  else {
    assert(false);
  }

  // numblocks
  std::getline(sstream, line);
  if(line.find("NumBlocks") != std::string::npos) {
    _num_blocks = std::stoi(line.substr(line.find(": ") + 1));
  }
  else {
    assert(false);
  }

  // numterminals
  std::getline(sstream, line);
  if(line.find("NumTerminals") != std::string::npos) {
    _num_terminals = std::stoi(line.substr(line.find(": ") + 1));
  }
  else {
    assert(false);
  }

  std::cerr << "outline: "              << _outline.first << ", " << _outline.second << "\n"
            << "number of blocks: "     << _num_blocks << "\n"
            << "number of terminals: "  << _num_terminals << "\n";

  _blocks.reserve(_num_blocks + 2);
  _blocks_map.reserve(_num_blocks + 2);
  _terminals_map.reserve(_num_terminals);


  while(std::getline(sstream, line)) {

    if(!line.empty() && (line[line.length() - 1] == '\r' || line[line.length() - 1] == '\n')) {
      line.erase(line.length() - 1);
    }
    if(!line.empty() && (line[line.length() - 1] == '\r' || line[line.length() - 1] == '\n')) {
      line.erase(line.length() - 1);
    }

    if(line.size() > 1) {
      if(line.find_first_not_of(' ') == std::string::npos) { continue; }
      std::stringstream line_stream(line);
      std::string token;
      std::vector<std::string> tokens;


      if(line.find("terminal") != std::string::npos) {
        //std::cerr << "line: " << line << "\n";
        while(std::getline(line_stream, token, ' ')) {
          if(token != "" && token != " " && token != "\r") {
            tokens.push_back(token);
          }
        }

        std::string tmp1, tmp2, tmp3;
        std::stringstream term_stream(tokens[2]);
        tmp1 = tokens[0];
        std::getline(term_stream, tmp2, '\t');
        std::getline(term_stream, tmp3, '\n');
        _terminals_map.emplace(
          std::piecewise_construct, 
          std::forward_as_tuple(tmp1), 
          std::forward_as_tuple(tmp1, _terminals_map.size(), std::stoi(tmp2), std::stoi(tmp3))
        );
      }
      else {
        std::cerr << "line: " << line << "\n";
        while(std::getline(line_stream, token, ' ')) {
          if(token != "" && token != " " && token != "\r") {
            tokens.push_back(token);
          }
        }
        if(tokens.size() < 3) {
          // this is to handle 3.block...
          tokens.clear();
          std::stringstream line_stream(line);
          while(std::getline(line_stream, token, '\t')) {
            if(token != "" && token != " " && token != "\r") {
              if(token[token.length() - 1] == '\r' || token[token.length() - 1] == '\n' || token[token.length() - 1] == ' ') {
                token.erase(token.length() - 1);
              }
              tokens.push_back(token);
            }
          }
          std::cerr << tokens[0] << ", " << tokens[1] << ", " << tokens[2] << "\n";
        }

        auto tmp = _blocks_map.emplace(
          std::piecewise_construct, 
          std::forward_as_tuple(tokens[0]), 
          std::forward_as_tuple(tokens[0], _blocks_map.size(), std::stoi(tokens[1]), std::stoi(tokens[2]))
        );
        auto& b = (*(tmp.first)).second;
        _blocks.push_back(&b);
      }
    }
  }

  assert(_blocks.size() == _num_blocks + 2);
  assert(_blocks_map.size() == _num_blocks + 2);
  assert(_terminals_map.size() == _num_terminals);

  //for(size_t i = 2; i < _num_blocks + 2; ++i) {
    //std::cerr << _blocks[i]->_name << ": (width, height) -> (" << _blocks[i]->_width << ", " << _blocks[i]->_height << ")\n";
  //}
}

void SP::_parse_net(
  const std::filesystem::path& netf
) {
  auto sstream = read_file_to_sstream(netf);
  std::string line;


  // numnets
  std::getline(sstream, line);
  if(line.find("NumNets") != std::string::npos) {
    _num_nets = std::stoi(line.substr(line.find(": ") + 1));
  }
  else {
    assert(false);
  }

  size_t degree{0};
  std::vector<Block*> blocks;
  std::vector<Terminal*> terminals;
  while(std::getline(sstream, line)) {
    if(!line.empty() && (line[line.length() - 1] == '\r' || line[line.length() - 1] == '\n')) {
      line.erase(line.length() - 1);
    }

    if(line.find("NetDegree") != std::string::npos) {
      if(degree != 0) {
        _nets.emplace_back(blocks, terminals, degree);
      }

      blocks.clear();
      terminals.clear();

      degree = std::stoi(line.substr(line.find(": ") + 1));
      blocks.reserve(degree);
      terminals.reserve(degree / 2);
    }
    else {
      auto btmp = _blocks_map.find(line);
      if(btmp != _blocks_map.end()) {
        blocks.push_back(&(btmp->second));
      }
      else { 
        auto ttmp = _terminals_map.find(line);
        if(ttmp != _terminals_map.end()) {
          terminals.push_back(&(ttmp->second));
          //std::cerr << "template: " << ttmp->second._name << "\n";
        }
        else {
          std::cerr << "error: " << line << "....\n";
          assert(false);
        }
      }
    }
  }

  if(degree != 0) {
    _nets.emplace_back(blocks, terminals, degree);
  }

  std::cerr << "num nets: " << _num_nets << "\n";
  //std::cerr << _nets.size();
  assert(_nets.size() == _num_nets);
}

} // end of namespace ===================================================
