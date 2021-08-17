
#ifndef _EMERGENCY_PROCESSOR_HPP__
#define _EMERGENCY_PROCESSOR_HPP__

#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/message_bag.hpp>

#include <limits>
#include <assert.h>
#include <string>
#include <random>
#include <cmath>
#include <iostream>
#include <vector>

#include <cadmium/web/json.hpp>
#include <cadmium/web/web_model.hpp>
#include <cadmium/web/output/message_type.hpp>

#include "../data_structures/emergency.hpp"

using namespace cadmium;
using namespace std;
using json = nlohmann::json;

//Port definition
struct Hospital_defs{
    struct processor_out : public cadmium::web::web_out_port<Emergency_t> {};
    struct processor_in : public cadmium::web::web_in_port<Emergency_t> {};
};

// model parameters
struct Hospital_params {
	std::string id = "";
	std::string name = "";
	int rate = 0;
	int capacity = 0;
};

template<typename TIME>
class Hospital{
public:

    // ports definition
    using output_ports = tuple<typename Hospital_defs::processor_out>;
    using input_ports = tuple<typename Hospital_defs::processor_in>;

    // state definition
    struct state_type{
        int active = 0;
        int total = 0;
        int released = 0;
        int rejected = 0;
        std::vector<Emergency_t> emergencies;
    };

    state_type state;
    Hospital_params params;

    Hospital() {}

    Hospital(Hospital_params ext_params) {
    	params = ext_params;
    }

    static Hospital_params params_from_feature(json j) {
		struct Hospital_params p;

		p.id = j.at("properties").at("index").get<std::string>();
		p.name = j.at("properties").at("facility_name").get<std::string>();
		p.rate = j.at("properties").at("rate").get<int>();
		p.capacity = j.at("properties").at("capacity").get<int>();

		return p;
    }

    void external_transition(TIME e, typename make_message_bags<input_ports>::type mbs) {
        for(const auto &msg : get_messages<typename Hospital_defs::processor_in>(mbs)){
            state.active += msg.quantity;
            state.total += msg.quantity;

            int rejected = state.active > params.capacity ? state.active - params.capacity : 0;

            state.rejected += rejected;
            state.active -= rejected;

            if (rejected > 0) {
            	state.emergencies.push_back({ msg.area_id, msg.port_i, rejected });
            }
        }
    }

    void internal_transition() {
    	if (state.emergencies.size() > 0) state.emergencies.clear();

    	else {
            int released = params.rate > state.active ? state.active : params.rate;

            state.active -= released;
            state.released += released;
    	}
    }

    // output function
    typename make_message_bags<output_ports>::type output() const {
        typename make_message_bags<output_ports>::type bags;

		for(const auto &em : state.emergencies){
			std::get<message_bag<typename Hospital_defs::processor_out>>(bags).messages.push_back(em);
		}

        return bags;
    }

    TIME time_advance() const {
        if (state.emergencies.size() > 0) return TIME("00:00:00:000");

        if (state.active > 0) return TIME("24:00:00:000");

        else return numeric_limits<TIME>::infinity();
    }

    void confluence_transition(TIME e, typename make_message_bags<input_ports>::type mbs) {
        internal_transition();
        external_transition(TIME(), move(mbs));
    }

    friend ostringstream& operator<<(ostringstream& os, const typename Hospital<TIME>::state_type& i) {
        os << i.active << "," << i.total << "," << i.released << "," << i.rejected;

        return os;
    }

    message_type get_state_message_type() {
    	vector<string> fields({ "active", "total", "released", "rejected" });
    	string description = "Number of active, total, released and rejected emergencies at this hospital.";

    	return message_type("s_hospital", fields, description);
    }

	vector<message_type> get_port_message_type() {

    	return {  };
	}
};
#endif // _EMERGENCY_PROCESSOR_HPP__
