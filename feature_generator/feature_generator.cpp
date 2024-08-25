#include <cmath>
#include <map>
#include <tuple>
#include <vector>
#include <deque>
#include <iostream>
#include <string>

using namespace std;

struct Row {
    double User_X;
    double User_Y;
    double GazeDirection_X;
    double GazeDirection_Y;
    double AGV_X;
    double AGV_Y;
    int TimestampID;
};

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
};

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
    features.Wait_time = (features.User_speed < 0.1) ? (index > 0 ? rows[index - 1].TimestampID : 0) : 0;

    // Most close station and intent to cross
    auto station_direction = get_most_close_station_direction(row);
    features.Gazing_station = station_direction.second;
    features.intent_to_cross = get_user_agv_direction_cos(row) > 0.5;

    // Possible interaction (as an example)
    features.possible_interaction = station_direction.first > 0.5;

    // Example features (need more context to compute correctly)
    features.facing_along_sidewalk = false;
    features.facing_to_road = false;
    features.On_sidewalks = false;
    features.On_road = false;
    features.closest_station = features.Gazing_station;
    features.distance_to_closest_station = features.AGV_distance_X; // Simplified
    features.distance_to_closest_station_X = features.AGV_distance_X;
    features.distance_to_closest_station_Y = features.AGV_distance_Y;
    features.looking_at_AGV = get_user_agv_direction_cos(row) > 0.5;

    // Start and end station coordinates (as an example, hard-coded)
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


int main() {
    // Example usage
    deque<Row> rows = {
        {1000, 2000, 0.5, 0.5, 1500, 2500, 1},
        {1005, 2005, 0.6, 0.4, 1510, 2510, 2},
        {1010, 2010, 0.7, 0.3, 1520, 2520, 3}
    };

    vector<Features> features_list = process_rows(rows);

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

    return 0;
}