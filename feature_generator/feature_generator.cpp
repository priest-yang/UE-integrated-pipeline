#include <cmath>
#include <map>
#include <tuple>
#include <vector>
#include <deque>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <config.hpp>
#include <chrono>
#include <numeric> 
#include <argparse.hpp>

using namespace std;

const map<int, pair<double, double>> stations = {
    {1, {1580, 8683}},
    {2, {1605, 5800}},
    {3, {5812, 8683}},
    {4, {5800, 5786}},
    {5, {7632, 8683}},
    {6, {7639, 5786}},
    {7, {13252, 8683}},
    {8, {13319, 5796}}
};

tuple<double, double> get_direction_normalized(const tuple<double, double>& start, const tuple<double, double>& end) {
    double x = get<0>(end) - get<0>(start);
    double y = get<1>(end) - get<1>(start);
    double length = sqrt(x * x + y * y);
    return make_tuple(x / length, y / length);
}

double get_angle_between_normalized_vectors(const tuple<double, double>& v1, const tuple<double, double>& v2) {
    double dot_product = get<0>(v1) * get<0>(v2) + get<1>(v1) * get<1>(v2);
    return acos(dot_product);
}

// TODO: Does this function compute the closest station or the station that matches the gaze direction most closely?
pair<double, int> get_most_close_station_direction(const Row& row) {
    double max_cos = -1;
    int most_common_station = -1;

    for (const auto& station : stations) {
        tuple<double, double> direction_normalized = get_direction_normalized(
            make_tuple(row.User_X, row.User_Y), station.second);
        double cosine_gaze_direction = row.GazeDirection_X * get<0>(direction_normalized) +
                                       row.GazeDirection_Y * get<1>(direction_normalized);
        if (cosine_gaze_direction > max_cos) {
            max_cos = cosine_gaze_direction;
            most_common_station = station.first;
        }
    }
    return make_pair(max_cos, most_common_station);
}

double get_user_agv_direction_cos(const Row& row) {
    tuple<double, double> direction_normalized = get_direction_normalized(
        make_tuple(row.User_X, row.User_Y), make_tuple(row.AGV_X, row.AGV_Y));
    return row.GazeDirection_X * get<0>(direction_normalized) +
           row.GazeDirection_Y * get<1>(direction_normalized);
}

Features extract_features(const deque<Row>& rows, size_t index) {
    const Row& row = rows[index];
    Features features;

    // Calculate distances
    features.AGV_distance_X = abs(row.User_X - row.AGV_X);
    features.AGV_distance_Y = abs(row.User_Y - row.AGV_Y);

    // Calculate speeds and velocities using previous row data
    if (index > 0) {
        const Row& prev_row = rows[index - 1];
        features.AGV_speed_X = (row.AGV_X - prev_row.AGV_X) / (row.TimestampID - prev_row.TimestampID);
        features.AGV_speed_Y = (row.AGV_Y - prev_row.AGV_Y) / (row.TimestampID - prev_row.TimestampID);
        features.AGV_speed = sqrt(features.AGV_speed_X * features.AGV_speed_X + features.AGV_speed_Y * features.AGV_speed_Y);

        features.User_speed_X = (row.User_X - prev_row.User_X) / (row.TimestampID - prev_row.TimestampID);
        features.User_speed_Y = (row.User_Y - prev_row.User_Y) / (row.TimestampID - prev_row.TimestampID);
        features.User_speed = sqrt(features.User_speed_X * features.User_speed_X + features.User_speed_Y * features.User_speed_Y);

        features.User_velocity_X = features.User_speed_X;
        features.User_velocity_Y = features.User_speed_Y;
    } else {
        features.AGV_speed_X = 0.0;
        features.AGV_speed_Y = 0.0;
        features.AGV_speed = 0.0;
        features.User_speed_X = 0.0;
        features.User_speed_Y = 0.0;
        features.User_speed = 0.0;
        features.User_velocity_X = 0.0;
        features.User_velocity_Y = 0.0;
    }

    // Wait time calculation (simplified as an example) 
    //TODO: Can we use the WALK_STAY_THRESHOLD here instead of the 0.1?
    //TODO: Understand this...
    features.Wait_time = (features.User_speed < 0.1) ? (index > 0 ? rows[index - 1].TimestampID : 0) : 0;

    // Most close station and intent to cross
    //TODO: Is this the station that is closest to the user's gaze direction?
    auto station_direction = get_most_close_station_direction(row);
    features.Gazing_station = station_direction.second;

    //TODO: Include another constant in constant.hpp for this instead of using a random float here...
    features.intent_to_cross = get_user_agv_direction_cos(row) > 0.5;

    // Possible interaction (as an example)
    //TODO: Include another constant in constant.hpp for this instead of using a random float here...
    //TODO: Not sure how this corresponds to possible interaction.
    //TODO: Please explain this feature
    features.possible_interaction = station_direction.first > 0.5;

    // Example features (need more context to compute correctly)
    // TODO: These features have not been computed. Are we not using them anymore?
    features.facing_along_sidewalk = false;
    features.facing_to_road = false;
    features.On_sidewalks = false;
    features.On_road = false;

    // TODO: Is the gazing station always the closest station?
    features.closest_station = features.Gazing_station;

    // TODO: This is incorrect
    features.distance_to_closest_station = features.AGV_distance_X; // Simplified
    features.distance_to_closest_station_X = features.AGV_distance_X;
    features.distance_to_closest_station_Y = features.AGV_distance_Y;

    //TODO: Include another constant in constant.hpp for this instead of using a random float here...
    features.looking_at_AGV = get_user_agv_direction_cos(row) > 0.5;

    // Start and end station coordinates (as an example, hard-coded)
    // TODO: These also seem incorrect. Are we not using these features?
    features.start_station_X = 0.0;
    features.start_station_Y = 0.0;
    features.end_station_X = 100.0;
    features.end_station_Y = 100.0;
    features.distance_from_start_station_X = row.User_X - features.start_station_X;
    features.distance_from_start_station_Y = row.User_Y - features.start_station_Y;
    features.distance_from_end_station_X = row.User_X - features.end_station_X;
    features.distance_from_end_station_Y = row.User_Y - features.end_station_Y;
    features.facing_start_station = false;
    features.facing_end_station = false;

    // TODO: This statement also feels dubious
    features.looking_at_closest_station = features.looking_at_AGV;

    // Copy raw features
    features.GazeDirection_X = row.GazeDirection_X;
    features.GazeDirection_Y = row.GazeDirection_Y;
    features.AGV_X = row.AGV_X;
    features.AGV_Y = row.AGV_Y;
    features.User_X = row.User_X;
    features.User_Y = row.User_Y;
    features.TimestampID = row.TimestampID;

    return features;
}

vector<Features> process_rows(const deque<Row>& rows) {
    vector<Features> features_list;
    for (size_t i = 0; i < rows.size(); ++i) {
        features_list.push_back(extract_features(rows, i));
    }
    return features_list;
}


// Parse CSV line into Row struct
Row parseCSVLine(const std::string& line, const std::vector<std::string>& headers) {
    std::istringstream lineStream(line);
    std::string cell;
    Row row;
    size_t columnIndex = 0;

    while (std::getline(lineStream, cell, ',')) {
        if (columnIndex < headers.size()) {
            row.setField(headers[columnIndex], cell);
        }
        columnIndex++;
    }

    return row;
}

// Read and parse CSV file
std::vector<Row> parseCSV(const std::string& filePath) {
    std::ifstream file(filePath);
    std::string line;
    std::vector<Row> records;
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

void vis_features(vector<Features> features_list){
  // Output features for verification
    for (const auto& features : features_list) {
        cout << "AGV_distance_X: " << features.AGV_distance_X
             << ", AGV_distance_Y: " << features.AGV_distance_Y
             << ", AGV_speed_X: " << features.AGV_speed_X
             << ", AGV_speed_Y: " << features.AGV_speed_Y
             << ", AGV_speed: " << features.AGV_speed
             << ", User_speed_X: " << features.User_speed_X
             << ", User_speed_Y: " << features.User_speed_Y
             << ", User_speed: " << features.User_speed
             << ", User_velocity_X: " << features.User_velocity_X
             << ", User_velocity_Y: " << features.User_velocity_Y
             << ", Wait_time: " << features.Wait_time
             << ", intent_to_cross: " << (features.intent_to_cross ? "true" : "false")
             << ", Gazing_station: " << features.Gazing_station
             << ", possible_interaction: " << (features.possible_interaction ? "true" : "false")
             << ", facing_along_sidewalk: " << (features.facing_along_sidewalk ? "true" : "false")
             << ", facing_to_road: " << (features.facing_to_road ? "true" : "false")
             << ", On_sidewalks: " << (features.On_sidewalks ? "true" : "false")
             << ", On_road: " << (features.On_road ? "true" : "false")
             << ", closest_station: " << features.closest_station
             << ", distance_to_closest_station: " << features.distance_to_closest_station
             << ", distance_to_closest_station_X: " << features.distance_to_closest_station_X
             << ", distance_to_closest_station_Y: " << features.distance_to_closest_station_Y
             << ", looking_at_AGV: " << (features.looking_at_AGV ? "true" : "false")
             << ", start_station_X: " << features.start_station_X
             << ", start_station_Y: " << features.start_station_Y
             << ", end_station_X: " << features.end_station_X
             << ", end_station_Y: " << features.end_station_Y
             << ", distance_from_start_station_X: " << features.distance_from_start_station_X
             << ", distance_from_start_station_Y: " << features.distance_from_start_station_Y
             << ", distance_from_end_station_X: " << features.distance_from_end_station_X
             << ", distance_from_end_station_Y: " << features.distance_from_end_station_Y
             << ", facing_start_station: " << (features.facing_start_station ? "true" : "false")
             << ", facing_end_station: " << (features.facing_end_station ? "true" : "false")
             << ", looking_at_closest_station: " << (features.looking_at_closest_station ? "true" : "false")
             << ", GazeDirection_X: " << features.GazeDirection_X
             << ", GazeDirection_Y: " << features.GazeDirection_Y
             << ", AGV_X: " << features.AGV_X
             << ", AGV_Y: " << features.AGV_Y
             << ", User_X: " << features.User_X
             << ", User_Y: " << features.User_Y
             << ", TimestampID: " << features.TimestampID
             << endl;
    }
}


int main(int argc, char** argv) {
    argparse::ArgumentParser program("FAM Benchmarking Program");

    // Add arguments
    program.add_argument("-f", "--file_path")
        .help("Path to the CSV file containing feature records")
        .default_value(std::string("data/demo/raw/0.csv"));

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

    std::deque<Row> file_buffer;
    vector<Features> features_list;

    std::vector<double> time_list;
    auto records = parseCSV(file_path);
    for (const auto& features : records) {
        file_buffer.push_back(features);
        if (file_buffer.size() > buffer_max_size) {
            file_buffer.pop_front();  // Maintain a fixed-size buffer
            
            auto start_time = std::chrono::high_resolution_clock::now();
            features_list = process_rows(file_buffer);
            auto end_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end_time - start_time;
            vis_features(features_list);
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