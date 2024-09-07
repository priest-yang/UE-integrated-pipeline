#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <vector>

#include "FiniteAutomationMachine.hpp"  // Include your state machine header file

#include <sstream>
#include <deque>
#include <algorithm>

#include <unordered_map>
#include <functional>
#include <numeric> 
#include <memory>
#include <argparse.hpp>


// Parse CSV line into Features struct
Features parseCSVLine(const std::string& line, const std::vector<std::string>& headers) {
    std::istringstream lineStream(line);
    std::string cell;
    Features features;
    size_t columnIndex = 0;

    while (std::getline(lineStream, cell, ',')) {
        if (columnIndex < headers.size()) {
            features.setField(headers[columnIndex], cell);
        }
        columnIndex++;
    }

    return features;
}

// Read and parse CSV file
std::vector<Features> parseCSV(const std::string& filePath) {
    std::ifstream file(filePath);
    std::string line;
    std::vector<Features> records;
    std::vector<std::string> headers;

    // Read headers
    if (std::getline(file, line)) {
        std::istringstream headerStream(line);
        std::string header;
        while (std::getline(headerStream, header, ',')) {
            headers.push_back(header);
        }
    }

    // Read data lines
    while (std::getline(file, line)) {
        records.push_back(parseCSVLine(line, headers));
    }

    return records;
}

int main(int argc, char** argv) {
    argparse::ArgumentParser program("FAM Benchmarking Program");

    // Add arguments
    program.add_argument("-f", "--file_path")
        .help("Path to the CSV file containing feature records")
        .default_value(std::string("data/demo/feature_fam/0.csv"));

    program.add_argument("-b", "--buffer_size")
        .help("Size of the buffer for processing")
        .default_value(40)
        .scan<'i', size_t>(); // Scanning as size_t; 

    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cout << err.what() << std::endl;
        std::cout << program;
        exit(0);
    }

    // Get the file path and buffer size from the arguments
    std::string file_path = program.get<std::string>("-f");
    const size_t buffer_max_size = program.get<size_t>("-b");

    std::deque<Features> file_buffer;  // Create a deque to hold the buffer

    // Initialize the FiniteAutomationMachine with default parameters

    FiniteAutomationMachine model{Features()};  // Assumes default initial state is ErrorState

    std::vector<double> time_list;

    // Parse the CSV file and load the records
    auto records = parseCSV(file_path);

    std::cout<<records.size()<<std::endl;

    // // Process each feature record
    for (const auto& features : records) {
        file_buffer.push_back(features);
        if (file_buffer.size() > buffer_max_size) {
            file_buffer.pop_front();  // Maintain a fixed-size buffer
        }

        // Process the buffer when it's full
        if (file_buffer.size() == buffer_max_size) {
            std::vector<std::string> states;
            auto start_time = std::chrono::high_resolution_clock::now();

            model.setCurrentStateByName(file_buffer[0].state);  // Set the initial state
            file_buffer.pop_front();
            for (const auto& buffered_features : file_buffer) {
                std::cout << model.getCurrentStateName() << " Ground Truth:  ";
                std::cout << buffered_features.state << std::endl;
                model.run(buffered_features);  // Run the model on each buffered item
                states.push_back(model.getCurrentStateName());  // Assuming getCurrentStateName() returns the state name
            }

            auto end_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end_time - start_time;
            time_list.push_back(elapsed.count());

            std::cout << "Time elapsed: " << elapsed.count() << " seconds\n";
            std::cout << std::endl;
        }
    }

    double total_time = std::accumulate(time_list.begin(), time_list.end(), 0.0);
    std::cout << "\n\n\n";
    std::cout << "Elapsed time: " << total_time << " seconds\n";
    std::cout << "Processed " << time_list.size() << " groups.\n";
    std::cout << "Speed: " << time_list.size() / total_time << " groups per second\n";

    return 0;
}
