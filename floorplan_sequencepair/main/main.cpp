#include  <src/sp.hpp>
#include <iostream>


int main(int argc, char** argv) {

  if(argc != 5) {
    throw std::runtime_error("Number of parameters should be exact 4!\n ./sp alpha input_block_file input_net_file output_file");
  }
  float alpha = std::stof(argv[1]);
  std::string blockf = argv[2];
  std::string netf = argv[3];
  std::ofstream output_file{argv[4]};

  fp::SP algo(alpha, blockf, netf);
  algo.apply();
  algo.dump(output_file);
  
}
