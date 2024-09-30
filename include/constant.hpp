#ifndef CONSTANT_HPP
#define CONSTANT_HPP

// Constants
#include <cmath>  // Include this for math constants and functions
#include <map>
// Define the stations as a map with int keys and pairs of doubles representing coordinates
std::map<int, std::pair<double, double>> stations = {
    {1, {1580, 8683}},
    {2, {1605, 5800}},
    {3, {5812, 8683}},
    {4, {5800, 5786}},
    {5, {7632, 8683}},
    {6, {7639, 5786}},
    {7, {13252, 8683}},
    {8, {13319, 5796}}
};

// Define the sidewalks as a map with int keys and tuples representing four coordinates (x1, y1, x2, y2)
std::map<int, std::tuple<double, double, double, double>> sidewalks = {
    {1, {2625, 15000, 2625, 8150}},
    {2, {2425, 15000, 2425, 8400}},
    {3, {2625, 8150, 0, 8150}},
    {4, {2425, 8400, 0, 8400}},
    {5, {0, 6295, 16000, 6295}},
    {6, {0, 6045, 6400, 6045}},
    {7, {6400, 6045, 6400, 4000}},
    {8, {6650, 6045, 6650, 4000}},
    {9, {6650, 6045, 16000, 6045}},
    {10, {4275, 8150, 9000, 8150}},
    {11, {4525, 8400, 8750, 8400}},
    {12, {4525, 15000, 4525, 8400}},
    {13, {4275, 15000, 4275, 8150}},
    {14, {8750, 8400, 8750, 15000}},
    {15, {9000, 8150, 9000, 15000}},
    {16, {10500, 8150, 10500, 15000}},
    {17, {10750, 8400, 10750, 15000}},
    {18, {10750, 8400, 16000, 8400}},
    {19, {10500, 8150, 16000, 8150}}
};

// Constants for pedestrian motion and station proximity
constexpr double WALK_STAY_THRESHOLD = 0.3;  // Threshold between walking and waiting
constexpr double CLOSE_TO_STATION_THRESHOLD_X = 3.0;  // Proximity threshold for X axis in meters
constexpr double CLOSE_TO_STATION_THRESHOLD_Y = 2.0;  // Proximity threshold for Y axis in meters
constexpr double CLOSE_TO_STATION_THRESHOLD = 3.0;  // Close proximity to a station in meters
constexpr double MARGIN_NEAR_SIDEWALKS = 1.0;  // Margin near sidewalks in meters
constexpr double STATION_LENGTH = 5.0;  // Length of a station in meters

// Constants for angular thresholds
constexpr double GAZING_ANGLE_THRESHOLD = 40;  // Gazing angle threshold in degrees
constexpr double GAZING_ANGLE_THRESHOLD_RADIUS = M_PI * GAZING_ANGLE_THRESHOLD / 180.0;  // Convert degrees to radians
const double GAZING_ANGLE_THRESHOLD_COS = std::cos(GAZING_ANGLE_THRESHOLD_RADIUS);  // Cosine of the gazing angle threshold

// Speed thresholds for determining motion state
constexpr double SPEED_THRESHOLD_LOW = 0.2;  // m/s, below this speed, considered stopped
constexpr double SPEED_THRESHOLD_HIGH = 0.8;  // m/s, above this speed, considered actively moving

// Angular thresholds for determining the direction of gaze relative to the user's current heading
constexpr double ANGULAR_THRESHOLD_LOW = M_PI / 3;  // radians, lower angular threshold
constexpr double ANGULAR_THRESHOLD_HIGH = 2 * M_PI / 3;  // radians, higher angular threshold

// Radii for spatial thresholds around stations and sidewalks
constexpr double RADIUS_1 = 2.5;  // meters, radius to determine at a station
constexpr double RADIUS_2 = 3.5;  // meters, radius for approaching a station or sidewalk
constexpr double RADIUS_3 = 6.0;  // meters, radius for moving along the sidewalk

constexpr double COLLISION_THRESHOLD = 0.5;  // meters, threshold for collision detection

#endif // CONSTANT_HPP