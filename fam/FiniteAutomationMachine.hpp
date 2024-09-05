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
#include <config.hpp>
#include <constant.hpp>

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