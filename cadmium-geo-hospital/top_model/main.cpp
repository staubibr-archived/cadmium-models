//Cadmium Simulator headers
#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/dynamic_model.hpp>
#include <cadmium/modeling/dynamic_model_translator.hpp>
#include <cadmium/engine/pdevs_dynamic_runner.hpp>
#include <cadmium/basic_model/pdevs/iestream.hpp>
#include <cadmium/web/web_logger.hpp>
#include <cadmium/web/web_model.hpp>
#include <cadmium/web/web_results.hpp>
#include <cadmium/web/web_logger.hpp>
#include <cadmium/web/json.hpp>
#include <NDTime.hpp>
#include <iostream>
#include <string>

//Data structures and atomic models
#include "../data_structures/emergency.hpp"
#include "../atomics/emergency_area.hpp"
#include "../atomics/hospital.hpp"

using namespace std;
using namespace cadmium;
using namespace cadmium::basic_models::pdevs;

using TIME = NDTime;
using json = nlohmann::json;

int main(){
	try {
	    /*******TOP MODEL********/
	    dynamic::modeling::Ports iports_TOP = {};
	    dynamic::modeling::Ports oports_TOP = {};
	    dynamic::modeling::EICs eics_TOP = {};
	    dynamic::modeling::EOCs eocs_TOP = {};
	    dynamic::modeling::ICs ics_TOP = {};
	    dynamic::modeling::Models submodels_TOP = {};

		json hospitals = tools::read_geojson("D:/Data/Geo Hospital 2/output/Gatineau/hospitals.geojson");
		json areas = tools::read_geojson("D:/Data/Geo Hospital 2/output/Gatineau/emergency_areas.geojson");

		map<std::string, EmergencyArea_params> m_areas;
		map<std::string, Hospital_params> m_hospitals;

		for (auto& f : areas.at("features")) {
			struct EmergencyArea_params p = EmergencyArea<TIME>::params_from_feature(f);

			std::string id = p.id;
			m_areas[id] = p;

			submodels_TOP.push_back(web::make_web_atomic_model<EmergencyArea, TIME, EmergencyArea_params>(id, "area", move(p)));
		}

		for (auto& f : hospitals.at("features")) {
			struct Hospital_params p = Hospital<TIME>::params_from_feature(f);

			std::string id = p.id;
			m_hospitals[id] = p;

			submodels_TOP.push_back(web::make_web_atomic_model<Hospital, TIME, Hospital_params>(id, "hospital", move(p)));
		}

	    /*******Internal Couplings********/
		for (auto& f : areas.at("features")) {
			std::string area_id = f.at("properties").at("dauid").get<std::string>();
			std::string hospitals = f.at("properties").at("hospitals").get<std::string>();
			std::vector<std::string> split = tools::split(tools::trim(hospitals), ',');

			EmergencyArea_params area = m_areas[area_id];

			Hospital_params h1 = m_hospitals[split[0]];
			Hospital_params h2 = m_hospitals[split[1]];
			Hospital_params h3 = m_hospitals[split[2]];

			// std::cout << "Linking area " << area.id << " with hospitals " << h1.id << ", " << h2.id << ", " << h3.id << endl;

			// Connect 3 area outputs to each hospital input for emergencies dispatch
			ics_TOP.push_back(dynamic::translate::make_IC<EmergencyArea_defs::out_1, Hospital_defs::processor_in>(area.id, h1.id));
			ics_TOP.push_back(dynamic::translate::make_IC<EmergencyArea_defs::out_2, Hospital_defs::processor_in>(area.id, h2.id));
			ics_TOP.push_back(dynamic::translate::make_IC<EmergencyArea_defs::out_3, Hospital_defs::processor_in>(area.id, h3.id));

			// Connect hospital outputs to area for rejected emergencies
			ics_TOP.push_back(dynamic::translate::make_IC<Hospital_defs::processor_out, EmergencyArea_defs::rejected_1>(h1.id, area.id));
			ics_TOP.push_back(dynamic::translate::make_IC<Hospital_defs::processor_out, EmergencyArea_defs::rejected_1>(h2.id, area.id));
			ics_TOP.push_back(dynamic::translate::make_IC<Hospital_defs::processor_out, EmergencyArea_defs::rejected_1>(h3.id, area.id));
		}

	    shared_ptr<web::coupled_web<TIME>> TOP = web::make_web_top_model<TIME>("gis_emergencies_1", "gis_emergencies", submodels_TOP, iports_TOP, oports_TOP, eics_TOP, eocs_TOP, ics_TOP);

	    /*************** Loggers *******************/
	    std::string s_out_messages = "../simulation_results/Emergency_Area_Test_Outputs.txt";
	    static ofstream out_messages(s_out_messages);
	    struct oss_sink_messages{
	        static ostream& sink(){
	            return out_messages;
	        }
	    };

	    std::string s_out_state = "../simulation_results/Emergency_Area_Test_State.txt";
	    static ofstream out_state(s_out_state);
	    struct oss_sink_state{
	        static ostream& sink(){
	            return out_state;
	        }
	    };

	    using state=web::logger<logger::logger_state, web::formatter<TIME>, oss_sink_state>;
	    using log_messages=web::logger<logger::logger_messages, web::formatter<TIME>, oss_sink_messages>;
	    using global_time_mes=web::logger<logger::logger_global_time, web::formatter<TIME>, oss_sink_messages>;
	    using global_time_sta=web::logger<logger::logger_global_time, web::formatter<TIME>, oss_sink_state>;

	    using logger_top=web::multilogger<state, log_messages, global_time_mes, global_time_sta>;

	    /************** Runner call ************************/
	    dynamic::engine::runner<NDTime, logger_top> r(TOP, {0});
	    r.run_until(NDTime("2400:00:00:000"));

	    web::output_results(TOP, s_out_state, s_out_messages, "../simulation_results/");

	    return 0;
	}
	catch (const std::exception& e) {
		std::cerr << e.what();
	}

    return 0;
}
