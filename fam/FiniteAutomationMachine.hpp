#ifndef FINITE_STATE_MACHINE_HPP
#define FINITE_STATE_MACHINE_HPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>


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

// Features structure
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

// Base class for finite automation states
class FiniteAutomationState {
public:
    std::string name;
    Features features;
    bool constraints_satisfied = false;
    FiniteAutomationState* next_state = nullptr;

    virtual ~FiniteAutomationState() = default;
    static bool check(const Features& features);
    virtual std::pair<FiniteAutomationState*, double> transition() = 0;
    void update_features(const Features& new_features);
};

// Error state class
class ErrorState : public FiniteAutomationState {
    std::string S_prev;
    std::unordered_map<std::string, std::vector<double>> MLE;

public:
    ErrorState(const Features& features, const std::string& S_prev = "Error");
    void initializeMLE();
    static bool check(const Features& features);
    std::pair<FiniteAutomationState*, double> transition() override;
    FiniteAutomationState* getStateByName(const std::string& name, const Features& features);
};

// Other state classes
class AtStationState : public FiniteAutomationState {
public:
    AtStationState(const Features& features);
    static bool check(const Features& features);
    std::pair<FiniteAutomationState*, double> transition() override;
};

class WaitingState : public FiniteAutomationState {
public:
    WaitingState(const Features& features);
    static bool check(const Features& features);
    std::pair<FiniteAutomationState*, double> transition() override;
};

class CrossingState : public FiniteAutomationState {
public:
    CrossingState(const Features& features);
    static bool check(const Features& features);
    std::pair<FiniteAutomationState*, double> transition() override;
};

class ApproachingSidewalkState : public FiniteAutomationState {
public:
    ApproachingSidewalkState(const Features& features);
    static bool check(const Features& features);
    std::pair<FiniteAutomationState*, double> transition() override;
};

class MovingAlongSidewalkState : public FiniteAutomationState {
public:
    MovingAlongSidewalkState(const Features& features);
    static bool check(const Features& features);
    std::pair<FiniteAutomationState*, double> transition() override;
};

class ApproachingStationState : public FiniteAutomationState {
public:
    ApproachingStationState(const Features& features);
    static bool check(const Features& features);
    std::pair<FiniteAutomationState*, double> transition() override;
};

// Finite state machine controller
class FiniteAutomationMachine {
    FiniteAutomationState* current_state;
    FiniteAutomationState* next_state;

public:
    FiniteAutomationMachine(FiniteAutomationState* initial_state);
    ~FiniteAutomationMachine();
    void run(const Features& features);
};

#endif // FINITE_STATE_MACHINE_HPP