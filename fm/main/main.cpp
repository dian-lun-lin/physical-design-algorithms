
#include  <src/circuit.hpp>
#include <iostream>


int main(int argc, char** argv) {

  std::string input_file = argv[1];
  std::ofstream output_file{argv[2]};

  fm::Circuit circuit(input_file);
  circuit.fm();
  circuit.dump(output_file);

  
}
