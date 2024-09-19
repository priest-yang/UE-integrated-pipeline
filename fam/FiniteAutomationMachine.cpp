#include "FiniteAutomationMachine.hpp"


ErrorState::ErrorState(const Features& features, const std::string& S_prev) {
    name = "Error";
    this->features = features;
    this->S_prev = S_prev;
    initializeMLE();
}

ErrorState::ErrorState(const std::string& S_prev) {
        name = "Error";
        this->S_prev = S_prev;
        initializeMLE();
    }

void ErrorState::initializeMLE() {
        // Initialize the MLE table manually as C++ doesn't support DataFrame directly
        MLE["Wait"] = {0.022282, 0.805907, 0.087452, 0.015152, 0.000000, 0.0075};
        MLE["At Station"] = {0.950089, 0.018987, 0.000000, 0.000000, 0.252874, 0.0000};
        MLE["Approach Sidewalk"] = {0.023619, 0.126582, 0.562738, 0.000000, 0.000000, 0.0050};
        MLE["Move Along Sidewalk"] = {0.000446, 0.008439, 0.087452, 0.003367, 0.004598, 0.9200};
        MLE["Approach Target Station"] = {0.003565, 0.000000, 0.000000, 0.131313, 0, 0.0650};
        MLE["Cross"] = {0.000000, 0.040084, 0.262357, 0.850168, 0.000000, 0.0025};

        index = {"At Station", "Wait", "Approach Sidewalk", "Cross", "Approach Target Station", "Move Along Sidewalk"};

    }


    std::pair<FiniteAutomationState*, double> ErrorState::transition() {
        if (this->S_prev == "Error"){
            if (AtStationState::mycheck(features)) return {new AtStationState(features), 1.0};
            if (WaitingState::mycheck(features)) return {new WaitingState(features), 1.0};
            if (CrossingState::mycheck(features)) return {new CrossingState(features), 1.0};
            if (ApproachingSidewalkState::mycheck(features)) return {new ApproachingSidewalkState(features), 1.0};
            if (MovingAlongSidewalkState::mycheck(features)) return {new MovingAlongSidewalkState(features), 1.0};
            if (ApproachingStationState::mycheck(features)) return {new ApproachingStationState(features), 1.0};
            return {this, 1.0};
        }

        // Simulating the transition logic based on provided features and previous state
        std::vector<std::pair<std::string, double>> Q;

        // Assuming these check functions are static and correctly defined in respective classes
        if (AtStationState::mycheck(features)) Q.push_back({"At Station", MLE[this->S_prev][std::find(index.begin(), index.end(), "At Station") - index.begin()]});
        if (WaitingState::mycheck(features)) Q.push_back({"Wait", MLE[this->S_prev][std::find(index.begin(), index.end(), "Wait") - index.begin()]});
        if (CrossingState::mycheck(features)) Q.push_back({"Cross", MLE[this->S_prev][std::find(index.begin(), index.end(), "Cross") - index.begin()]});
        if (ApproachingSidewalkState::mycheck(features)) Q.push_back({"Approach Sidewalk", MLE[this->S_prev][std::find(index.begin(), index.end(), "Approach Sidewalk") - index.begin()]});
        if (MovingAlongSidewalkState::mycheck(features)) Q.push_back({"Move Along Sidewalk", MLE[this->S_prev][std::find(index.begin(), index.end(), "Move Along Sidewalk") - index.begin()]});
        if (ApproachingStationState::mycheck(features)) Q.push_back({"Approach Target Station", MLE[this->S_prev][std::find(index.begin(), index.end(), "Approach Target Station") - index.begin()]});

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

    FiniteAutomationState* ErrorState::getStateByName(const std::string& name, const Features& features) {
        // You need to implement a factory or similar to construct states by name
        if (name == "At Station") return new AtStationState(features);
        if (name == "Wait") return new WaitingState(features);
        if (name == "Cross") return new CrossingState(features);
        if (name == "Approach Sidewalk") return new ApproachingSidewalkState(features);
        if (name == "Move Along Sidewalk") return new MovingAlongSidewalkState(features);
        if (name == "Approach Target Station") return new ApproachingStationState(features);
        return this; // Default return to self, should be improved
    }




    AtStationState::AtStationState(const Features& features) {
        name = "At Station";
        this->features = features;
    }
    

    std::pair<FiniteAutomationState*, double> AtStationState::transition(){
        // AtStation -> ApproachingSidewalkState or WaitingState
        bool is_moving = std::abs(features.User_speed) > WALK_STAY_THRESHOLD;
        if (is_moving &&
            (features.On_sidewalks || features.facing_along_sidewalk)) {
            next_state = new ApproachingSidewalkState(features);  // Assuming this state is properly defined elsewhere
            return {next_state, 1.0};
        } else if (!is_moving &&
                   features.intent_to_cross && features.possible_interaction) {
            next_state = new WaitingState(features);  // Assuming this state is properly defined elsewhere
            return {next_state, 1.0};
        } else {
            // Stay in AtStation State
            return {this, 1.0};
        }
    }




    WaitingState::WaitingState(const Features& features) {
        name = "Wait";
        this->features = features;
    }

    bool WaitingState::check(const Features& features) {
        // Check based on the speed and various interaction possibilities
        bool is_stationary = std::abs(features.User_speed) <= WALK_STAY_THRESHOLD;
        bool is_interactive = features.possible_interaction || features.looking_at_AGV || features.On_road;

        return is_stationary && is_interactive;
    }


    std::pair<FiniteAutomationState*, double> WaitingState::transition()  {
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




    CrossingState::CrossingState(const Features& features) {
        name = "Cross";
        this->features = features;
    }

    bool CrossingState::check(const Features& features) {
        // Check for movement in the Y direction and if on the road, including gazing considerations
        bool moving = std::abs(features.User_speed_Y) > WALK_STAY_THRESHOLD;
        bool on_road = features.On_road;
        bool looking_at_road = features.facing_to_road;
        bool looking_at_agv = features.looking_at_AGV;

        return moving && on_road && (looking_at_road || looking_at_agv);
    }

    std::pair<FiniteAutomationState*, double> CrossingState::transition(){
        // Determine the next state based on the current features

        // CrossingState -> MovingAlongSidewalkState
        if (features.On_sidewalks &&
            (std::abs(features.User_speed_X) > 1.5 * std::abs(features.User_speed_Y) ||
             (features.facing_along_sidewalk && std::abs(features.User_speed_X) > 0.5 * WALK_STAY_THRESHOLD))) {
            next_state = new MovingAlongSidewalkState(features);  // Assuming this state is properly defined elsewhere
            return {next_state, 1.0};
        }

        bool is_moving = std::abs(features.User_speed) > WALK_STAY_THRESHOLD;
        // CrossingState -> ApproachingStationState
        if (is_moving &&
            features.closest_station == features.Gazing_station &&
            !features.On_road) {
            next_state = new ApproachingStationState(features);  // Assuming this state is properly defined elsewhere
            return {next_state, 1.0};
        }

        // CrossingState -> WaitingState (wait for AGV)
        if (!is_moving &&
            features.possible_interaction &&
            features.looking_at_AGV &&
            features.On_road) {
            next_state = new WaitingState(features);  // Assuming this state is properly defined elsewhere
            return {next_state, 1.0};
        }

        // CrossingState -> AtStationState (previously ArrivedState, but simplified for C++)
        if (!is_moving &&
            !features.facing_to_road &&
            features.distance_to_closest_station <= CLOSE_TO_STATION_THRESHOLD * 100) {
            next_state = new AtStationState(features);  // Assuming this state is properly defined elsewhere
            return {next_state, 1.0};
        }

        // Stay in the CrossingState if no conditions are met
        return {this, 1.0};
    }





    ApproachingSidewalkState::ApproachingSidewalkState(const Features& features) {
        name = "Approach Sidewalk";
        this->features = features;
    }

    bool ApproachingSidewalkState::check(const Features& features) {
        // Check for proximity to station and movement constraints
        bool near_start_station = std::abs(features.distance_to_closest_station_Y) <= CLOSE_TO_STATION_THRESHOLD_Y * 100 * 2;
        bool moving = features.User_speed_Y > WALK_STAY_THRESHOLD * 0.3;

        return near_start_station && moving && !features.On_road;
    }

    std::pair<FiniteAutomationState*, double> ApproachingSidewalkState::transition()  {
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
             (features.facing_along_sidewalk && std::abs(features.User_speed_X) > WALK_STAY_THRESHOLD)) && //TODO: Different from line 175. Why?
            (!near_station || features.facing_along_sidewalk)) {
            next_state = new MovingAlongSidewalkState(features);  // Assuming this state is properly defined elsewhere
            return {next_state, 1.0};
        }

        // Stay in the ApproachingSidewalkState if no conditions are met
        return {this, 1.0};
    }




    MovingAlongSidewalkState::MovingAlongSidewalkState(const Features& features) {
        name = "Move Along Sidewalk";
        this->features = features;
    }

    bool MovingAlongSidewalkState::check(const Features& features) {
        // Check for movement along the sidewalk within constraints
        bool moving = features.User_speed_X > WALK_STAY_THRESHOLD * 0.8;
        bool within_sidewalk = features.distance_from_start_station_Y < 500 + MARGIN_NEAR_SIDEWALKS * 100;
        within_sidewalk = within_sidewalk || (features.distance_from_end_station_Y < 500 + MARGIN_NEAR_SIDEWALKS * 100);

        return within_sidewalk && moving;
    }

    std::pair<FiniteAutomationState*, double> MovingAlongSidewalkState::transition()  {
        // Determine the next state based on current features

        // MovingAlongSidewalkState -> CrossingState
        if ((std::abs(features.User_speed_Y) > 1.5 * std::abs(features.User_speed_X) ||
             (std::abs(features.User_speed_Y) > WALK_STAY_THRESHOLD && features.facing_to_road)) &&
            (features.intent_to_cross || features.On_road)) {
            next_state = new CrossingState(features);  // Assuming this state is properly defined elsewhere
            return {next_state, 1.0};
        }

        bool is_stationary = std::abs(features.User_speed) < WALK_STAY_THRESHOLD;

        // MovingAlongSidewalkState -> WaitingState
        if (is_stationary &&
            features.intent_to_cross && features.possible_interaction) {
            next_state = new WaitingState(features);  // Assuming this state is properly defined elsewhere
            return {next_state, 1.0};
        }

        // MovingAlongSidewalkState -> ApproachingStationState
        if ((is_stationary || features.looking_at_closest_station) &&
            !features.facing_to_road &&
            features.distance_to_closest_station <= CLOSE_TO_STATION_THRESHOLD * 200) {
            next_state = new ApproachingStationState(features);  // Assuming this state is properly defined elsewhere
            return {next_state, 1.0};
        }

        // Stay in the MovingAlongSidewalkState if no conditions are met
        return {this, 1.0};
    }



    ApproachingStationState::ApproachingStationState(const Features& features) {
        name = "Approach Target Station";
        this->features = features;
    }

    bool ApproachingStationState::check(const Features& features) {
        // Check for proximity to the station and other conditions
        bool near_station_X = features.distance_from_end_station_X < STATION_LENGTH * 200;
        bool near_station_Y = features.distance_from_end_station_Y < CLOSE_TO_STATION_THRESHOLD * 150;
        bool looking_at_station = features.facing_end_station;
        bool on_road = features.On_road;

        return !on_road && near_station_X && near_station_Y && (features.User_speed > WALK_STAY_THRESHOLD * 0.2);
    }

    std::pair<FiniteAutomationState*, double> ApproachingStationState::transition()  {
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



    FiniteAutomationMachine::FiniteAutomationMachine(const Features& features, int error_flag_size, 
                            std::unique_ptr<FiniteAutomationState> initial_state)
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
    void FiniteAutomationMachine::run(const Features& features, std::unique_ptr<FiniteAutomationState> new_state) {
        if (new_state) {
            current_state = std::move(new_state);
        }

        current_state->update_features(features);
        auto [next, probability] = current_state->transition();

        if (probability > 0.8 && next->name != current_state->name) {
            next_state.reset(next);
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

    
    bool FiniteAutomationMachine::anyOf(const std::vector<bool>& flags) {
        for (bool flag : flags) {
            if (flag) return true;
        }
        return false;
    }
