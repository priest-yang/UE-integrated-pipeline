#ifndef FINITE_STATE_MACHINE_HPP
#define FINITE_STATE_MACHINE_HPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <memory>
#include <stdexcept> 
#include <sstream>
#include <functional>

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
    std::string state;

//     // Maps field names to setter functions
    std::unordered_map<std::string, std::function<void(const std::string&)>> setters;

    Features() {
        setters["User_speed_X"] = [this](const std::string& val) { User_speed_X = std::stod(val); };
        setters["User_speed_Y"] = [this](const std::string& val) { User_speed_Y = std::stod(val); };
        setters["User_speed"] = [this](const std::string& val) { User_speed = std::stod(val); };
        setters["User_velocity_X"] = [this](const std::string& val) { User_velocity_X = std::stod(val); };
        setters["User_velocity_Y"] = [this](const std::string& val) { User_velocity_Y = std::stod(val); };
        setters["Wait_time"] = [this](const std::string& val) { Wait_time = std::stod(val); };
        setters["intent_to_cross"] = [this](const std::string& val) { intent_to_cross = val == "True"; };
        setters["Gazing_station"] = [this](const std::string& val) { Gazing_station = std::stoi(val); };
        setters["possible_interaction"] = [this](const std::string& val) { possible_interaction = val == "True"; };
        setters["facing_along_sidewalk"] = [this](const std::string& val) { facing_along_sidewalk = val == "True"; };
        setters["facing_to_road"] = [this](const std::string& val) { facing_to_road = val == "True"; };
        setters["On_sidewalks"] = [this](const std::string& val) { On_sidewalks = val == "True"; };
        setters["On_road"] = [this](const std::string& val) { On_road = val == "True"; };
        setters["closest_station"] = [this](const std::string& val) { closest_station = std::stoi(val); };
        setters["distance_to_closest_station"] = [this](const std::string& val) { distance_to_closest_station = std::stod(val); };
        setters["distance_to_closest_station_X"] = [this](const std::string& val) { distance_to_closest_station_X = std::stod(val); };
        setters["distance_to_closest_station_Y"] = [this](const std::string& val) { distance_to_closest_station_Y = std::stod(val); };
        setters["looking_at_AGV"] = [this](const std::string& val) { looking_at_AGV = val == "True"; };
        setters["start_station_X"] = [this](const std::string& val) { start_station_X = std::stod(val); };
        setters["start_station_Y"] = [this](const std::string& val) { start_station_Y = std::stod(val); };
        setters["end_station_X"] = [this](const std::string& val) { end_station_X = std::stod(val); };
        setters["end_station_Y"] = [this](const std::string& val) { end_station_Y = std::stod(val); };
        setters["distance_from_start_station_X"] = [this](const std::string& val) { distance_from_start_station_X = std::stod(val); };
        setters["distance_from_start_station_Y"] = [this](const std::string& val) { distance_from_start_station_Y = std::stod(val); };
        setters["distance_from_end_station_X"] = [this](const std::string& val) { distance_from_end_station_X = std::stod(val); };
        setters["distance_from_end_station_Y"] = [this](const std::string& val) { distance_from_end_station_Y = std::stod(val); };
        setters["facing_start_station"] = [this](const std::string& val) { facing_start_station = val == "True"; };
        setters["facing_end_station"] = [this](const std::string& val) { facing_end_station = val == "True"; };
        setters["looking_at_closest_station"] = [this](const std::string& val) { looking_at_closest_station = val == "True"; };
        setters["GazeDirection_X"] = [this](const std::string& val) { GazeDirection_X = std::stod(val); };
        setters["GazeDirection_Y"] = [this](const std::string& val) { GazeDirection_Y = std::stod(val); };
        setters["AGV_X"] = [this](const std::string& val) { AGV_X = std::stod(val); };
        setters["AGV_Y"] = [this](const std::string& val) { AGV_Y = std::stod(val); };
        setters["User_X"] = [this](const std::string& val) { User_X = std::stod(val); };
        setters["User_Y"] = [this](const std::string& val) { User_Y = std::stod(val); };
        setters["TimestampID"] = [this](const std::string& val) { TimestampID = std::stoi(val); };
        setters["state"] = [this](const std::string& val) { state = val; };
    }

    void setField(const std::string& name, const std::string& value) {
        if (setters.find(name) != setters.end()) {
            setters[name](value);
        }
    }
};

// Base class for finite automation states
class FiniteAutomationState {
public:
    std::string name;
    Features features;
    bool constraints_satisfied = false;
    FiniteAutomationState* next_state = nullptr;

    virtual ~FiniteAutomationState() = default;
    virtual bool check(const Features& features){
        throw std::runtime_error("Function not yet implemented");
    };
    virtual std::pair<FiniteAutomationState*, double> transition(){
        throw std::runtime_error("Function not yet implemented");
    };
    void update_features(const Features& new_features){
        features = new_features;
    };
};

// Error state class
class ErrorState : public FiniteAutomationState {
    std::string S_prev;
    std::unordered_map<std::string, std::vector<double>> MLE;

public:
    ErrorState(const Features& features, const std::string& S_prev = "Error");
    ErrorState(const std::string& S_prev = "Error");
    void initializeMLE();
    bool check(const Features& features){return true;};
    static bool mycheck(const Features& features){return true;};
    std::pair<FiniteAutomationState*, double> transition();
    FiniteAutomationState* getStateByName(const std::string& name, const Features& features);
};

// Other state classes
class AtStationState : public FiniteAutomationState {
public:
    AtStationState(const Features& features);
    bool check(const Features& features){
        // Constraint 1: Be stationary
        bool stationary = std::abs(features.User_speed) <= WALK_STAY_THRESHOLD * 2;
        // Constraint 2: Be within a small distance of the station
        bool near_station_X = features.distance_to_closest_station_X < CLOSE_TO_STATION_THRESHOLD_X * 200;
        bool near_station_Y = features.distance_to_closest_station_Y < CLOSE_TO_STATION_THRESHOLD_Y * 200;
        bool near_station = near_station_X && near_station_Y;
        // Constraint 3: Not be on the road
        bool on_road = features.On_road;
        return stationary && near_station && !on_road;
    };

    static bool mycheck(const Features& features){
        // Constraint 1: Be stationary
        bool stationary = std::abs(features.User_speed) <= WALK_STAY_THRESHOLD * 2;
        // Constraint 2: Be within a small distance of the station
        bool near_station_X = features.distance_to_closest_station_X < CLOSE_TO_STATION_THRESHOLD_X * 200;
        bool near_station_Y = features.distance_to_closest_station_Y < CLOSE_TO_STATION_THRESHOLD_Y * 200;
        bool near_station = near_station_X && near_station_Y;
        // Constraint 3: Not be on the road
        bool on_road = features.On_road;
        return stationary && near_station && !on_road;
    };
    std::pair<FiniteAutomationState*, double> transition();
};

class WaitingState : public FiniteAutomationState {
public:
    WaitingState(const Features& features);
    bool check(const Features& features);
    static bool mycheck(const Features& features){
        // Check based on the speed and various interaction possibilities
        bool is_stationary = std::abs(features.User_speed) <= WALK_STAY_THRESHOLD;
        bool is_interactive = features.possible_interaction || features.looking_at_AGV || features.On_road;
        return is_stationary && is_interactive;
    };
    std::pair<FiniteAutomationState*, double> transition();
};

class CrossingState : public FiniteAutomationState {
public:
    CrossingState(const Features& features);
    bool check(const Features& features);
    static bool mycheck(const Features& features){
        // Check for movement in the Y direction and if on the road, including gazing considerations
        bool moving = std::abs(features.User_speed_Y) > WALK_STAY_THRESHOLD;
        bool on_road = features.On_road;
        bool looking_at_road = features.facing_to_road;
        bool looking_at_agv = features.looking_at_AGV;

        return moving && on_road && (looking_at_road || looking_at_agv);
    };
    std::pair<FiniteAutomationState*, double> transition();
};

class ApproachingSidewalkState : public FiniteAutomationState {
public:
    ApproachingSidewalkState(const Features& features);
    bool check(const Features& features);
    static bool mycheck(const Features& features){
        // Check for proximity to station and movement constraints
        bool near_start_station = std::abs(features.distance_to_closest_station_Y) <= CLOSE_TO_STATION_THRESHOLD_Y * 100 * 2;
        bool moving = features.User_speed_Y > WALK_STAY_THRESHOLD * 0.3;

        return near_start_station && moving && !features.On_road;
    };
    std::pair<FiniteAutomationState*, double> transition();
};

class MovingAlongSidewalkState : public FiniteAutomationState {
public:
    MovingAlongSidewalkState(const Features& features);
    bool check(const Features& features);
    static bool mycheck(const Features& features){
        // Check for movement along the sidewalk within constraints
        bool moving = features.User_speed_X > WALK_STAY_THRESHOLD * 0.8;
        bool within_sidewalk = features.distance_from_start_station_Y < 500 + MARGIN_NEAR_SIDEWALKS * 100;
        within_sidewalk = within_sidewalk || (features.distance_from_end_station_Y < 500 + MARGIN_NEAR_SIDEWALKS * 100);

        return within_sidewalk && moving;
    };
    std::pair<FiniteAutomationState*, double> transition();
};

class ApproachingStationState : public FiniteAutomationState {
public:
    ApproachingStationState(const Features& features);
    bool check(const Features& features);
    static bool mycheck(const Features& features){
        // Check for proximity to the station and other conditions
        bool near_station_X = features.distance_from_end_station_X < STATION_LENGTH * 200;
        bool near_station_Y = features.distance_from_end_station_Y < CLOSE_TO_STATION_THRESHOLD * 150;
        bool looking_at_station = features.facing_end_station;
        bool on_road = features.On_road;

        return !on_road && near_station_X && near_station_Y && (features.User_speed > WALK_STAY_THRESHOLD * 0.2);
    };
    std::pair<FiniteAutomationState*, double> transition();
};

// Finite state machine controller
class FiniteAutomationMachine {
private:
    std::unique_ptr<FiniteAutomationState> current_state;
    std::unique_ptr<FiniteAutomationState> next_state;
    std::vector<bool> errorFlag;
    std::vector<bool> default_error_flag;
    std::string S_prev;

public:
    // Constructor
    FiniteAutomationMachine(const Features& features, int error_flag_size = 3, 
                            std::unique_ptr<FiniteAutomationState> initial_state = std::make_unique<ErrorState>(Features()));

    // Destructor
    ~FiniteAutomationMachine() = default;

    // Run function
    void run(const Features& features, std::unique_ptr<FiniteAutomationState> new_state = nullptr);
    std::string getCurrentStateName() const;
    void setCurrentState(std::unique_ptr<FiniteAutomationState> new_state){
        current_state = std::move(new_state);
    };
    void setCurrentStateByName(const std::string& name){
        if (name == "At Station") {
            current_state = std::make_unique<AtStationState>(Features());
        } else if (name == "Wait") {
            current_state = std::make_unique<WaitingState>(Features());
        } else if (name == "Cross") {
            current_state = std::make_unique<CrossingState>(Features());
        } else if (name == "Approach Target Station") {
            current_state = std::make_unique<ApproachingStationState>(Features());
        } else if (name == "Move Along Sidewalk") {
            current_state = std::make_unique<MovingAlongSidewalkState>(Features());
        } else if (name == "Error") {
            current_state = std::make_unique<ErrorState>(Features());
        } else if (name == "Approach Sidewalk") {
            current_state = std::make_unique<ApproachingSidewalkState>(Features());
        } else {
            throw std::runtime_error("No Such State: " + name);
        }
    };
private:
    bool anyOf(const std::vector<bool>& flags);
};
#endif // FINITE_STATE_MACHINE_HPP