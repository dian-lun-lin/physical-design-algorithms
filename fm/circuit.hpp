#pragma once

#include <array>
#include <list>
#include <vector>
#include <unordered_map>

#include <string>
#include <filesystem>
#include <fstream>

#include <random>

#include "parser.hpp"

enum Partition {
  A,
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

    Partition get_partition();

    void set_partition(Partition par);


  private:

    std::string _name;
    std::vector<Cell*> _connected_cells;
    Partition _par;
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

Partition Net::get_partition() {
  return _par;
}

void Net::set_partition(Partition par) {
  _par = par;
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

  private:

    std::string _name;
    std::vector<Net*> _nets;
    Partition _par;

    bool _is_locked{false};
    int _gain{0};
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


  private:

    void _parse();

    void _caculate_gain();

    void _check_balance();

    void _initialize_partition();

    void _initialize_max_gain();

    void _set_bucket();

    void _choose_and_update();

    std::unordered_map<std::string, Net*> _nets;
    std::unordered_map<std::string, Cell*> _cells;
    float _balance_factor;
    std::filesystem::path _input_path;
    int _max_gain{0};
    std::vector<std::list<Cell*>> _bucket;

  
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


void Circuit::_parse() {
  auto sstream = read_file_to_sstream(_input_path);
  std::string line;

  // first line is balance factor
  std::getline(sstream, line);
  _balance_factor = std::stof(line);

  std::vector<std::string> tokens;
  while(std::getline(sstream, line)) {
    std::stringstream line_stream(line);
    std::string token;
    tokens.clear();

    while(std::getline(line_stream, token, ';')) {
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

void Circuit::fm() {
  _initialize_partition();
  _caculate_gain();
  _set_bucket();
}

void Circuit::_set_bucket() {
  _initialize_max_gain();
  _bucket.resize(_max_gain * 2 + 1);
  
  // gain -1 s maped to 1
  // gain 2 is maped to 4
  for(auto&& s_c: _cells) {
    Cell* c = s_c.second;
    if(c->_gain < 0) {
      _bucket[c->_gain * -1].push_back(c);
    }
    else {
      _bucket[c->_gain * 2].push_back(c);
    }
  }
}

void Circuit::_choose_and_update() {
  for(size_t i = _max_gain * 2; i >= 0; --i) {
    if(!_bucket[i].empty()) {
    }
  }
}

void Circuit::_initialize_partition() {
  // random
  std::mt19937 eng;
  std::uniform_int_distribution<> distr(0, 1);
  std::array<Partition, 2> choose{Partition::A, Partition::B}; 

  for(auto&& s_c: _cells) {
    Cell* c = s_c.second;
    c->_gain = 0;
    c->set_partition(choose[distr(eng)]);
  }

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

void Circuit::_caculate_gain() {
  for(auto&& s_c: _cells) {
    Cell* c = s_c.second;
    for(auto* n: c->get_nets()) {
      for(auto* cc: n->get_cells()) {
        if(c != cc) {
          if(c->get_partition() != cc->get_partition()) {
            ++c->_gain;
          }
          else {
            --c->_gain;
          }
        }
      }
    }


  }

  return;
}

void Circuit::_check_balance() {
}


