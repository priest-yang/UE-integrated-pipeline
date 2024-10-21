#ifndef LOG_READER_HPP
#define LOG_READER_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <deque>
#include <filesystem>
#include <sstream>
#include <chrono>
#include <thread>
#include <mutex>

namespace fs = std::filesystem;

class LogReader {
private:
    std::string log_dir;
    std::deque<std::vector<float>> logs_deque;  // To store the most recent 30 logs
    std::mutex data_mutex;  // Mutex to protect shared data access
    bool new_data_flag;     // Flag to indicate if the newest data has been used
    std::thread reader_thread;
    bool stop_thread;
    int input_size;

    // Internal method to read a single CSV log file
    std::vector<float> read_log_file(const std::string& filename) {
        std::vector<float> data;
        std::ifstream file(filename);
        std::string line;

        // Skip the header line
        std::getline(file, line);

        // Read the data line
        if (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string value;
            while (std::getline(ss, value, ',')) {
                data.push_back(std::stof(value));  // Convert string to float and add to vector
            }
        }

        return data;
    }

    // Thread method to monitor logs
    void monitor_logs() {
        while (!stop_thread) {
            std::vector<fs::path> log_files;

            // Get all CSV files in the log directory
            for (const auto& entry : fs::directory_iterator(log_dir)) {
                if (entry.path().extension() == ".csv") {
                    log_files.push_back(entry.path());
                }
            }

            // Sort log files by modification time (most recent first)
            std::sort(log_files.begin(), log_files.end(), [](const fs::path& a, const fs::path& b) {
                return fs::last_write_time(a) > fs::last_write_time(b);
            });

            // Lock the mutex before modifying shared data
            std::lock_guard<std::mutex> lock(data_mutex);

            // Clear the deque and track only the most recent 30 logs
            logs_deque.clear();
            for (size_t i = 0; i < std::min(log_files.size(), size_t(input_size)); ++i) {
                std::vector<float> log_data = read_log_file(log_files[i].string());
                logs_deque.push_back(log_data);
            }

            // Set the new_data_flag to true indicating fresh data is available and length of logs_deque is not zero
            if (logs_deque.size() == input_size) {
                new_data_flag = true;
            }
            // Sleep for a while before checking for new logs
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }

public:
    // Constructor
    LogReader(const std::string& directory) : log_dir(directory), new_data_flag(false), stop_thread(false), input_size(30) {
        // Start the thread for monitoring logs
        reader_thread = std::thread(&LogReader::monitor_logs, this);
    }

    // Destructor to ensure thread is stopped properly
    ~LogReader() {
        stop_thread = true;
        if (reader_thread.joinable()) {
            reader_thread.join();
        }
    }

    // Method to get the newest data and mark it as used
    std::deque<std::vector<float>> _newest_data() {
        std::lock_guard<std::mutex> lock(data_mutex);
        new_data_flag = false;  // Reset the flag once data is retrieved
        return logs_deque;
    }

    // Method to check if new data is available
    bool has_new_data() {
        std::lock_guard<std::mutex> lock(data_mutex);
        return new_data_flag;
    }
};

#endif

// int main() {
//     // Specify the log directory
//     std::string log_directory = "logs";

//     // Create a LogReader object
//     LogReader log_reader(log_directory);

//     // Main loop to check for new data
//     while (true) {
//         // Check if new data is available
//         if (log_reader.has_new_data()) {
//             // Retrieve the newest data
//             std::deque<std::vector<float>> new_data = log_reader._newest_data();

//             // Process the new data (for demonstration, we'll just print the size)
//             std::cout << "New data received! Number of logs: " << new_data.size() << std::endl;

//             // Print the first log for demonstration
//             if (!new_data.empty()) {
//                 std::cout << "First log data: ";
//                 for (float value : new_data.front()) {
//                     std::cout << value << " ";
//                 }
//                 std::cout << std::endl;
//             }
//         }

//         // Simulate doing something else
//         std::this_thread::sleep_for(std::chrono::seconds(1));
//     }

//     return 0;
// }