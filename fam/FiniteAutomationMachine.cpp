#ifndef FINITE_STATE_MACHINE_HPP
#define FINITE_STATE_MACHINE_HPP
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cassert>
#include <memory>
#include "FiniteAutomationMachine.hpp"

class FiniteAutomationState {
public:
    std::string name;
    Features features;
    bool constraints_satisfied = false;
    FiniteAutomationState* next_state = nullptr;

    static bool check(const Features& features);
    virtual std::pair<FiniteAutomationState*, double> transition() = 0;

    void update_features(const Features& new_features) {
        features = new_features;
    }
};


class ErrorState : public FiniteAutomationState {
    std::string S_prev;
    std::unordered_map<std::string, std::vector<double>> MLE;

public:
    ErrorState(const Features& features, const std::string& S_prev = "Error") {
        name = "Error";
        this->features = features;
        this->S_prev = S_prev;
        initializeMLE();
    }

    ErrorState(const std::string& S_prev = "Error") {
        name = "Error";
        this->S_prev = S_prev;
        initializeMLE();
    }

    void initializeMLE() {
        // Initialize the MLE table manually as C++ doesn't support DataFrame directly
        MLE["Wait"] = {0.022282, 0.805907, 0.087452, 0.015152, 0.000000, 0.0075};
        MLE["At Station"] = {0.950089, 0.018987, 0.000000, 0.000000, 0.252874, 0.0000};
        MLE["Approach Sidewalk"] = {0.023619, 0.126582, 0.562738, 0.000000, 0.000000, 0.0050};
        MLE["Move Along Sidewalk"] = {0.000446, 0.008439, 0.087452, 0.003367, 0.004598, 0.9200};
        MLE["Approach Target Station"] = {0.003565, 0.000000, 0.000000, 0.131313, 0, 0.0650};
        MLE["Cross"] = {0.000000, 0.040084, 0.262357, 0.850168, 0.000000, 0.0025};
    }

    static bool check(const Features& features) {
        // This state always returns true for the check
        return true;
    }

    std::pair<FiniteAutomationState*, double> transition() override {
        // Simulating the transition logic based on provided features and previous state
        std::vector<std::pair<std::string, double>> Q;

        // Assuming these check functions are static and correctly defined in respective classes
        if (AtStationState::check(features)) Q.push_back({"At Station", MLE["At Station"][0]});
        if (WaitingState::check(features)) Q.push_back({"Wait", MLE["Wait"][0]});
        if (CrossingState::check(features)) Q.push_back({"Cross", MLE["Cross"][0]});
        if (ApproachingSidewalkState::check(features)) Q.push_back({"Approach Sidewalk", MLE["Approach Sidewalk"][0]});
        if (MovingAlongSidewalkState::check(features)) Q.push_back({"Move Along Sidewalk", MLE["Move Along Sidewalk"][0]});
        if (ApproachingStationState::check(features)) Q.push_back({"Approach Target Station", MLE["Approach Target Station"][0]});

        if (Q.empty()) {
            return {this, 1.0};
        } else {
            std::sort(Q.begin(), Q.end(), [](const auto& a, const auto& b) {
                return a.second > b.second;
            });
            const auto& [state_name, prob] = Q.front();
            // Example on how to instantiate the next state based on state_name
            // This requires a proper mapping of names to state instances
            return {getStateByName(state_name, features), prob};
        }
    }

    FiniteAutomationState* getStateByName(const std::string& name, const Features& features) {
        // You need to implement a factory or similar to construct states by name
        if (name == "At Station") return new AtStationState(features);
        if (name == "Wait") return new WaitingState(features);
        if (name == "Cross") return new CrossingState(features);
        if (name == "Approach Sidewalk") return new ApproachingSidewalkState(features);
        if (name == "Move Along Sidewalk") return new MovingAlongSidewalkState(features);
        if (name == "Approach Target Station") return new ApproachingStationState(features);
        return this; // Default return to self, should be improved
    }
};



class AtStationState : public FiniteAutomationState {
public:
    AtStationState(const Features& features) {
        name = "At Station";
        this->features = features;
    }

    static bool check(const Features& features) {
        // Constraint 1: Be stationary
        bool stationary = std::abs(features.User_speed) <= WALK_STAY_THRESHOLD * 2;

        // Constraint 2: Be within a small distance of the station
        bool near_station_X = features.distance_to_closest_station_X < CLOSE_TO_STATION_THRESHOLD_X * 200;
        bool near_station_Y = features.distance_to_closest_station_Y < CLOSE_TO_STATION_THRESHOLD_Y * 200;
        bool near_station = near_station_X && near_station_Y;

        // Constraint 3: Not be on the road
        bool on_road = features.On_road;

        return stationary && near_station && !on_road;
    }

    std::pair<FiniteAutomationState*, double> transition() override {
        // AtStation -> ApproachingSidewalkState or WaitingState
        if (std::abs(features.User_speed) > WALK_STAY_THRESHOLD &&
            (features.On_sidewalks || features.facing_along_sidewalk)) {
            next_state = new ApproachingSidewalkState(features);  // Assuming this state is properly defined elsewhere
            return {next_state, 1.0};
        } else if (std::abs(features.User_speed) <= WALK_STAY_THRESHOLD &&
                   features.intent_to_cross && features.possible_interaction) {
            next_state = new WaitingState(features);  // Assuming this state is properly defined elsewhere
            return {next_state, 1.0};
        } else {
            // Stay in AtStation State
            return {this, 1.0};
        }
    }
};


class WaitingState : public FiniteAutomationState {
public:
    WaitingState(const Features& features) {
        name = "Wait";
        this->features = features;
    }

    static bool check(const Features& features) {
        // Check based on the speed and various interaction possibilities
        bool is_stationary = std::abs(features.User_speed) <= WALK_STAY_THRESHOLD;
        bool is_interactive = features.possible_interaction || features.looking_at_AGV || features.On_road;

        return is_stationary && is_interactive;
    }

    std::pair<FiniteAutomationState*, double> transition() override {
        // Determine the next state based on the current feature conditions

        // WaitingState -> CrossingState
        if (std::abs(features.User_speed) > 0.8 * WALK_STAY_THRESHOLD &&
            features.On_road && features.facing_to_road) {
            next_state = new CrossingState(features);  // Assuming this state is properly defined elsewhere
            return {next_state, 1.0};
        }

        // WaitingState -> ApproachingSidewalkState
        if (std::abs(features.User_speed) > WALK_STAY_THRESHOLD && features.On_sidewalks) {
            next_state = new ApproachingSidewalkState(features);  // Assuming this state is properly defined elsewhere
            return {next_state, 1.0};
        }

        // WaitingState -> MovingAlongSidewalkState
        if (std::abs(features.User_speed_X) > 0.8 * WALK_STAY_THRESHOLD &&
            (features.On_sidewalks || features.facing_along_sidewalk)) {
            next_state = new MovingAlongSidewalkState(features);  // Assuming this state is properly defined elsewhere
            return {next_state, 1.0};
        }

        // Stay in the WaitingState if no conditions are met
        return {this, 1.0};
    }
};


class CrossingState : public FiniteAutomationState {
public:
    CrossingState(const Features& features) {
        name = "Cross";
        this->features = features;
    }

    static bool check(const Features& features) {
        // Check for movement in the Y direction and if on the road, including gazing considerations
        bool moving = std::abs(features.User_speed_Y) > WALK_STAY_THRESHOLD;
        bool on_road = features.On_road;
        bool looking_at_road = features.facing_to_road;
        bool looking_at_agv = features.looking_at_AGV;

        return moving && on_road && (looking_at_road || looking_at_agv);
    }

    std::pair<FiniteAutomationState*, double> transition() override {
        // Determine the next state based on the current features

        // CrossingState -> MovingAlongSidewalkState
        if (features.On_sidewalks &&
            (std::abs(features.User_speed_X) > 1.5 * std::abs(features.User_speed_Y) ||
             (features.facing_along_sidewalk && std::abs(features.User_speed_X) > 0.5 * WALK_STAY_THRESHOLD))) {
            next_state = new MovingAlongSidewalkState(features);  // Assuming this state is properly defined elsewhere
            return {next_state, 1.0};
        }

        // CrossingState -> ApproachingStationState
        if (std::abs(features.User_speed) > WALK_STAY_THRESHOLD &&
            features.closest_station == features.Gazing_station &&
            !features.On_road) {
            next_state = new ApproachingStationState(features);  // Assuming this state is properly defined elsewhere
            return {next_state, 1.0};
        }

        // CrossingState -> WaitingState (wait for AGV)
        if (std::abs(features.User_speed) < WALK_STAY_THRESHOLD &&
            features.possible_interaction &&
            features.looking_at_AGV &&
            features.On_road) {
            next_state = new WaitingState(features);  // Assuming this state is properly defined elsewhere
            return {next_state, 1.0};
        }

        // CrossingState -> AtStationState (previously ArrivedState, but simplified for C++)
        if (std::abs(features.User_speed) < WALK_STAY_THRESHOLD &&
            !features.facing_to_road &&
            features.distance_to_closest_station <= CLOSE_TO_STATION_THRESHOLD * 100) {
            next_state = new AtStationState(features);  // Assuming this state is properly defined elsewhere
            return {next_state, 1.0};
        }

        // Stay in the CrossingState if no conditions are met
        return {this, 1.0};
    }
};



class ApproachingSidewalkState : public FiniteAutomationState {
public:
    ApproachingSidewalkState(const Features& features) {
        name = "Approach Sidewalk";
        this->features = features;
    }

    static bool check(const Features& features) {
        // Check for proximity to station and movement constraints
        bool near_start_station = std::abs(features.distance_to_closest_station_Y) <= CLOSE_TO_STATION_THRESHOLD_Y * 100 * 2;
        bool moving = features.User_speed_Y > WALK_STAY_THRESHOLD * 0.3;

        return near_start_station && moving && !features.On_road;
    }

    std::pair<FiniteAutomationState*, double> transition() override {
        // Determine the next state based on the current features

        bool near_station_X = features.distance_to_closest_station_X < CLOSE_TO_STATION_THRESHOLD_X * 100;
        bool near_station_Y = features.distance_to_closest_station_Y < CLOSE_TO_STATION_THRESHOLD_Y * 100;
        bool near_station = near_station_X && near_station_Y;

        // ApproachingSidewalkState -> CrossingState
        if (std::abs(features.User_speed_Y) > 0.5 * WALK_STAY_THRESHOLD &&
            features.facing_to_road && features.On_road) {
            next_state = new CrossingState(features);  // Assuming this state is properly defined elsewhere
            return {next_state, 1.0};
        }

        // ApproachingSidewalkState -> WaitingState
        if (std::abs(features.User_speed) < WALK_STAY_THRESHOLD &&
            features.intent_to_cross && features.possible_interaction) {
            next_state = new WaitingState(features);  // Assuming this state is properly defined elsewhere
            return {next_state, 1.0};
        }

        // ApproachingSidewalkState -> MovingAlongSidewalkState
        if ((std::abs(features.User_speed_X) > 1.5 * std::abs(features.User_speed_Y) ||
             (features.facing_along_sidewalk && std::abs(features.User_speed_X) > WALK_STAY_THRESHOLD)) &&
            (!near_station || features.facing_along_sidewalk)) {
            next_state = new MovingAlongSidewalkState(features);  // Assuming this state is properly defined elsewhere
            return {next_state, 1.0};
        }

        // Stay in the ApproachingSidewalkState if no conditions are met
        return {this, 1.0};
    }
};



class MovingAlongSidewalkState : public FiniteAutomationState {
public:
    MovingAlongSidewalkState(const Features& features) {
        name = "Move Along Sidewalk";
        this->features = features;
    }

    static bool check(const Features& features) {
        // Check for movement along the sidewalk within constraints
        bool moving = features.User_speed_X > WALK_STAY_THRESHOLD * 0.8;
        bool within_sidewalk = features.distance_from_start_station_Y < 500 + MARGIN_NEAR_SIDEWALKS * 100;
        within_sidewalk = within_sidewalk || (features.distance_from_end_station_Y < 500 + MARGIN_NEAR_SIDEWALKS * 100);

        return within_sidewalk && moving;
    }

    std::pair<FiniteAutomationState*, double> transition() override {
        // Determine the next state based on current features

        // MovingAlongSidewalkState -> CrossingState
        if ((std::abs(features.User_speed_Y) > 1.5 * std::abs(features.User_speed_X) ||
             (std::abs(features.User_speed_Y) > WALK_STAY_THRESHOLD && features.facing_to_road)) &&
            (features.intent_to_cross || features.On_road)) {
            next_state = new CrossingState(features);  // Assuming this state is properly defined elsewhere
            return {next_state, 1.0};
        }

        // MovingAlongSidewalkState -> WaitingState
        if (std::abs(features.User_speed) < WALK_STAY_THRESHOLD &&
            features.intent_to_cross && features.possible_interaction) {
            next_state = new WaitingState(features);  // Assuming this state is properly defined elsewhere
            return {next_state, 1.0};
        }

        // MovingAlongSidewalkState -> ApproachingStationState
        if ((std::abs(features.User_speed) < WALK_STAY_THRESHOLD || features.looking_at_closest_station) &&
            !features.facing_to_road &&
            features.distance_to_closest_station <= CLOSE_TO_STATION_THRESHOLD * 200) {
            next_state = new ApproachingStationState(features);  // Assuming this state is properly defined elsewhere
            return {next_state, 1.0};
        }

        // Stay in the MovingAlongSidewalkState if no conditions are met
        return {this, 1.0};
    }
};


class ApproachingStationState : public FiniteAutomationState {
public:
    ApproachingStationState(const Features& features) {
        name = "Approach Target Station";
        this->features = features;
    }

    static bool check(const Features& features) {
        // Check for proximity to the station and other conditions
        bool near_station_X = features.distance_from_end_station_X < STATION_LENGTH * 200;
        bool near_station_Y = features.distance_from_end_station_Y < CLOSE_TO_STATION_THRESHOLD * 150;
        bool looking_at_station = features.facing_end_station;
        bool on_road = features.On_road;

        return !on_road && near_station_X && near_station_Y && (features.User_speed > WALK_STAY_THRESHOLD * 0.2);
    }

    std::pair<FiniteAutomationState*, double> transition() override {
        // Determine the next state based on the current features
        if (std::abs(features.User_speed) < WALK_STAY_THRESHOLD &&
            !features.facing_to_road &&
            features.distance_to_closest_station <= CLOSE_TO_STATION_THRESHOLD * 300) {
            next_state = new AtStationState(features);  // Assuming AtStationState is properly defined elsewhere
            return {next_state, 1.0};
        }

        // Stay in ApproachingStationState if no transition conditions are met
        return {this, 1.0};
    }
};




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
                            std::unique_ptr<FiniteAutomationState> initial_state = std::make_unique<ErrorState>())
        : current_state(std::move(initial_state)), S_prev("Error") {

        // Initialize error flags
        if (error_flag_size < 2) {
            default_error_flag.assign(error_flag_size, true);
        } else {
            default_error_flag.assign(2, true);
            default_error_flag.insert(default_error_flag.end(), error_flag_size - 2, false);
        }
        errorFlag = default_error_flag;
    }

    // Run function
    void run(const Features& features, std::unique_ptr<FiniteAutomationState> new_state = nullptr) {
        if (new_state) {
            current_state = std::move(new_state);
        }

        current_state->update_features(features);
        auto [next, probability] = current_state->transition();
        next_state.reset(next);

        if (probability > 0.8) {
            current_state = std::move(next_state);
        }

        // Check whether the constraints are satisfied
        bool constraints_satisfied = current_state->check(features);
        errorFlag.erase(errorFlag.begin());
        errorFlag.push_back(constraints_satisfied);

        // If constraints are not satisfied for past 3 times, move to ErrorState
        if (!anyOf(errorFlag) && !errorFlag.empty()) {
            S_prev = current_state->name;  // Assuming `getName()` method exists in FiniteAutomationState
            current_state = std::make_unique<ErrorState>(features, S_prev);
            errorFlag = default_error_flag;
        }
    }

    std::string FiniteAutomationMachine::getCurrentStateName() const {
    if (current_state) {
        return current_state->name;  // Assuming `getName()` method exists in FiniteAutomationState
    }
    return "Unknown";  // Return a default value if the current state is not set
    }

    private:
    bool anyOf(const std::vector<bool>& flags) {
        for (bool flag : flags) {
            if (flag) return true;
        }
        return false;
    }
};

#endif