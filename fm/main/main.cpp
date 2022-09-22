
#include  <src/circuit.hpp>
#include <iostream>


int main(int argc, char** argv) {

  if(argc != 4) {
    throw std::runtime_error("Number of parameters should be exact 3!\n ./fm input_file output_file 1/0 (enable multiple passes or not)");
  }
  std::string input_file = argv[1];
  std::ofstream output_file{argv[2]};
  int enabled = std::stoi(argv[3]);

  fm::Circuit circuit(input_file, enabled);
  circuit.fm();
  circuit.dump(output_file);

  
}
