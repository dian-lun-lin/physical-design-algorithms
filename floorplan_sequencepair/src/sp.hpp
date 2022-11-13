#pragma once
#include <declarations.h>
#include <src/utility.hpp>
#include <src/block.hpp>
#include <src/terminal.hpp>
#include <src/net.hpp>

namespace fp { // begin of namespace ====================================

class SP {

  public:

    SP(
      const float alpha,
      const std::filesystem::path& blockf, 
      const std::filesystem::path& netf
    );

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
    void _get_results();
    void _get_average();
    void _set_coordinate();
    void _set_wirelength();
    void _reject();
    void _accept();
    void _update_best();
    void _compress();
    bool _is_overlapped(Block* a, Block* b);

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
    std::vector<size_t> _second_seq_id_loc_map;
    float _cost;

    std::vector<size_t> _prev_first_seq;
    std::vector<size_t> _prev_second_seq;
    std::vector<size_t> _prev_second_seq_id_loc_map;
    float _prev_cost;

    std::vector<size_t> _best_first_seq;
    std::vector<size_t> _best_second_seq;
    std::vector<size_t> _best_second_seq_id_loc_map;
    float _best_cost;

    std::vector<size_t> _length_h;
    std::vector<size_t> _length_v;

    Block* _s;
    Block* _t;

    float _wire_length;
    float _chip_area;
    float _chip_height;
    float _chip_width;
    float _alpha;

    float _area_average;
    float _wire_length_average;

    std::random_device _rd{};
    std::mt19937 _eng{_rd()};
    std::chrono::time_point<std::chrono::steady_clock> _tic;
    std::chrono::time_point<std::chrono::steady_clock> _toc;
    float _runtime;

};

SP::SP(
  const float alpha,
  const std::filesystem::path& blockf, 
  const std::filesystem::path& netf
):_alpha{alpha} {
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
}

void SP::_get_average() {
  _area_average = 0;
  _wire_length_average = 0;
  for(size_t i = 0; i < 100; ++i) {
    _initialize();
    _build_connections();
    _get_results();
    _area_average += _chip_area;
    _wire_length_average += _wire_length;
  }

  _area_average /= 100;
  _wire_length_average /= 100;
}


void SP::apply() {

  _get_average();

  _initialize();
  _build_connections();
  _get_results();

  // accept inital solution
  _accept();
  _update_best();

  bool is_legal = (_outline.first >= _chip_width) && (_outline.second >= _chip_height);
  if(!is_legal) { 
    _cost = INT_MAX;
    _best_cost = INT_MAX; 
  };

  float temp{100000000};
  std::uniform_real_distribution dist(0.f, 1.f);
  std::uniform_int_distribution<size_t> dist2(1, _num_blocks);

  size_t worse_count{0};
  while(temp > 5 && worse_count < 1000) {
    auto move_times = dist2(_eng);
    for(size_t i = 0; i < move_times; ++i) {
      _move();
    }
    _build_connections();
    _get_results();

    float penalty_h = 0;
    float penalty_v = 0;
    penalty_h = _outline.first < _chip_width ? 10.f / temp : 0;
    penalty_v = _outline.second < _chip_height ? 10.f / temp : 0;
    bool is_legal = (_outline.first >= _chip_width) && (_outline.second >= _chip_height);
    _cost += penalty_h;
    _cost += penalty_v;



    if(_cost < _prev_cost) {
      _accept();
      if(_cost < _best_cost && is_legal) {
        _update_best();
      }
      std::cerr << "accept\n";
      worse_count = 0;
    }
    else {
      ++worse_count;
      auto random = dist(_eng);
      //float _beta = temp > 1000 ? 10.f : 50.f;
      std::cerr << "delta cost: " << _prev_cost - _cost << "\n";
      //float _beta = 10.f;
      //float accr = std::min(1.0f, std::exp(_beta * (_prev_cost - _cost)));
      float accr = std::exp((_cost - _prev_cost) * -1 / temp);
      std::cerr << "accr rate: " << accr << "\n";
      if(accr > random) {
        _accept();
        std::cerr << "accept\n";
      }
      else {
        _reject();
        std::cerr << "reject\n";
      }
    }

    temp *= 0.85;
  }
  std::cerr << "worse count: " << worse_count << "\n";

  //_move();
  _first_seq = _best_first_seq;
  _second_seq = _best_second_seq;
  _second_seq_id_loc_map = _best_second_seq_id_loc_map;
  _build_connections();
  _get_results();
}

void SP::_update_best() {
  _best_first_seq = _first_seq;
  _best_second_seq = _second_seq;
  _best_second_seq_id_loc_map = _second_seq_id_loc_map;
  _best_cost = _cost;
}

void SP::_accept() {
  _prev_first_seq = _first_seq;
  _prev_second_seq = _second_seq;
  _prev_second_seq_id_loc_map = _second_seq_id_loc_map;
  _prev_cost = _cost;
}

void SP::_reject() {
  _first_seq = _prev_first_seq;
  _second_seq = _prev_second_seq;
  _second_seq_id_loc_map = _prev_second_seq_id_loc_map;
  _cost = _prev_cost;
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

  _first_seq.resize(_num_blocks);
  _second_seq.resize(_num_blocks);
  _second_seq_id_loc_map.resize(_num_blocks + 2);

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
    _second_seq_id_loc_map[s] = i;
  }

  //std::iota(_first_seq.begin(), _first_seq.end(), 0);
  //std::iota(_second_seq.begin(), _second_seq.end(), 0);
  //std::shuffle(_first_seq.begin(), _first_seq.end(), _eng);
  //std::shuffle(_second_seq.begin(), _second_seq.end(), _eng);

}

void SP::_get_results() {
  _spfa_h();
  _spfa_v();

  _set_coordinate();
  _compress();
  //_chip_width = _length_h[1];
  //_chip_height = _length_v[1];
  //_chip_area = _chip_height * _chip_width;

  _set_wirelength();
  _chip_width = (*std::max_element(_blocks.begin() + 2, _blocks.end(), [](const Block* a, const Block* b){ return a->_x2 < b->_x2; }))->_x2;
  _chip_height = (*std::max_element(_blocks.begin() + 2, _blocks.end(), [](const Block* a, const Block* b){ return a->_y2 < b->_y2; }))->_y2;
  _chip_area = _chip_height * _chip_width;

  // cost
  _cost = _alpha * _chip_area / _area_average + (1-_alpha) * _wire_length / _wire_length_average;

  std::cerr << "restriced outline: " << "width: " << _outline.first << " height: " << _outline.second << "\n";
  std::cerr << "current outline: " << "width: " << _chip_width << " height: " << _chip_height << "\n"; 
}

void SP::_compress() {

  int count = 10;
  while(count-- > 0) {
    std::vector<size_t> sorted_idx_x(_num_blocks);
    std::iota(sorted_idx_x.begin(), sorted_idx_x.end(), 2);
    std::sort(sorted_idx_x.begin(), sorted_idx_x.end(), [&](const size_t a, const size_t b) {
      return _blocks[a]->_x1 < _blocks[b]->_x1;
    });
    std::vector<size_t> sorted_idx_y(_num_blocks);
    std::iota(sorted_idx_y.begin(), sorted_idx_y.end(), 2);
    std::sort(sorted_idx_y.begin(), sorted_idx_y.end(), [&](const size_t a, const size_t b) {
      return _blocks[a]->_y1 < _blocks[b]->_y1;
    });

    // horizonal
    for(auto i: sorted_idx_x) {
      assert(i != 0 && i != 1);
      size_t from = _blocks[i]->_x1;
      for(int x = from - 10; x > 0; x-=10) {
        size_t prev_x1 = _blocks[i]->_x1;
        size_t prev_y1 = _blocks[i]->_y1;
        size_t prev_x2 = _blocks[i]->_x2;
        size_t prev_y2 = _blocks[i]->_y2;
        bool is_legal = true;
        _blocks[i]->_x1 = x;
        _blocks[i]->_x2 = _blocks[i]->_x1 + _blocks[i]->_width;
        
        for(size_t k = 2; k < _num_blocks + 2; ++k) {
          //for(size_t k = 0; k < i; ++k) {
          //size_t idx = sorted_idx_x[k];
          if(k != i) {
            if(_is_overlapped(_blocks[i], _blocks[k])) {
              is_legal = false;
              break;
            }
          }
        }
        if(!is_legal) {
          _blocks[i]->set_coordinate(prev_x1, prev_y1, prev_x2, prev_y2);
        }
        
      }
    }

    // vertical
    for(auto i: sorted_idx_y) {
      assert(i != 0 && i != 1);
      size_t from = _blocks[i]->_y1;
      for(int y = from - 10; y > 0; y-=10) {
        size_t prev_x1 = _blocks[i]->_x1;
        size_t prev_y1 = _blocks[i]->_y1;
        size_t prev_x2 = _blocks[i]->_x2;
        size_t prev_y2 = _blocks[i]->_y2;
        bool is_legal = true;
        _blocks[i]->_y1 = y;
        _blocks[i]->_y2 = _blocks[i]->_y1 + _blocks[i]->_height;
        
        for(size_t k = 2; k < _num_blocks + 2; ++k) {
        //for(size_t k = 0; k < i; ++k) {
          //size_t idx = sorted_idx_y[k];
          if(k != i) {
            if(_is_overlapped(_blocks[i], _blocks[k])) {
              is_legal = false;
              break;
            }
          }
        }
        if(!is_legal) {
          _blocks[i]->set_coordinate(prev_x1, prev_y1, prev_x2, prev_y2);
        }
        
      }
    }
  }
}

bool SP::_is_overlapped(Block* a, Block* b) {
  return 
    !(a->_x2 <= b->_x1 || a->_x1 >= b->_x2 || a->_y2 <= b->_y1 || a->_y1 >= b->_y2);
    //(((a->_x1 <= b->_x1) && (b->_x1 <= a->_x2) && (a->_y1 <= b->_y1) && (b->_y1 <= a->_y2)) ||
    //((a->_x1 <= b->_x1) && (b->_x1 <= a->_x2) && (a->_y1 <= b->_y2) && (b->_y2 <= a->_y2)) ||
    //((a->_x1 <= b->_x2) && (b->_x2 <= a->_x2) && (a->_y1 <= b->_y1) && (b->_y1 <= a->_y2)) ||
    //((a->_x1 <= b->_x2) && (b->_x2 <= a->_x2) && (a->_y1 <= b->_y2) && (b->_y2 <= a->_y2)));
}

void SP::_set_coordinate() {

  for(size_t i = 0; i < _num_blocks + 2; ++i) {
    _blocks[i]->set_coordinate(_length_h[i] - _blocks[i]->_width, _length_v[i] - _blocks[i]->_height, _length_h[i], _length_v[i]);
  }
}

void SP::_set_wirelength() {
  
  _wire_length = 0;
  for(auto& n: _nets) {
    float lg;

    size_t bx_min = (*std::min_element(n._blocks.begin(), n._blocks.end(), [](const Block* a, const Block* b){ return a->_x1 < b->_x1; }))->_x1;
    size_t bx_max = (*std::max_element(n._blocks.begin(), n._blocks.end(), [](const Block* a, const Block* b){ return a->_x2 < b->_x2; }))->_x2;

    size_t by_min = (*std::min_element(n._blocks.begin(), n._blocks.end(), [](const Block* a, const Block* b){ return a->_y1 < b->_y1; }))->_y1;
    size_t by_max = (*std::max_element(n._blocks.begin(), n._blocks.end(), [](const Block* a, const Block* b){ return a->_y2 < b->_y2; }))->_y2;

    if(!n._terminals.empty()) {
      auto tx_min_max = std::minmax_element(n._terminals.begin(), n._terminals.end(), [](const Terminal* a, const Terminal* b){ return a->_x < b->_x; });
      size_t tx_min = (*tx_min_max.first)->_x;
      size_t tx_max = (*tx_min_max.second)->_x;

      auto ty_min_max = std::minmax_element(n._terminals.begin(), n._terminals.end(), [](const Terminal* a, const Terminal* b){ return a->_y < b->_y; });
      size_t ty_min = (*ty_min_max.first)->_y;
      size_t ty_max = (*ty_min_max.second)->_y;

      size_t x_min = std::min(bx_min, tx_min);
      size_t x_max = std::max(bx_max, tx_max);
      size_t y_min = std::min(by_min, ty_min);
      size_t y_max = std::max(by_max, ty_max);
    }
    else {
      size_t x_min = bx_min;
      size_t x_max = bx_max;
      size_t y_min = by_min;
      size_t y_max = by_max;
      lg = (x_max - x_min) / 2.0f + (y_max - y_min) / 2.0f;
    }

    n.set_length(lg);
    _wire_length += lg;
  }
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

  for(size_t i = 0; i < _num_blocks; ++i) {
    auto id = _first_seq[i];
    auto sec_loc = _second_seq_id_loc_map[id];

    for(size_t k = 0; k < i; ++k) {
      auto id2 = _first_seq[k];
      if(sec_loc > _second_seq_id_loc_map[id2]) {
        // horizontal
        _blocks[id2]->_hconnects.push_back(_blocks[id]);
      }
      else {
        // vertical
        _blocks[id]->_vconnects.push_back(_blocks[id2]);
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

  //std::cerr << "check hconnect for the firt block: ";
  //for(auto& b: _blocks[2]->_hconnects) {
    //std::cerr << b->_id << " ";
  //}
  //std::cerr << "\n";

  //std::cerr << "check vconnect for the firt block: ";
  //for(auto& b: _blocks[2]->_vconnects) {
    //std::cerr << b->_id << " ";
  //}
  //std::cerr << "\n";
  
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

void SP::_move() {

  std::uniform_int_distribution<> _random_move(1, 4);
  std::uniform_int_distribution<> _random_idx(0, _num_blocks - 1);
  auto random = _random_move(_eng);
  
  switch(random) {
    case 1: {
      // move 1
      auto loc1 = _random_idx(_eng);
      auto loc2 = _random_idx(_eng);

      std::swap(_first_seq[loc1], _first_seq[loc2]);
      
    }
    case 2: {
      // move 2
      auto loc1 = _random_idx(_eng);
      auto loc2 = _random_idx(_eng);


      auto second_loc1 =  _second_seq_id_loc_map[_first_seq[loc1]];
      auto second_loc2 =  _second_seq_id_loc_map[_first_seq[loc2]];

      //_second_seq_id_loc_map[_first_seq[loc1]] = second_loc2;
      //_second_seq_id_loc_map[_first_seq[loc2]] = second_loc1;
      std::swap(_second_seq_id_loc_map[_first_seq[loc1]], _second_seq_id_loc_map[_first_seq[loc2]]);

      std::swap(_first_seq[loc1], _first_seq[loc2]);
      std::swap(_second_seq[second_loc1], _second_seq[second_loc2]);

    }

    case 3: {
      // move 3
      auto idx = _random_idx(_eng);
      auto* b = _blocks[_first_seq[idx]];
      std::swap(b->_height, b->_width);

    }
    case 4: {
      // new move
      auto loc1 = _random_idx(_eng);
      auto loc2 = _random_idx(_eng);
      std::swap(_second_seq_id_loc_map[_second_seq[loc1]], _second_seq_id_loc_map[_second_seq[loc2]]);

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

  std::getline(sstream, line);

  std::vector<std::string> tokens;
  while(std::getline(sstream, line)) {

    if(!line.empty() && line[line.length() - 1] == '\r') {
      line.erase(line.length() - 1);
    }

    if(line.size() > 1) {
      //line_stream >> std::ws;
      std::stringstream line_stream(line);
      std::string token;
      tokens.clear();

      std::cerr << "line: " << line << "\n";
      while(std::getline(line_stream, token, ' ')) {
        if(token != "" && token != " " && token != "\r") {
          tokens.push_back(token);
        }
      }

      if(line.find("terminal") != std::string::npos) {
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
    if(!line.empty() && line[line.length() - 1] == '\r') {
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
          std::cerr << "template: " << ttmp->second._name << "\n";
        }
        else {
          assert(false);
        }
      }
    }
  }

  if(degree != 0) {
    _nets.emplace_back(blocks, terminals, degree);
  }

  std::cerr << "num nets: " << _num_nets << "\n";
  std::cerr << _nets.size();
  assert(_nets.size() == _num_nets);
}

} // end of namespace ===================================================
