
#include <src/stree.hpp>
#include <fstream>

int main(int argc, char** argv) {
  if(argc != 3) {
    throw std::runtime_error("Number of parameters should be exact 2!\n");
  }
  std::filesystem::path input_file{argv[1]};
  std::ofstream output_file{argv[2]};
  st::Stree st(input_file);
  st.apply();
  st.dump(output_file);
}
