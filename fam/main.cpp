#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <vector>
// #include "FiniteAutomationMachine.hpp"  // Include your state machine header file

#include <sstream>


#include <string>
#include <unordered_map>
#include <functional>
struct Features {
    double AGV_distance_X;
    double AGV_distance_Y;
    double AGV_speed_X;
    double AGV_speed_Y;
    double AGV_speed;
    double User_speed_X;
    double User_speed_Y;
    double User_speed;
    double User_velocity_X;
    double User_velocity_Y;
    double Wait_time;
    bool intent_to_cross;
    int Gazing_station;
    bool possible_interaction;
    bool facing_along_sidewalk;
    bool facing_to_road;
    bool On_sidewalks;
    bool On_road;
    int closest_station;
    double distance_to_closest_station;
    double distance_to_closest_station_X;
    double distance_to_closest_station_Y;
    bool looking_at_AGV;
    double start_station_X;
    double start_station_Y;
    double end_station_X;
    double end_station_Y;
    double distance_from_start_station_X;
    double distance_from_start_station_Y;
    double distance_from_end_station_X;
    double distance_from_end_station_Y;
    bool facing_start_station;
    bool facing_end_station;
    bool looking_at_closest_station;
    double GazeDirection_X;
    double GazeDirection_Y;
    double AGV_X;
    double AGV_Y;
    double User_X;
    double User_Y;
    int TimestampID;

    std::unordered_map<std::string, std::function<void(const std::string&)>> setters;

    Features() {
        setters["AGV_distance_X"] = [this](const std::string& val) { AGV_distance_X = std::stod(val); };
        // Initialize other setters similarly...
        setters["looking_at_AGV"] = [this](const std::string& val) { looking_at_AGV = val == "True"; };
    }

    void setField(const std::string& name, const std::string& value) {
        if (setters.find(name) != setters.end()) {
            std::cout << "name: " << name << " value: " << value << std::endl;
            setters[name](value);
        }
    }
};

//     // Maps field names to setter functions
//     std::unordered_map<std::string, std::function<void(const std::string&)>> setters;

//     Features() {
//         setters["User_speed_X"] = [this](const std::string& val) { User_speed_X = std::stod(val); };
//         setters["User_speed_Y"] = [this](const std::string& val) { User_speed_Y = std::stod(val); };
//         setters["User_speed"] = [this](const std::string& val) { User_speed = std::stod(val); };
//         setters["User_velocity_X"] = [this](const std::string& val) { User_velocity_X = std::stod(val); };
//         setters["User_velocity_Y"] = [this](const std::string& val) { User_velocity_Y = std::stod(val); };
//         setters["Wait_time"] = [this](const std::string& val) { Wait_time = std::stod(val); };
//         setters["intent_to_cross"] = [this](const std::string& val) { intent_to_cross = val == "true"; };
//         setters["Gazing_station"] = [this](const std::string& val) { Gazing_station = std::stoi(val); };
//         setters["possible_interaction"] = [this](const std::string& val) { possible_interaction = val == "true"; };
//         setters["facing_along_sidewalk"] = [this](const std::string& val) { facing_along_sidewalk = val == "true"; };
//         setters["facing_to_road"] = [this](const std::string& val) { facing_to_road = val == "true"; };
//         setters["On_sidewalks"] = [this](const std::string& val) { On_sidewalks = val == "true"; };
//         setters["On_road"] = [this](const std::string& val) { On_road = val == "true"; };
//         setters["closest_station"] = [this](const std::string& val) { closest_station = std::stoi(val); };
//         setters["distance_to_closest_station"] = [this](const std::string& val) { distance_to_closest_station = std::stod(val); };
//         setters["distance_to_closest_station_X"] = [this](const std::string& val) { distance_to_closest_station_X = std::stod(val); };
//         setters["distance_to_closest_station_Y"] = [this](const std::string& val) { distance_to_closest_station_Y = std::stod(val); };
//         setters["looking_at_AGV"] = [this](const std::string& val) { looking_at_AGV = val == "true"; };
//         setters["start_station_X"] = [this](const std::string& val) { start_station_X = std::stod(val); };
//         setters["start_station_Y"] = [this](const std::string& val) { start_station_Y = std::stod(val); };
//         setters["end_station_X"] = [this](const std::string& val) { end_station_X = std::stod(val); };
//         setters["end_station_Y"] = [this](const std::string& val) { end_station_Y = std::stod(val); };
//         setters["distance_from_start_station_X"] = [this](const std::string& val) { distance_from_start_station_X = std::stod(val); };
//         setters["distance_from_start_station_Y"] = [this](const std::string& val) { distance_from_start_station_Y = std::stod(val); };
//         setters["distance_from_end_station_X"] = [this](const std::string& val) { distance_from_end_station_X = std::stod(val); };
//         setters["distance_from_end_station_Y"] = [this](const std::string& val) { distance_from_end_station_Y = std::stod(val); };
//         setters["facing_start_station"] = [this](const std::string& val) { facing_start_station = val == "true"; };
//         setters["facing_end_station"] = [this](const std::string& val) { facing_end_station = val == "true"; };
//         setters["looking_at_closest_station"] = [this](const std::string& val) { looking_at_closest_station = val == "true"; };
//         setters["GazeDirection_X"] = [this](const std::string& val) { GazeDirection_X = std::stod(val); };
//         setters["GazeDirection_Y"] = [this](const std::string& val) { GazeDirection_Y = std::stod(val); };
//         setters["AGV_X"] = [this](const std::string& val) { AGV_X = std::stod(val); };
//         setters["AGV_Y"] = [this](const std::string& val) { AGV_Y = std::stod(val); };
//         setters["User_X"] = [this](const std::string& val) { User_X = std::stod(val); };
//         setters["User_Y"] = [this](const std::string& val) { User_Y = std::stod(val); };
//         setters["TimestampID"] = [this](const std::string& val) { TimestampID = std::stoi(val); };
//     }

//     void setField(const std::string& name, const std::string& value) {
//         if (setters.find(name) != setters.end()) {
//             setters[name](value);
//         }
//     }
// };





std::vector<Features> parseCSV(const std::string& filePath) {
    std::ifstream file(filePath);
    std::string line;
    std::vector<std::string> headers;
    std::vector<Features> records;

    if (!std::getline(file, line))
        return records;  // Return empty if no header

    std::istringstream headerStream(line);
    std::string column;

    while (std::getline(headerStream, column, ',')) {
        headers.push_back(column);
    }

    while (std::getline(file, line)) {
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

        records.push_back(features);
    }

    return records;
}


int main() {
    std::string filePath = "/home/shaoze/Documents/Boeing/UE-integrated-pipeline/data/PandasData/Sampled_state/PID001_NSL.csv";
    auto featuresList = parseCSV(filePath);

    for (const auto& features : featuresList) {
        // Use the features for processing
        std::cout << "User_speed_X: " << features.User_speed_X << std::endl;
        // Print other fields as needed
    }

    return 0;
}
