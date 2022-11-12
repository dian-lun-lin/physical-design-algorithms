#pragma once
#include <declarations.h>
#include <src/utility.hpp>
#include <src/block.hpp>
#include <src/net.hpp>

namespace sp { // begin of namespace ====================================

class SP {

  public:

    SP(
      const std::filesystem::path& blockf, 
      const std::filesystem::path& netf
    );

  private:

    void _parse_block(
      const std::filesystem::path& blockf
    );

    void _parse_net(
      const std::filesystem::path& netf
    );

    std::vector<Block> _blocks;
    std::vector<Net> _nets;
    size_t _num_blocks;
    std::pair<size_t, size_t> _outline;
    size_t _num_terminals;
};

SP::SP(
  const std::filesystem::path& blockf, 
  const std::filesystem::path& netf
) {
  _parse_block(blockf);
  _parse_net(netf);
}

void SP::_parse_block(
  const std::filesystem::path& blockf
) {
  auto sstream = read_file_to_sstream(blockf);
  std::string line;
  std::string tmp;


  // outline
  std::getline(sstream, line);
  if(line.find("Outline") != std::string::npos) {
    line = line.substr(line.find(": ") + 1);
    std::stringstream line_stream(line);

    std::getline(line_stream, tmp, ' ');
    std::cerr << line << "\n";
    _outline.first = std::stoi(tmp);
    std::getline(line_stream, tmp, ' ');
    _outline.second = std::stoi(tmp);

  }
  else {
    assert(false);
  }
  // numblocks
  if(line.find("NumBlocks") != std::string::npos) {
    _num_blocks = std::stoi(line.substr(line.find(": ") + 1));
  }
  else {
    assert(false);
  }
  // numterminals
  if(line.find("NumTerminals") != std::string::npos) {
    _num_terminals = std::stoi(line.substr(line.find(": ") + 1));
  }
  else {
    assert(false);
  }

  std::cerr << "outline: "              << _outline.first << ", " << _outline.second << "\n"
            << "number of blocks: "     << _num_blocks
            << "number of terminals: "  << _num_terminals;


  //std::vector<std::string> tokens;
  //while(std::getline(sstream, line, ';')) {
    //line.erase(std::remove(line.begin(), line.end(), '\n'), line.cend());
    //std::stringstream line_stream(line);
    //std::string token;
    //tokens.clear();

    //line_stream >> std::ws;
    //while(std::getline(line_stream, token, ' ')) {
      //tokens.push_back(token);
    //}

    //// net
    //Net* net = new Net(tokens[1]);
    //_nets.insert({net->get_name(), net});

    //// cells
    //for(size_t i = 2; i < tokens.size(); ++i) {
      //if(tokens[i] != "") {
        //auto iter = _cells.find(tokens[i]);
        //Cell* cell{nullptr};

        //if(iter == _cells.end()) {
          //cell = new Cell(tokens[i]);
        //}
        //else {
          //cell = (*iter).second;
        //}

        //_cells.insert({cell->get_name(), cell});
        //net->add_cell(cell);
        //cell->add_net(net);
      //}
    //}
  //}
}

void SP::_parse_net(
  const std::filesystem::path& netf
) {
}


} // end of namespace ===================================================
