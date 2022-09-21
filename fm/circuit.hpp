#pragma once

#include <array>
#include <list>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <iostream>
#include <string>
#include <filesystem>
#include <cassert>
#include <fstream>

#include <algorithm>
#include <random>
#include <climits>

#include "parser.hpp"

enum Partition {
  A = 0,
  B
};

class Cell;
class Net;
class Circuit;

// ==============================================================================
//
// Declaration of class Net
//
// ==============================================================================

class Net {

  friend class Circuit;

  public:

    Net(const std::string& name);

    const std::vector<Cell*>& get_cells();

    std::string get_name();

    void add_cell(Cell* cell);

  private:

    std::string _name;
    std::vector<Cell*> _connected_cells;

    // TODO: should we pre-find critical path?
    //bool _is_critical{false};
};

// ==============================================================================
//
// Definition of class Net
//
// ==============================================================================

Net::Net(const std::string& name): _name{name} {
}

const std::vector<Cell*>& Net::get_cells() {
  return _connected_cells;
}

std::string Net::get_name() {
  return _name;
}

void Net::add_cell(Cell* cell) {
  _connected_cells.push_back(cell);
  return;
}

// ==============================================================================
//
// Declaration of class Cell
//
// ==============================================================================

class Cell {

  friend class Circuit;

  public:
  
    Cell(const std::string& name);

    const std::vector<Net*>& get_nets();

    std::string get_name();

    void add_net(Net*);

    void set_partition(Partition par);

    Partition get_partition();

    void change_partition();

    void set_fixed(bool fix);
    bool is_fixed();

    void caculate_gain();

  private:

    std::string _name;
    std::vector<Net*> _nets;

    bool _is_fixed{false};
    Partition _par;
    int _gain{0};
    int _prev_gain{0};
    std::list<Cell*>::iterator _loc;
};




// ==============================================================================
//
// Definition of class Cell
//
// ==============================================================================


Cell::Cell(const std::string& name): _name{name} {
}

const std::vector<Net*>& Cell::get_nets() {
  return _nets;
}

std::string Cell::get_name() {
  return _name;
}

void Cell::add_net(Net* net) {
  _nets.push_back(net);
  return;
}

Partition Cell::get_partition() {
  return _par;
}

void Cell::set_partition(Partition par) {
  _par = par;
}

void Cell::change_partition() {
  switch(_par) {
    case Partition::A:
      _par = Partition::B;
      break;
    case Partition::B:
      _par = Partition::A;
      break;
  }
}

bool Cell::is_fixed() {
  return _is_fixed;
}

void Cell::set_fixed(bool fix) {
  _is_fixed = fix;
}

void Cell::caculate_gain() {
  _gain = 0;

  for(auto* n: _nets) {
    int from{0};
    int to{0};

    for(auto* c: n->get_cells()) {
      if(_par != c->get_partition()) {
        ++to;
      }
      else {
        ++from;
      }

      if(to > 1 && from > 1) {
        break;
      }
    } 

    if(from == 1) {
      ++_gain;
    }
    if(to == 0) {
      --_gain;
    }
  }

  return;
}


// ==============================================================================
//
// Declaration of class Circuit
//
// ==============================================================================


class Circuit {

  public:

    Circuit(const std::string& input_path);

    ~Circuit();

    void fm();

    void dump(std::ostream& os);


  private:

    void _parse();

    void _initialize_cells();

    void _initialize_partition();

    void _initialize_max_gain();

    void _initialize_buckets();

    void _reset_pass();

    Cell* _choose_candidate();

    bool _check(Cell* cell);

    void _update(Cell* cell);

    void _reverse();

    void _undo(Cell* cand);

    void _caculate_cut_size();

    void _set_max_gain();

    std::unordered_map<std::string, Net*> _nets;
    std::unordered_map<std::string, Cell*> _cells;
    float _balance_factor;
    std::filesystem::path _input_path;
    int _max_gain{0};
    size_t _cut_size{0};
    std::vector<std::list<Cell*>> _bucket_a;
    std::vector<std::list<Cell*>> _bucket_b;

    // 0 -> partition a
    // 1 -> partition b
    std::array<size_t, 2> _num_cells_in_partition{0, 0};

    std::vector<std::pair<Cell*, int>> _cand_gains;
};

// ==============================================================================
//
// Definition of class Circuit
//
// ==============================================================================

Circuit::Circuit(const std::string& input_path):_input_path{input_path} {
  _parse();
}

Circuit::~Circuit() {

  for(auto&& n: _nets) {
    delete n.second;
  }

  for(auto&& c: _cells) {
    delete c.second;
  }
}

void Circuit::fm() {

  _initialize_partition();
  _set_max_gain();

  std::cerr << "finish initialization\n";
  std::cerr << "Maximum available gain: " << _max_gain << "\n";
  _caculate_cut_size();
  std::cerr << "Initial cut size: " << _cut_size << "\n";


  for(size_t p = 0; p < 1; ++p) {
    std::cerr << "Pass: " << p << "\n";
    int gain{0};
    _reset_pass();
    std::cerr << "finish resetting\n";

    Cell* cand = _choose_candidate();

    while(cand != nullptr) {
      gain += cand->_gain;
      _update(cand);
      _cand_gains.push_back({cand, gain});
      cand = _choose_candidate();
    }

    _reverse();
    _caculate_cut_size();
    std::cerr << "current cut size: " << _cut_size << "\n";
  }

  std::cerr << "finish fm pass\n";
}

void Circuit::dump(std::ostream& os) {

  std::vector<std::string> _cells_par_a;
  std::vector<std::string> _cells_par_b;
  _cells_par_a.reserve(_num_cells_in_partition[0]);
  _cells_par_b.reserve(_num_cells_in_partition[1]);

  for(auto&& s_c: _cells) {
    if(s_c.second->get_partition() == Partition::A) {
      _cells_par_a.push_back(s_c.first);
    }
    else {
      _cells_par_b.push_back(s_c.first);
    }
  }

  os << "Cutsize = " << _cut_size << "\n";
  os << "G1 " << _num_cells_in_partition[0] << "\n"; 

  for(auto&& str: _cells_par_a) {
    os << str << " ";
  }
  os << ";\n";

  os << "G2 " << _num_cells_in_partition[1] << "\n"; 

  for(auto&& str: _cells_par_b) {
    os << str << " ";
  }
  os << ";\n";
  
  
}


void Circuit::_parse() {
  auto sstream = read_file_to_sstream(_input_path);
  std::string line;

  // first line is balance factor
  std::getline(sstream, line);
  _balance_factor = std::stof(line);

  std::vector<std::string> tokens;
  while(std::getline(sstream, line, ';')) {
    line.erase(std::remove(line.begin(), line.end(), '\n'), line.cend());
    std::stringstream line_stream(line);
    std::string token;
    tokens.clear();

    line_stream >> std::ws;
    while(std::getline(line_stream, token, ' ')) {
      tokens.push_back(token);
    }

    // net
    Net* net = new Net(tokens[1]);
    _nets.insert({net->get_name(), net});

    // cells
    for(size_t i = 2; i < tokens.size(); ++i) {
      if(tokens[i] != "") {
        auto iter = _cells.find(tokens[i]);
        Cell* cell{nullptr};

        if(iter == _cells.end()) {
          cell = new Cell(tokens[i]);
        }
        else {
          cell = (*iter).second;
        }

        _cells.insert({cell->get_name(), cell});
        net->add_cell(cell);
        cell->add_net(net);
      }
    }
  }
}

void Circuit::_set_max_gain() {
  _max_gain = 0;
  for(auto&& s_c: _cells) {
    Cell* c = s_c.second;
    if(c->get_nets().size() > _max_gain) {
      _max_gain = c->get_nets().size();
    }
  }
  return;
}

void Circuit::_initialize_cells() {
  _cand_gains.clear();
  _cand_gains.reserve(_cells.size());

  for(auto&& s_c: _cells) {
    Cell* c = s_c.second;
    c->caculate_gain();
    c->_prev_gain = c->_gain;
    c->set_fixed(false);
  }
  return;
}

// random
void Circuit::_initialize_partition() {
  srand(time(NULL));
  //std::random_device rd{};
  //std::mt19937 eng(rd());
  //std::mt19937 eng;
  //std::uniform_int_distribution<> distr(0, 1);
  std::array<Partition, 2> choose{Partition::A, Partition::B}; 

  for(auto&& s_c: _cells) {
    //auto random = distr(eng);
    int random = rand() % 2;

    Cell* c = s_c.second;
    c->_gain = 0;
    c->set_partition(choose[random]);

    ++_num_cells_in_partition[random];
  }

  assert(
    (_num_cells_in_partition[0] + _num_cells_in_partition[1]) == _cells.size()
  );

  return;
}

void Circuit::_initialize_buckets() {
  _bucket_a.clear();
  _bucket_b.clear();
  _bucket_a.resize(_max_gain * 2 + 1);
  _bucket_b.resize(_max_gain * 2 + 1);
  
  for(auto&& s_c: _cells) {
    Cell* c = s_c.second;
    switch(c->get_partition()) {
      case Partition::A:
        _bucket_a[c->_gain + _max_gain].push_back(c);
        c->_loc = _bucket_a[c->_gain + _max_gain].end();
        --c->_loc;
        break;
      case Partition::B:
        _bucket_b[c->_gain + _max_gain].push_back(c);
        c->_loc = _bucket_b[c->_gain + _max_gain].end();
        --c->_loc;
        break;
    }
  }
}

void Circuit::_reset_pass() {
  _initialize_cells();
  _initialize_buckets();
}

void Circuit::_update(Cell* cand) {
  
  Partition prev_par = cand->get_partition();
  cand->change_partition();
  --_num_cells_in_partition[prev_par];
  ++_num_cells_in_partition[(prev_par + 1) % 2];

  //std::cerr << "1111111\n";
  //std::chrono::time_point<std::chrono::steady_clock> tic;
  //std::chrono::time_point<std::chrono::steady_clock> toc;


  //tic = std::chrono::steady_clock::now();
  // =======================================================  
  //  find critical nets and update corresponding cells
  // =======================================================  
  for(auto* n: cand->get_nets()) {
    int prev_from{1};
    int prev_to{0};
    for(auto* c: n->get_cells()) {
      if(c != cand) {
        if(prev_par != c->get_partition()) {
          ++prev_to;
        }
        else {
          ++prev_from;
        }

        if(prev_to > 2 && prev_from > 2) {
          break;
        }
      }
    }


    // case 1 before move
    if(prev_to == 0) {
      for(auto* c: n->get_cells()) {
        if(!c->is_fixed()) {
          ++c->_gain;
        }
      }
    }
    // case 2 before move
    else if(prev_to == 1) {
      for(auto* c: n->get_cells()) {
        if(!c->is_fixed() && c->get_partition() != prev_par) {
          --c->_gain;
        }
      }
    }

    int from = prev_from - 1;
    int to = prev_to + 1;

    // case 1 after move
    if(from == 0) {
      for(auto* c: n->get_cells()) {
        if(!c->is_fixed()) {
          --c->_gain;
        }
      }
    }
    // case 2 after move
    else if (from == 1) {
      for(auto* c: n->get_cells()) {
        if(!c->is_fixed() && c->get_partition() == prev_par) {
          ++c->_gain;
        }
      }
    }
  }
  //toc = std::chrono::steady_clock::now();
  //std::cerr << "update gain time: " << std::chrono::duration_cast<std::chrono::microseconds>(toc - tic).count() << "\n";

  //tic = std::chrono::steady_clock::now();
  for(auto* n: cand->get_nets()) {
    for(auto* c: n->get_cells()) {
      if((!c->is_fixed()) && (c->_prev_gain != c->_gain)) {

        switch(c->get_partition()) {
          case Partition::A:
            _bucket_a[c->_gain + _max_gain].push_back(c);
            _bucket_a[c->_prev_gain + _max_gain].erase(c->_loc);
            c->_loc = _bucket_a[c->_gain + _max_gain].end();
            --c->_loc;
            break;
          case Partition::B:
            _bucket_b[c->_gain + _max_gain].push_back(c);
            _bucket_b[c->_prev_gain + _max_gain].erase(c->_loc);
            c->_loc = _bucket_b[c->_gain + _max_gain].end();
            --c->_loc;
            break;
        }

        c->_prev_gain = c->_gain;
      }
    }
  }
  //std::cerr << "33333333\n";
  //toc = std::chrono::steady_clock::now();
  //std::cerr << "update bucket time: " << std::chrono::duration_cast<std::chrono::microseconds>(toc - tic).count() << "\n";

  return;
}

Cell* Circuit::_choose_candidate() {

  Cell* cand = nullptr;
  for(int i = _max_gain * 2; i >= 0; --i) {

    while(!_bucket_a[i].empty() || !_bucket_b[i].empty()) {
      if(!_bucket_a[i].empty()) {
        cand = _bucket_a[i].front();
        _bucket_a[cand->_gain + _max_gain].erase(_bucket_a[cand->_gain + _max_gain].begin());
      }
      else {
        cand = _bucket_b[i].front();
        _bucket_b[cand->_gain + _max_gain].erase(_bucket_b[cand->_gain + _max_gain].begin());
      }

      cand->set_fixed(true);

      if(_check(cand)) {
        return cand;
      }
    }
  }

  return nullptr;
}

bool Circuit::_check(Cell* cell) {


  bool valid{false};

  switch(cell->_par) {

    case Partition::A:
      valid =
      ((_cells.size() * (1 - _balance_factor) / 2) < (_num_cells_in_partition[0] - 1)) &&
      ((_num_cells_in_partition[1] + 1) < (_cells.size() * (1 + _balance_factor) / 2));
      break;

    case Partition::B:
      valid = 
      ((_cells.size() * (1 - _balance_factor) / 2) < (_num_cells_in_partition[1] - 1)) &&
      ((_num_cells_in_partition[0] + 1) < (_cells.size() * (1 + _balance_factor) / 2));
      break;
  }

  return valid;

}

// find maximum total gain and reverse
void Circuit::_reverse() {
  int max{INT_MIN};
  int max_id{0};
  for(int i = _cand_gains.size() - 1; i >= 0; --i) {
    if(_cand_gains[i].second > max) {
      max = _cand_gains[i].second;
      max_id = i;
    }
  }

  for(int i = _cand_gains.size() - 1; i > max_id; --i) {
    _undo(_cand_gains[i].first);
  }
}

void Circuit::_undo(Cell* cand) {
  Partition prev_par = cand->get_partition();
  cand->change_partition();
  --_num_cells_in_partition[prev_par];
  ++_num_cells_in_partition[(prev_par + 1) % 2];
}


void Circuit::_caculate_cut_size() {

  _cut_size = 0;

  for(auto&& s_n: _nets) {
    Net* n = s_n.second;

    std::array<int, 2> pars{0, 0};
    for(auto* c: n->get_cells()) {

      switch(c->get_partition()) {
        case Partition::A:
          ++pars[0];
          break;
        case Partition::B:
          ++pars[1];
          break;
      }

      if(pars[0] != 0 && pars[1] != 0) {
        ++_cut_size;
        break;
      }
    }

  }
}

