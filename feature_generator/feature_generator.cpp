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
#include <constant.hpp>

using namespace std;

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

// fixed: Does this function compute the closest station or the station that matches the gaze direction most closely?
std::tuple<double, int, double, double> get_most_close_station_direction(const Row& row) {
    double max_cos = -1;
    int most_common_station = -1;
    double closest_station_X;
    double closest_station_Y;
    for (const auto& [station, position] : stations) {
        // Get normalized direction vector
        std::pair<double, double> direction_normalized = get_direction_normalized({row.User_X, row.User_Y}, position);

        // Calculate cosine of the angle between gaze direction and station direction
        double cosine_gaze_direction = row.GazeDirection_X * direction_normalized.first + row.GazeDirection_Y * direction_normalized.second;

        
        // Update max cosine and station if current cosine is greater
        if (cosine_gaze_direction > max_cos) {
            max_cos = cosine_gaze_direction;
            most_common_station = station;
            closest_station_X = direction_normalized.first;
            closest_station_Y = direction_normalized.second;
        }
    }

    return { max_cos, most_common_station, closest_station_X, closest_station_Y };
}

double get_user_agv_direction_cos(const Row& row) {
    tuple<double, double> direction_normalized = get_direction_normalized(
        make_tuple(row.User_X, row.User_Y), make_tuple(row.AGV_X, row.AGV_Y));
    return row.GazeDirection_X * get<0>(direction_normalized) +
           row.GazeDirection_Y * get<1>(direction_normalized);
}

bool intent_to_cross_helper(const Features& row) {
    const double THRESHOLD_ANGLE = 30;
    const double THRESHOLD_COS = std::cos(THRESHOLD_ANGLE * M_PI / 180);  // Convert angle to radians

    bool facing_to_road = true;

    // Check for moving down and above threshold
    if (row.User_velocity_Y < 0 && row.User_Y > 6295) {
        // If moving down, should be looking down
        facing_to_road = -row.GazeDirection_Y > THRESHOLD_COS;
    } else if (row.User_velocity_Y < -WALK_STAY_THRESHOLD && row.User_Y < 6295) {
        facing_to_road = false;
    }

    // Check for moving up and below threshold
    if (row.User_velocity_Y > 0 && row.User_Y < 8150) {
        // If moving up, should be looking up
        facing_to_road = row.GazeDirection_Y > THRESHOLD_COS;
    } else if (row.User_velocity_Y > WALK_STAY_THRESHOLD && row.User_Y > 8150) {
        facing_to_road = false;
    }

    // Determine if the user intends to cross the road
    if ((row.gazing_station_direction_cos > THRESHOLD_COS && 
         std::abs(row.User_Y - std::get<1>(stations[row.Gazing_station])) > 300) ||
        (row.user_agv_direction_cos > THRESHOLD_COS) && facing_to_road) {
        return true;
    } else {
        return false;
    }
}

// Helper function to compute the distance between two points (x1, y1) and (x2, y2)
double compute_distance(double x1, double y1, double x2, double y2) {
    return std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

// Possible interaction function to check for collision
bool possible_interaction_helper(const Features& row, double COLLISION_THRESHOLD) {
    // Relative velocity between the AGV and the user
    double relative_velocity_X = row.User_speed_X - row.AGV_speed_X;
    double relative_velocity_Y = row.User_speed_Y - row.AGV_speed_Y;

    // Initial distance between the AGV and the user
    double initial_distance = compute_distance(row.User_X, row.User_Y, row.AGV_X, row.AGV_Y);

    // If initial distance is less than the threshold, assume possible interaction
    if (initial_distance < COLLISION_THRESHOLD) {
        return true;
    }

    // Time to collision (assuming constant velocity model)
    double relative_speed_squared = relative_velocity_X * relative_velocity_X + relative_velocity_Y * relative_velocity_Y;

    // If relative speed is zero, no collision can happen (they are moving parallel or stationary)
    if (relative_speed_squared == 0) {
        return false;
    }

    // Projected future positions: Compute the time when they would collide
    double time_to_collision = -((row.User_X - row.AGV_X) * relative_velocity_X + (row.User_Y - row.AGV_Y) * relative_velocity_Y) / relative_speed_squared;

    // If the collision time is positive and the objects are projected to be within the threshold distance at that time
    if (time_to_collision > 0) {
        double future_user_X = row.User_X + row.User_speed_X * time_to_collision;
        double future_user_Y = row.User_Y + row.User_speed_Y * time_to_collision;
        double future_agv_X = row.AGV_X + row.AGV_speed_X * time_to_collision;
        double future_agv_Y = row.AGV_Y + row.AGV_speed_Y * time_to_collision;

        double future_distance = compute_distance(future_user_X, future_user_Y, future_agv_X, future_agv_Y);

        if (future_distance < COLLISION_THRESHOLD) {
            return true;
        }
    }

    return false;  // No collision is expected
}

bool facing_road_helper(const Features& row) {
    // If moving down and Y is greater than 6295
    if (row.User_velocity_Y < 0 && row.User_Y > 6295) {
        // If moving down, check if gaze is also down
        return -row.GazeDirection_Y > GAZING_ANGLE_THRESHOLD_COS;
    } 
    // If moving down below the threshold and Y is less than 6295
    else if (row.User_velocity_Y < -WALK_STAY_THRESHOLD && row.User_Y < 6295) {
        return false;
    }

    // If moving up and Y is less than 8150
    if (row.User_velocity_Y > 0 && row.User_Y < 8150) {
        // If moving up, check if gaze is also up
        return row.GazeDirection_Y > GAZING_ANGLE_THRESHOLD_COS;
    } 
    // If moving up above the threshold and Y is greater than 8150
    else if (row.User_velocity_Y > WALK_STAY_THRESHOLD && row.User_Y > 8150) {
        return false;
    }

    // Assume they are facing the road if they are stationary
    return true;
}


// Function to calculate the distance to the closest station
std::tuple<int, double, double, double> generate_distance_to_closest_station_helper(const Row& row) {
    double mindis = std::numeric_limits<double>::max();
    int closest_station = -1;
    double mindis_X = 0.0, mindis_Y = 0.0;

    // Iterate over stations to find the closest one
    for (const auto& station : stations) {
        double station_X = station.second.first;
        double station_Y = station.second.second;

        // Calculate the distance between the user and the station
        double dis = std::sqrt((row.User_X - station_X) * (row.User_X - station_X) +
                               (row.User_Y - station_Y) * (row.User_Y - station_Y));

        // Update minimum distance and closest station if a closer one is found
        if (dis < mindis) {
            mindis = dis;
            closest_station = station.first;
            mindis_X = std::abs(row.User_X - station_X);
            mindis_Y = std::abs(row.User_Y - station_Y);
        }
    }

    // Return the closest station, minimum distance, and X, Y distance components
    return std::make_tuple(closest_station, mindis, mindis_X, mindis_Y);
}

Features extract_features(const deque<Row>& rows, size_t index) {
    const Row& row = rows[index];
    Features features;

    // Copy raw features
    features.GazeDirection_X = row.GazeDirection_X;
    features.GazeDirection_Y = row.GazeDirection_Y;
    features.AGV_X = row.AGV_X;
    features.AGV_Y = row.AGV_Y;
    features.User_X = row.User_X;
    features.User_Y = row.User_Y;
    features.TimestampID = row.TimestampID;

    // Calculate distances
    features.AGV_distance_X = abs(row.User_X - row.AGV_X);
    features.AGV_distance_Y = abs(row.User_Y - row.AGV_Y);

    // Normalized gaze direction
    double gaze_direction_length = sqrt(row.GazeDirection_X * row.GazeDirection_X + row.GazeDirection_Y * row.GazeDirection_Y);
    features.GazeDirection_X /= gaze_direction_length;
    features.GazeDirection_Y /= gaze_direction_length;

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
    features.user_agv_direction_cos = get_user_agv_direction_cos(row);

    //fixed: Can we use the WALK_STAY_THRESHOLD here instead of the 0.1?
    // Most close station and intent to cross
    //fixed: Is this the station that is closest to the user's gaze direction?
    auto close_station_res = get_most_close_station_direction(row);
    auto station_direction = std::make_pair(std::get<0>(close_station_res), std::get<1>(close_station_res));
    features.gazing_station_direction_cos = std::get<0>(close_station_res);
    features.Gazing_station = station_direction.second;
    features.closest_station_dir_X = std::get<2>(close_station_res);
    features.closest_station_dir_Y = std::get<3>(close_station_res);

    //fixed: Include another constant in constant.hpp for this instead of using a random float here...
    features.intent_to_cross = intent_to_cross_helper(features);

    // Possible interaction (as an example)
    //fixed: Include another constant in constant.hpp for this instead of using a random float here...
    //fixed: Not sure how this corresponds to possible interaction.
    //fixed: Please explain this feature
    features.possible_interaction = possible_interaction_helper(features, COLLISION_THRESHOLD);

    // Example features (need more context to compute correctly)
    // fixed: These features have not been computed. Are we not using them anymore?
    features.facing_along_sidewalk = features.GazeDirection_X > GAZING_ANGLE_THRESHOLD_COS;
    features.facing_to_road = facing_road_helper(features);
    // features.On_sidewalks = false; updated below
    // features.On_road = false; updated below

    // fixed: Is the gazing station always the closest station?
    auto closest_station_res = generate_distance_to_closest_station_helper(row);
    features.closest_station = std::get<0>(closest_station_res);
    features.distance_to_closest_station = std::get<1>(closest_station_res);
    features.distance_to_closest_station_X = std::get<2>(closest_station_res);
    features.distance_to_closest_station_Y = std::get<3>(closest_station_res);

    //TODO: Include another constant in constant.hpp for this instead of using a random float here...
    features.looking_at_AGV = features.user_agv_direction_cos > GAZING_ANGLE_THRESHOLD_COS;

    // TODO: This statement also feels dubious
    features.looking_at_closest_station = features.gazing_station_direction_cos > GAZING_ANGLE_THRESHOLD_COS;

    return features;
}


vector<Features> generate_wait_time(vector<Features>& rows, double H1 = 0.2, double H2 = 0.1, double THRESHOLD_ANGLE = 30, double frame_rate = 30) {
    
    // Constants and data
    const double ERROR_RANGE = 50;  // Replace with actual error range
    double threshold_COSINE = cos(THRESHOLD_ANGLE * M_PI / 180);  // Convert angle to radians and find cosine

    bool begin_wait_Flag = false;
    bool AGV_passed_Flag = false;
    size_t begin_wait_Timestamp = 0;

    for (size_t index = 0; index < rows.size(); ++index) {
        Features& row = rows[index];  // Get the current row

        // If AGV already passed, skip this row
        if (AGV_passed_Flag) {
            continue;
        }

        // Check if the user is on the sidewalk
        bool on_sidewalk = (row.User_Y > 8150 - ERROR_RANGE && row.User_Y < 8400 + ERROR_RANGE) || 
                           (row.User_Y > 6045 - ERROR_RANGE && row.User_Y < 6295 + ERROR_RANGE);

        // Check if the user is on the road
        bool on_road = (row.User_Y < 8150 - ERROR_RANGE / 2) && (row.User_Y > 6295 + ERROR_RANGE / 2);

        rows[index].On_sidewalks = on_sidewalk;
        rows[index].On_road = on_road;
        // Check if user is looking at AGV using angle and cosine threshold
        // tuple<double, double> target_station_pos = stations[User_trajectory[stoi(row.AGV_name.substr(3))][1]];
        // tuple<double, double> user_target_station_dir = get_direction_normalized(make_tuple(row.User_X, row.User_Y), target_station_pos);

        tuple<double, double> user_agv_dir = get_direction_normalized(make_tuple(row.User_X, row.User_Y), make_tuple(row.AGV_X, row.AGV_Y));

        double user_target_station_angle = get_angle_between_normalized_vectors(make_tuple(row.GazeDirection_X, row.GazeDirection_Y), user_agv_dir);
        double user_agv_angle = get_angle_between_normalized_vectors(make_tuple(row.GazeDirection_X, row.GazeDirection_Y), make_tuple(row.closest_station_dir_X, row.closest_station_dir_Y));

        bool looking_at_AGV = (user_target_station_angle > threshold_COSINE || user_agv_angle > threshold_COSINE);

        // Check if user is in a waiting state
        bool wait_state = (sqrt(row.User_speed_X * row.User_speed_X + row.User_speed_Y * row.User_speed_Y) < H1);

        // Begin waiting state
        if (!begin_wait_Flag) {  // User is walking
            if (wait_state && on_sidewalk && !looking_at_AGV) {
                begin_wait_Flag = true;
                begin_wait_Timestamp = (index > 1) ? index - 1 : 1;  // Use the previous index or first
                row.Wait_time = (index - begin_wait_Timestamp) / frame_rate;
            } else {
                continue;
            }
        } else {  // User is in a waiting state
            if (sqrt(row.User_speed_X * row.User_speed_X + row.User_speed_Y * row.User_speed_Y) <= H2) {  // Still waiting
                row.Wait_time = (index - begin_wait_Timestamp) / frame_rate;
            } else {  // End waiting state
                begin_wait_Flag = false;
                begin_wait_Timestamp = 0;
                row.Wait_time = 0;
                AGV_passed_Flag = true;  // AGV has passed
            }
        }
    }

    // Assign the extracted features to the result (convert deque to vector or return features in some form)
    return rows;
}



vector<Features> process_rows(const deque<Row>& rows) {
    vector<Features> features_list;
    for (size_t i = 0; i < rows.size(); ++i) {
        features_list.push_back(extract_features(rows, i));
    }
    // for wait time
    features_list = generate_wait_time(features_list);
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