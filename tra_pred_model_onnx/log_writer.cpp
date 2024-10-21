#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <filesystem>

// Define a structure to hold each row's data
struct UserData {
    std::string User_X;
    std::string User_Y;
};

// Function to read a CSV file into a vector of UserData
std::vector<UserData> read_csv(const std::string& filename) {
    std::vector<UserData> data;
    std::ifstream file(filename);
    std::string line;

    // Skip the header row (assuming there's a header)
    std::getline(file, line);

    // Read each line from the CSV
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string value_x, value_y;

        // Read each comma-separated value into User_X and User_Y
        std::getline(ss, value_x, ',');
        std::getline(ss, value_y, ',');

        // Add the data to the vector
        data.push_back(UserData{value_x, value_y});
    }
    return data;
}

// Function to write a log for each row into separate CSV files
void write_logs(const std::vector<UserData>& data, const std::string& output_dir, int delay_ms) {
    // Create the output directory if it doesn't exist
    std::__fs::filesystem::create_directories(output_dir);

    // Iterate over each row of data and write to a CSV file
    for (size_t i = 0; i < data.size(); ++i) {
        // Construct the log file name
        std::string filename = output_dir + "/log_" + std::to_string(i) + ".csv";
        std::ofstream log_file(filename);

        // Write the header (column names)
        log_file << "User_X,User_Y\n";

        // Write the values of the current row
        log_file << data[i].User_X << "," << data[i].User_Y << "\n";

        // Close the file
        log_file.close();

        // Delay for the specified time (in milliseconds)
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    }
}

int main() {
    // File path of the CSV
    std::string input_file = "/Users/shawn/Documents/UMSI/Boeing_Project/UE-integrated-pipeline/tra_pred_model_onnx/data/demo/feature_model/0.csv";

    // Output directory for the log files
    std::string output_dir = "./data/demo/feature_cvm/logs";

    // Delay between writing each log file (in milliseconds)
    int delay_ms = 20;  // 1 second delay

    // Read data from the CSV file
    std::vector<UserData> data = read_csv(input_file);

    // Write the logs to individual CSV files
    write_logs(data, output_dir, delay_ms);

    std::cout << "Logs have been written successfully!" << std::endl;
    return 0;
}
