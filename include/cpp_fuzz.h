#ifndef CPPFUZZ_H
#define CPPFUZZ_H

#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <filesystem>
#include <boost/program_options.hpp>
#include <string.h>
#include <map>
#include <cstdlib>
#include <signal.h>
#include <unistd.h>

#define INCLUDE_PATH "-I../../include "
#define LIBRARY_PATH "-L../../lib "
#define LIBRARY_FILE "-lnowic -lrand "

void program_exe(std::string exe_file);
std::vector<std::string> generate_fuzz_inputs(const std::string& input, int range, int numInputs);
std::map<std::string, std::string> parse_input_options();
std::vector<std::string> find_files(std::string directory_path);
void compile_file(const std::string& filename);

#endif