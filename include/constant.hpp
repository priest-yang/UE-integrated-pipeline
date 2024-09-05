#ifndef CONSTANT_HPP
#define CONSTANT_HPP

// Constants
#include <cmath>  // Include this for math constants and functions

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




#endif // CONSTANT_HPP