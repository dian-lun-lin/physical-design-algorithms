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

#include <random>
#include <climits>

#include "parser.hpp"

enum Partition {
  A = 0,
  B
};

//enum CutType {
  //CUT = 0,
  //UNCUT
//};

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

    //CutType get_cut_type();
    //void set_cut_type(CutType ct);

  private:

    std::string _name;
    std::vector<Cell*> _connected_cells;

    // 0 -> partition a
    // 1 -> partition b
    //std::array<size_t, 2> _num_cells_in_partition{0, 0};

    //CutType _ct;
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

//CutType Net::get_cut_type() {
  //return _ct;
//}

//void Net::set_cut_type(CutType ct) {
  //_ct = ct;
  //return;
//}

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

    void set_fixed();
    bool is_fixed();

    void caculate_gain();

  private:

    std::string _name;
    std::vector<Net*> _nets;
    Partition _par;

    bool _is_fixed{false};
    int _gain{0};
    int _prev_gain{0};
    size_t _list_loc;
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

void Cell::set_fixed() {
  _is_fixed = true;
}

void Cell::caculate_gain() {
  int fs{0};
  int te{0};

  for(auto* n: _nets) {
    bool diff_par{false};
    bool same_par{false};

    for(auto* neighbor: n->get_cells()) {
      if(neighbor != this) {
        if(_par != neighbor->get_partition()) {
          diff_par = true;
        }
        else {
          same_par = true;
        }

        if(same_par && diff_par) {
          break;
        }
        
      }
    } 

    if(same_par && !diff_par) {
      ++fs;
    }
    else if(!same_par && diff_par) {
      ++te;
    }
  }

  _gain = fs - te;

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

    void _caculate_gain();

    void _initialize_partition();

    void _initialize_max_gain();

    void _set_bucket();

    Cell* _choose_candidate();

    int _update(Cell* cell);
  
    bool _check(Cell* cell);

    void _pass_reset();

    void _caculate_cut_size();

    //void _set_cut_uncut_nets();

    std::unordered_map<std::string, Net*> _nets;
    std::unordered_map<std::string, Cell*> _cells;
    float _balance_factor;
    std::filesystem::path _input_path;
    int _max_gain{0};
    size_t _cut_size{0};
    std::vector<std::list<Cell*>> _bucket;

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
    if (line != "\n") {
      std::stringstream line_stream(line);
      std::string token;
      tokens.clear();

      while(std::getline(line_stream, token, ' ')) {
        tokens.push_back(token);
      }

      // net
      Net* net = new Net(tokens[1]);
      _nets.insert({net->get_name(), net});

      // cells
      for(size_t i = 2; i < tokens.size(); ++i) {
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

void Circuit::fm() {

  _initialize_partition();
  _caculate_gain();
  _set_bucket();

  std::cerr << "finish initialization\n";

  // TODO: run multiple passes
  Cell* cand = _choose_candidate();
  int gain{0};
  while(cand != nullptr) {
    gain += _update(cand);
    _cand_gains.push_back({cand, gain});
    cand = _choose_candidate();
  }
  std::cerr << "finish fm pass\n";


  // find maximum and reverse
  int max{INT_MIN};
  int max_id{0};
  for(int i = _cand_gains.size() - 1; i >= 0; --i) {
    if(_cand_gains[i].second > max) {
      max = _cand_gains[i].second;
      max_id = i;
    }
  }

  for(int i = _cand_gains.size() - 1; i > max_id; --i) {
    _update(_cand_gains[i].first);
  }

  _caculate_cut_size();
}

int Circuit::_update(Cell* cand) {
  
  int gain{0};
  Partition par = cand->get_partition();
  cand->change_partition();
  cand->set_fixed();

  --_num_cells_in_partition[par];
  ++_num_cells_in_partition[(par + 1) % 2];
  std::chrono::time_point<std::chrono::steady_clock> tic;
  std::chrono::time_point<std::chrono::steady_clock> toc;

  std::unordered_set<Cell*> updates;

  for(auto* n: cand->get_nets()) {
    //--n->_num_cells_in_partition[par];
    //++n->_num_cells_in_partition[(par + 1) % 2];

    for(auto* c: n->get_cells()) {
      auto i_b = updates.insert(c);
      //if(i_b.second) {
      //}
    }
  }


  // update bucket
  //tic = std::chrono::steady_clock::now();
  for(auto* c: updates) {
    c->caculate_gain();

    //auto idx = _bucket[c->_prev_gain + _max_gain].begin();
    //std::advance(idx, c->_list_loc);

    if(c->_prev_gain != c->_gain) {
      _bucket[c->_prev_gain + _max_gain].remove(c);
      _bucket[c->_gain + _max_gain].push_back(c);
      c->_prev_gain = c->_gain;
    }

    //c->_list_loc = _bucket[c->_gain + _max_gain].size() - 1;

    // update prev_gain
  }
  //toc = std::chrono::steady_clock::now();
  //std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(toc - tic).count() << "\n";


  return gain;
}

void Circuit::_set_bucket() {
  _initialize_max_gain();
  _bucket.resize(_max_gain * 2 + 1);
  
  // gain -1 s maped to 1
  // gain 2 is maped to 4

  for(auto&& s_c: _cells) {
    Cell* c = s_c.second;
    _bucket[c->_gain + _max_gain].push_back(c);
    c->_list_loc = _bucket[c->_gain + _max_gain].size() - 1;
  }
}

Cell* Circuit::_choose_candidate() {

  for(int i = _max_gain * 2; i >= 0; --i) {
    if(!_bucket[i].empty()) {
      Cell* cand = _bucket[i].front();
      // check if the cell meet balance criterion
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
      valid = (!cell->is_fixed()) &&
      ((_cells.size() * (1 - _balance_factor) / 2) < (_num_cells_in_partition[0] - 1)) &&
      ((_num_cells_in_partition[0] - 1) <= (_cells.size() * (1 + _balance_factor) / 2));
      break;

    case Partition::B:
      valid = (!cell->is_fixed()) &&
      ((_cells.size() * (1 - _balance_factor) / 2) < (_num_cells_in_partition[1] - 1)) &&
      ((_num_cells_in_partition[1] - 1) <= (_cells.size() * (1 + _balance_factor) / 2));
      break;
  }

  return valid;

}

void Circuit::_initialize_partition() {
  // random
  std::mt19937 eng;
  std::uniform_int_distribution<> distr(0, 1);
  std::array<Partition, 2> choose{Partition::A, Partition::B}; 

  for(auto&& s_c: _cells) {
    auto random = distr(eng);

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

void Circuit::_initialize_max_gain() {
  _max_gain = 0;

  for(auto&& s_c: _cells) {
    Cell* c = s_c.second;
    if(c->get_nets().size() > _max_gain) {
      _max_gain = c->get_nets().size();
    }
  }
}

void Circuit::_caculate_cut_size() {

  _cut_size = 0;

  for(auto&& s_n: _nets) {
    Net* n = s_n.second;
    bool is_cut_uncut{true};

    std::array<size_t, 2> pars{0, 0};
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
        _cut_size += 1;
        break;
      }
    }

  }
}

void Circuit::_caculate_gain() {
  for(auto&& s_c: _cells) {
    Cell* c = s_c.second;
    c->caculate_gain();
  }
  return;
}




