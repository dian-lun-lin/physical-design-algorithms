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
    void _update_average();
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

  _update_average();
}

void ParallelSP::_update_average() {
  double area_average{0};
  double wire_length_average{0};
  double penalty_average{0};

  for(auto& sp: _sps) {
    area_average += sp._area_average;
    wire_length_average += sp._wire_length_average;
    penalty_average += sp._penalty_average;
  }

  area_average /= _sps.size();
  wire_length_average /= _sps.size();
  penalty_average /= _sps.size();

  for(auto& sp: _sps) {
    sp._area_average = area_average;
    sp._wire_length_average = wire_length_average;
    sp._penalty_average = penalty_average;
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
    sp._cost = _best_sp->_best_cost;

    sp._update_all_to_best();
  }

  _update_average();
}

void ParallelSP::apply() {

  //_update_best();


  size_t width = _sps[0]._outline.first;
  size_t height = _sps[0]._outline.second;


  std::cout << "===================================================================================\n\n"
            << "                            Sequence-pair Placement           \n\n"
            << "./sp alpha input_file output_file num_threads (number of threads to find solution) \n\n"
            << "#1. I randomly initialize sequence pair and apply SA to improve cost.\n\n"
            << "#2. The number of thread should be always larger than 0. \n\n"
            << "#3. I apply openmp to find solution in parallel.\n"
            << "Each thread will create its sequence pair and perform SA. \n"
            << "After SA, I will find the best solution and update each thread's sequence pair to the best.\n\n"
            << "#4. I run SA at most 30 times.\n\n"
            << "#5. My cost function only considers whether the solution is legal.\n"
            << "I will jump out of SA loop once I find a legal solution.\n\n"
            << "#6. Finally, I will apply compress() to get the best result.\n\n"
            << "====================================================================================\n\n";


  std::cout << "Outline: "              << _sps[0]._outline.first << ", "   << _sps[0]._outline.second << "\n"
            << "Number of blocks: "     << _sps[0]._num_blocks    << "\n"
            << "Number of terminals: "  << _sps[0]._num_terminals << "\n"
            << "Number of threads: "    << _num_threads           << "\n\n";

  std::cout << "===========================\n\n";

  bool is_legal{false};
  size_t count{0};


  omp_set_num_threads(_num_threads);

  while(!is_legal && count < 30) {

    #pragma omp parallel for
    for(size_t i = 0; i < _sps.size(); ++i) {
      _sps[i].apply();
    }

    _update_best();

    std::cout << "best cost at iter "      << count << ": " << _best_sp->_cost << "\n";

    std::cout << "best chip area at iter " << count << ": " << "width: "       
              << _best_sp->_chip_width << " height: " << _best_sp->_chip_height << "\n\n"; 
    ++count;
    is_legal = (_best_sp->_chip_width <= width) && (_best_sp->_chip_height <= height);
  }


  //double out_width{0};
  //double out_height{0};
  //if(width < _best_sp->_chip_width) {
    //out_width= _chip_width - _outline.first;

  //}
  //if(_outline.second < _chip_height) {
    //out_height = _chip_height - _outline.second;
  //}
  //std::cerr << "best penalty: " << _best_cost << "\n";

}

void ParallelSP::dump(std::ostream& os) {
  std::cout << "dumping...\n";
  _best_sp->dump(os);
}

} // end of namespace ===================================================
