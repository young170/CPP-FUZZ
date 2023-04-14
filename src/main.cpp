/**
 * @file main.cpp
 * @author Seongbin Kim (seongbin10209@gmail.com)
 * @brief Fuzzer for programs written in cpp
 * @version 0.1
 * @date 2023-04-15
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "cpp_fuzz.h"

namespace fs = std::filesystem;



int main(int argc, char* argv[]) {
    std::map<std::string, std::string> input_options = parse_input_options();

	// get the directory path of the to-be-compiled files, input_options.directory
    std::string directory_path = input_options.at("directory_path");
	// get the executable files of the compiled files
    std::vector<std::string> cpp_files = find_files(directory_path);

	// compile all .cpp files
    for (const std::string& cpp_file : cpp_files) {
        compile_file(cpp_file);
    }

	// exe_files have ".out" appended to the filename
	std::vector<std::string> exe_files;
	for (std::string& exe_file : exe_files) {
        exe_file += ".out";
    }

	// execute each .out executable binary file
	for (const std::string exe_file : exe_files) {
        program_exe(exe_file);
    }

    return 0;
}

//////////////////////////////////////////////////////////////
// main functions: execute program & generate fuzzed inputs
//////////////////////////////////////////////////////////////

void program_exe(std::string exe_file) {
	int to_child_FD[2];
	int to_parent_FD[2];

	pid_t child_pid;
	int status;

	// initialize int arrays as pipe
	if (pipe(to_child_FD) != 0) {	// non-zero == error
		perror("Pipe Error");
		exit(1);
	}
	if (pipe(to_parent_FD) != 0) {
		perror("Pipe Error");
		exit(1);
	}

	child_pid = fork();
	if (child_pid < 0) {
		fprintf(stderr, "fork failed..\n");
		exit(1);
	} else if (child_pid == 0) {
		// use pipe() to redirect stdin input and stderr output
		close(to_child_FD[1]);	// close write
		close(to_parent_FD[0]);	// close read

		// dup2(source, target)
		// replaces target to the sources
		// ex) dup2(pipes[1], STDOUT_FILENO) - stdout is redirected to pipes[1]
        dup2(to_child_FD[0], STDIN_FILENO);		// macro : 0
		dup2(to_parent_FD[1], STDERR_FILENO);	// macro : 2

		// get the fuzz inputs
		std::vector<std::string> fuzz_inputs = generate_fuzz_inputs();

		// child process executes binary file
		// the *argv[] for the child process is generated from the fuzz_options() function
		if (execv(exe_file, fuzz_inputs) == -1) {
			fprintf(stderr, "child process failed..\n");
			exit(1);
		}
	}

	// parent process 
	close(to_child_FD[0]);	// close read
	close(to_parent_FD[1]);	// close write

	// write to pipe_to_child - stdin
	write(to_child_FD[1], input, strlen(input));

	// need to close write pipe - or else it constantly waits for input
	close(to_child_FD[1]);

	waitpid(child_pid, &status, 0);

	char* buf = (char*) malloc(sizeof(char) * BUFSIZ);
	// recieve the output of child via stderr using unnamed pipe
	int num_of_bytes = read(to_parent_FD[0], buf, BUFSIZ);
	buf[num_of_bytes] = '\0';
	// printf("%s\n", buf);

	close(to_parent_FD[0]);
}

std::vector<std::string> generate_fuzz_inputs(const std::string& input, int range, int numInputs) {
	std::random_device rd;  // obtain a random seed from the system
    std::mt19937 gen(rd()); // seed the random number generator
    std::uniform_int_distribution<> dis(-range, range); // define the range of mutation

    std::vector<std::string> fuzzedInputs(numInputs); // create a vector to hold the fuzzing inputs
    for (int i = 0; i < numInputs; i++) {
        int mutatedInput = std::stoi(input) + dis(gen); // generate a mutated input within the given range
        if (mutatedInput < 0) { // ensure mutated input is non-negative
            mutatedInput = 0;
        }
        fuzzedInputs[i] = std::to_string(mutatedInput); // convert the mutated input to a string and store it in the vector
    }
    return fuzzedInputs;
}

//////////////////////////////////////////////////////////////
// helper functions: parse options & manage files
//////////////////////////////////////////////////////////////

// parse the input options given in the command line
std::map<std::string, std::string> parse_input_options() {
    boost::program_options::options_description desc("Allowed options");
    std::map<std::string, std::string> input_options;

    desc.add_options()
        ("help", "produce help message")
        ("input", boost::program_options::value<std::string>(), "input file")
        ("output", boost::program_options::value<std::string>(), "output file")
    ;

    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
    boost::program_options::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return;
    }

    if (vm.count("input")) {
        std::string input_file = vm["input"].as<std::string>();
        input_options["input_file"] = input_file;
        std::cout << "Input file: " << input_file << std::endl;
    }

    if (vm.count("output")) {
        std::string output_file = vm["output"].as<std::string>();
		input_options["output_file"] = output_file;
        std::cout << "Output file: " << output_file << std::endl;
    }

	return input_options;
}

// find .cpp files in the given directory
std::vector<std::string> find_files(std::string directory_path) {
	std::vector<std::string> cpp_files;

	// loop over all files in the directory
    for (const auto& entry : fs::directory_iterator(directory_path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".cpp") {
            // append the file name to the vector
            cpp_files.push_back(entry.path().filename().string());
        }
    }

	return cpp_files;
}

// compile the given file
void compile_file(const std::string& filename) {
    std::string command = "g++ -std=c++11 -Wall -o " + filename + ".out " + filename + ".cpp " + INCLUDE_PATH + LIBRARY_PATH + LIBRARY_FILE;
    std::system(command.c_str());
}