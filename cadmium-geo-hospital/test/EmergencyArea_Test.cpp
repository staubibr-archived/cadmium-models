//Cadmium Simulator headers
#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/dynamic_model.hpp>
#include <cadmium/modeling/dynamic_model_translator.hpp>
#include <cadmium/engine/pdevs_dynamic_runner.hpp>
#include <cadmium/logger/common_loggers.hpp>

//Time class header
//Atomic model headers
#include <cadmium/basic_model/pdevs/iestream.hpp> //Atomic model for inputs
#include <iostream>
#include <string>
#include <NDTime.hpp>
#include <cadmium/web/json.hpp>

//Data structures
#include "../data_structures/emergency.hpp"
#include "../atomics/emergency_area.hpp"

using namespace std;
using namespace cadmium;
using namespace cadmium::basic_models::pdevs;

using TIME = NDTime;
using json = nlohmann::json;

/***** Define input port for coupled models *****/
// none for this test

/***** Define output ports for coupled model *****/
struct generator_out: public out_port<Emergency_t>{};

/****** Emergency Input Reader atomic model declarations *******************/
template<typename T>
class InputReader_t : public iestream_input<Emergency_t,T> {
public:
    InputReader_t () = default;
    InputReader_t (const char* file_path) : iestream_input<Emergency_t, T>(file_path) {}
};

json read_geojson(std::string path) {
	std::ifstream stream(path);

	if (!stream.is_open() || stream.fail()) cout << "Unable to open file";

	json token;

	try {
		if (json::accept(stream)){
			token = json::parse(stream);
		}
	}
	catch (const std::exception& e) {
		std::cerr << e.what();
	}

	stream.close();

	return token;
}

std::vector<json> read_features(json geojson) {
	std::vector<json> features;
	json f_json = geojson.at("features");

	for (json::iterator it = f_json.begin(); it != f_json.end(); ++it) {
		features.push_back(*it);
	}

	return features;
}

int main(){
	json hospitals = read_geojson("../data/hospitals.geojson");
	std::vector<json> f_hospitals = read_features(hospitals);

	json areas = read_geojson("../data/emergency_areas.geojson");
	std::vector<json> f_areas = read_features(areas);

	for (json::iterator it = areas.at("features").begin(); it != areas.at("features").end(); ++it) {
	  std::cout << *it << '\n';
	}

	// bool b_happy = j3["happy"].get<bool>();

    /****** Emergency Input Reader atomic model instantiation *******************/
    const char * i_input_data = "../input_data/EmergencyArea_Test.txt";
    shared_ptr<dynamic::modeling::model> input_reader = dynamic::translate::make_dynamic_atomic_model<InputReader_t, TIME, const char*>("input_reader", move(i_input_data));

    /****** Station Passenger Generator atomic model instantiation *******************/
	struct EmergencyArea_params p;

	p.id = 1;
	p.n_ports = 3;
	p.population = 5647;
	p.emergency_max = 6;

	shared_ptr<dynamic::modeling::model> area1 = dynamic::translate::make_dynamic_atomic_model<EmergencyArea, TIME, EmergencyArea_params>("461147", move(p));

    /*******TOP MODEL********/
    dynamic::modeling::Ports iports_TOP = {};
    dynamic::modeling::Ports oports_TOP = {typeid(generator_out)};

    // Order is important here. If input file has events at the same time has the area has transitions and, if the
    // input reader is first, then the reader will execute an output before the area1 executes a transition and therefore
    // the external transition of the area will be triggered before the internal transition.
    dynamic::modeling::Models submodels_TOP = {area1, input_reader};
    dynamic::modeling::EICs eics_TOP = {};
    dynamic::modeling::EOCs eocs_TOP;
	
    eocs_TOP = {
        dynamic::translate::make_EOC<EmergencyArea_defs::out_1, generator_out>("461147")
	};

    dynamic::modeling::ICs ics_TOP = {
		dynamic::translate::make_IC<iestream_input_defs<Emergency_t>::out, EmergencyArea_defs::rejected_1>("input_reader","461147"),
	};
	
    shared_ptr<dynamic::modeling::coupled<TIME>> TOP;
    TOP = make_shared<dynamic::modeling::coupled<TIME>>("TOP", submodels_TOP, iports_TOP, oports_TOP, eics_TOP, eocs_TOP, ics_TOP);

    /*************** Loggers *******************/
    static ofstream out_messages("../simulation_results/Emergency_Area_Test_Outputs.txt");
    struct oss_sink_messages{
        static ostream& sink(){          
            return out_messages;
        }
    };
    static ofstream out_state("../simulation_results/Emergency_Area_Test_State.txt");
    struct oss_sink_state{
        static ostream& sink(){          
            return out_state;
        }
    };
    
    using state=logger::logger<logger::logger_state, dynamic::logger::formatter<TIME>, oss_sink_state>;
    using log_messages=logger::logger<logger::logger_messages, dynamic::logger::formatter<TIME>, oss_sink_messages>;
    using global_time_mes=logger::logger<logger::logger_global_time, dynamic::logger::formatter<TIME>, oss_sink_messages>;
    using global_time_sta=logger::logger<logger::logger_global_time, dynamic::logger::formatter<TIME>, oss_sink_state>;

    using logger_top=logger::multilogger<state, log_messages, global_time_mes, global_time_sta>;

    /************** Runner call ************************/ 
    dynamic::engine::runner<NDTime, logger_top> r(TOP, {0});
    r.run_until(NDTime("288:00:00:000"));
    return 0;
}
