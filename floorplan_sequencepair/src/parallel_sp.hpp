#pragma once
#include <declarations.h>
#include <src/utility.hpp>
#include <src/block.hpp>
#include <src/terminal.hpp>
#include <src/net.hpp>
#include <src/sp.hpp>

namespace fp { // begin of namespace ====================================

class ParallelSP {

  public:

    ParallelSP(
      const float alpha,
      const std::filesystem::path& blockf, 
      const std::filesystem::path& netf,
      const size_t num_threads = 8
    );

    void apply();
    void dump(std::ostream& os);

  private:

    std::vector<SP> _sps;
    size_t _num_threads;
  
    SP* _best_sp{nullptr};

    void _update_best();
    void _set_average();
    std::random_device _rd{};
    std::mt19937 _eng{_rd()};

};

ParallelSP::ParallelSP(
  const float alpha,
  const std::filesystem::path& blockf, 
  const std::filesystem::path& netf,
  const size_t num_threads
): _num_threads{num_threads} {


  for(size_t i = 0; i < _num_threads; ++i) {
    //_sps.emplace_back(alpha, blockf, netf);
    _sps.emplace_back(alpha, blockf, netf, _eng);
  }

  _set_average();
}

void ParallelSP::_set_average() {
  size_t area_average = _sps[0]._area_average;
  size_t wire_length_average = _sps[0]._wire_length_average;
  size_t penalty_average = _sps[0]._penalty_average;


  for(auto& sp: _sps) {
    sp._area_average = area_average;
    sp._wire_length_average += wire_length_average;
    sp._penalty_average += penalty_average;
  }
}

void ParallelSP::_update_best() {

  //_best_sp = &(*std::min_element(
    //_sps.begin(), 
    //_sps.end(), 
    //[](SP& a, SP& b) { a._cost < b._cost; }
  //));
  _best_sp = &_sps[0];

  for(auto& sp: _sps) {
    if(_best_sp->_cost > sp._cost) {
      _best_sp = &sp;
    }
  }

  for(auto& sp: _sps) {
    sp._best_first_seq = _best_sp->_best_first_seq;
    sp._best_second_seq = _best_sp->_best_second_seq;
    sp._best_first_seq_id_loc_map = _best_sp->_best_first_seq_id_loc_map;
    sp._best_second_seq_id_loc_map = _best_sp->_best_second_seq_id_loc_map;
    sp._best_block_wh = _best_sp->_best_block_wh;

    sp._update_all_to_best();
  }
  
}

void ParallelSP::apply() {

  //_update_best();

  size_t width = _sps[0]._outline.first;
  size_t height = _sps[0]._outline.second;

  bool is_legal{false};
  size_t count{0};


  omp_set_num_threads(_num_threads);

  while(!is_legal && count < 1000) {

    #pragma omp parallel for
    for(size_t i = 0; i < _sps.size(); ++i) {
      std::cerr << omp_get_thread_num() << "\n";
      _sps[i].apply();
    }

    _update_best();
    ++count;
    is_legal = (_best_sp->_chip_width <= width) && (_best_sp->_chip_height <= height);
    std::cerr << "cost: " << _best_sp->_cost << "\n";
    std::cerr << "is legal: " << is_legal << "\n";
  }

  _best_sp->_build_connections();
  _best_sp->_get_results();

  //double out_width{0};
  //double out_height{0};
  //if(width < _best_sp->_chip_width) {
    //out_width= _chip_width - _outline.first;

  //}
  //if(_outline.second < _chip_height) {
    //out_height = _chip_height - _outline.second;
  //}
  //std::cerr << "best penalty: " << _best_cost << "\n";

  std::cerr << "best outline: " << "width: " << _best_sp->_chip_width << " height: " << _best_sp->_chip_height << "\n"; 
}

void ParallelSP::dump(std::ostream& os) {
  _best_sp->dump(os);
}

} // end of namespace ===================================================
