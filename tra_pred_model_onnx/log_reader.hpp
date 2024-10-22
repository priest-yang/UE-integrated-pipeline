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
    std::deque<std::vector<float>> logs_deque;  // To store the most recent logs
    std::deque<std::string> file_buffer;        // Buffer for tracking log file names
    std::mutex data_mutex;  // Mutex to protect shared data access
    bool new_data_flag;     // Flag to indicate if the newest data has been used
    std::thread reader_thread;
    bool stop_thread;
    int input_size;         // Maximum number of logs to keep in the deque
    int newest_log_index;   // Keep track of the index of the newest log

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

    // Check if the next log file exists (e.g., log_1.csv, log_2.csv, etc.)
    bool is_new_log_available() {
        std::string new_log_file = log_dir + "/log_" + std::to_string(newest_log_index) + ".csv";
        return fs::exists(new_log_file);
    }

    // Thread method to monitor logs
    void monitor_logs() {
        while (!stop_thread) {
            // Check if a new log file is available
            if (is_new_log_available()) {
                // Lock the mutex before modifying shared data
                std::lock_guard<std::mutex> lock(data_mutex);

                // Construct the file name for the new log
                std::string new_log_file = log_dir + "/log_" + std::to_string(newest_log_index) + ".csv";

                // Read the new log file and add to the deque
                std::vector<float> log_data = read_log_file(new_log_file);
                logs_deque.push_back(log_data);
                file_buffer.push_back(new_log_file);  // Track the log file in the buffer

                // Increment the newest log index
                newest_log_index++;

                // Maintain fixed buffer size by popping the oldest log when necessary
                if (logs_deque.size() > input_size) {
                    logs_deque.pop_front();
                    file_buffer.pop_front();  // Also remove from file buffer
                }

                // Set the new data flag
                if (logs_deque.size() == input_size) {
                    new_data_flag = true;
                }
            }

            // Sleep for a while before checking again
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

public:
    // Constructor
    LogReader(const std::string& directory) : log_dir(directory), new_data_flag(false), stop_thread(false), input_size(30), newest_log_index(0) {
        // Check for existing logs and update newest_log_index
        // for (int i = 0; ; ++i) {
        //     std::string log_file = log_dir + "/log_" + std::to_string(i) + ".csv";
        //     if (fs::exists(log_file)) {
        //         newest_log_index = i + 1;  // Set next expected log index
        //     } else {
        //         break;
        //     }
        // }
        newest_log_index = 0; 

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