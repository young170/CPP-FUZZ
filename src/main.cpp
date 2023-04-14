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
    std::map<std::string, std::string> input_options = parse_input_options(argc, argv);

	// get the directory path of the to-be-compiled files, input_options.directory
    std::string directory_path = input_options.at("directory");
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
        program_exe(exe_file, input_options.at("lower"), input_options.at("upper"), input_options.at("range"));
    }

    return 0;
}

//////////////////////////////////////////////////////////////
// main functions: execute program & generate fuzzed inputs
//////////////////////////////////////////////////////////////

// execute the target program using fuzzed inputs
// communicate to decide if it is semi-passed, via pipe
void program_exe(std::string exe_file, std::string lower_bound, std::string upper_bound, std::string range) {
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
		std::vector<std::string> fuzz_inputs = generate_fuzz_inputs(std::stoi(lower_bound), std::stoi(upper_bound), std::stoi(range));
        std::vector<char*> fuzz_inputs_placeholder;

		// allocate memory for each string and copy its contents to a char array
		for (const auto& input : fuzz_inputs) {
			char* str = new char[input.length() + 1];
			std::strcpy(str, input.c_str());

			fuzz_inputs_placeholder.push_back(str);
		}

		// convert the vector of char* pointers to a char** array
		char** fuzz_arguments = fuzz_inputs_placeholder.data();

		// child process executes binary file
		// the *argv[] for the child process is generated from the fuzz_options() function
		if (execv(exe_file.c_str(), fuzz_arguments) == -1) {
			fprintf(stderr, "child process failed..\n");
			exit(1);
		}
	}

	// // parent process 
	// close(to_child_FD[0]);	// close read
	// close(to_parent_FD[1]);	// close write

	// // write to pipe_to_child - stdin
	// // write(to_child_FD[1], input, strlen(input));

	// // need to close write pipe - or else it constantly waits for input
	// close(to_child_FD[1]);

	// waitpid(child_pid, &status, 0);

	// char* buf = (char*) malloc(sizeof(char) * BUFSIZ);
	// // recieve the output of child via stderr using unnamed pipe
	// int num_of_bytes = read(to_parent_FD[0], buf, BUFSIZ);
	// buf[num_of_bytes] = '\0';
	// // printf("%s\n", buf);

	// close(to_parent_FD[0]);
}

// generate fuzz inputs based on the range given to the paraments
std::vector<std::string> generate_fuzz_inputs(int lower_bound, int upper_bound, int range) {
	std::vector<std::string> fuzzed_inputs;

    std::random_device rd;
    std::mt19937 gen(rd());
	// change the range of mutations
    std::uniform_int_distribution<> dis(-100, 100);

    for (int i = 0; i < range; i++) {
        // add the integer value as a string to the vector
        fuzzed_inputs.push_back(std::to_string(i));

        // mutate the integer value by adding or subtracting a random number
        int mutation = dis(gen);
        int mutated = std::clamp(i + mutation, lower_bound, upper_bound); // Clamp mutated value to range
        fuzzed_inputs.push_back(std::to_string(mutated));
    }

    return fuzzed_inputs;
}

//////////////////////////////////////////////////////////////
// helper functions: parse options & manage files
//////////////////////////////////////////////////////////////

// parse the input options given in the command line
std::map<std::string, std::string> parse_input_options(int argc, char* argv[]) {
    std::map<std::string, std::string> input_options;
    int opt;
    std::string arg1, arg2, arg3, lower_bound, upper_bound, range;

    while ((opt = getopt(argc, argv, "i:o:d:lb:ub:r:")) != -1) {
        switch (opt) {
            case 'i': {
                arg1 = arg1.assign(optarg);
                input_options.insert( std::pair<std::string, std::string>(arg1, "input") );
                break;
            }
            case 'o': {
                arg2 = arg2.assign(optarg);
                input_options.insert( std::pair<std::string, std::string>(arg2, "output") );
                break;
            }
            case 'd': {
                arg3 = arg3.assign(optarg);
                input_options.insert( std::pair<std::string, std::string>(arg3, "directory") );
                break;
            }
			case 'lb': {
                lower_bound = lower_bound.assign(optarg);
                input_options.insert( std::pair<std::string, std::string>(lower_bound, "lower") );
                break;
            }
			case 'ub': {
                upper_bound = upper_bound.assign(optarg);
                input_options.insert( std::pair<std::string, std::string>(upper_bound, "upper") );
                break;
            }
			case 'r': {
                range = range.assign(optarg);
                input_options.insert( std::pair<std::string, std::string>(range, "range") );
                break;
            }
            default:
                std::cerr << "Usage: " << argv[0] << " -i <arg1> -o <arg2> -d <arg3> -lb <lower_bound> -ub <upper_bound> -r <range>" << std::endl;
        }
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