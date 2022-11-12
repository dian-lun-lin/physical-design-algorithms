#include  <src/sp.hpp>
#include <iostream>


int main(int argc, char** argv) {

  if(argc != 4) {
    throw std::runtime_error("Number of parameters should be exact 3!\n ./sp input_block_file input_net_file output_file");
  }
  std::string blockf = argv[1];
  std::string netf = argv[2];
  std::ofstream output_file{argv[3]};

  sp::SP(blockf, netf);
  
}
