
#include <src/stree.hpp>

int main(int argc, char** argv) {
  if(argc != 2) {
    throw std::runtime_error("Number of parameters should be exact 2!\n");
  }
  std::filesystem::path input_file{argv[1]};
  st::Stree st(input_file);
  st.apply();
  st.dump(std::cerr);
}
