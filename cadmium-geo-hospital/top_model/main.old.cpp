#include <iostream>
#include <string>
#include <vector>
#include <NDTime.hpp>

#include <external/json.hpp>
#include <web/web_logger.hpp>
#include <web/web_ports.hpp>
#include <web/web_model.hpp>
#include <web/web_results.hpp>
#include <web/web_tools.hpp>

#include "../atomics/emergency_area.hpp"
#include "../atomics/hospital.hpp"

using json = nlohmann::json;

int main(){
	try {
	    web::Ports iports_TOP = {};
	    web::Ports oports_TOP = {};
	    web::EICs eics_TOP = {};
	    web::EOCs eocs_TOP = {};
	    web::ICs ics_TOP = {};
	    web::Models submodels_TOP = {};

	    // NOTE: THIS IS FOR MY SPECIFIC MODEL, NOT NEEDED FOR STUDENTS
		json hospitals = web::tools::read_geojson("D:/Data/Geo Hospital 2/output/hospitals.geojson");
		json areas = web::tools::read_geojson("D:/Data/Geo Hospital 2/output/emergency_areas.geojson");

		map<string, json> m_areas;
		map<string, json> m_hospitals;

		// NOTE: I BUILD MODELS BASED ON THE GEOJSON FILES ABOVE, STUDENTS WOULD BUILD MODELS THE NORMAL WAY
		for (auto& f : areas.at("features")) {
			json j = f.at("properties");
			string id = j.at("dauid").get<string>();
			m_areas[id] = j;

			// NOTE: SLIGHT DIFFERENCE HERE, THERE'S A SECOND PARAMETER TO THE CALL TO MAKE AN ATOMIC MODEL. THAT PARAMETER IS A STRING IDENTIFYING THE MODEL TYPE.
			submodels_TOP.push_back(web::make_atomic_model<emergency_area, NDTime, json>(id, "area", move(j)));
		}

		for (auto& f : hospitals.at("features")) {
			json j = f.at("properties");
			string id = j.at("index").get<string>();
			m_hospitals[id] = j;

			// NOTE: SLIGHT DIFFERENCE HERE, THERE'S A SECOND PARAMETER TO THE CALL TO MAKE AN ATOMIC MODEL. THAT PARAMETER IS A STRING IDENTIFYING THE MODEL TYPE.
			submodels_TOP.push_back(web::make_atomic_model<hospital, NDTime, json>(id, "hospital", move(j)));
		}

		// NOTE: I BUILD COUPLINGS BASED ON THE GEOJSON FILES ABOVE, STUDENTS WOULD LINK MODELS THE NORMAL WAY
		for (auto& f : areas.at("features")) {
			string area_id = f.at("properties").at("dauid").get<string>();
			string hospitals = f.at("properties").at("hospitals").get<string>();
			vector<string> split = web::tools::split(web::tools::trim(hospitals), ',');

			json area = m_areas[area_id];

			json h1 = m_hospitals[split[0]];
			json h2 = m_hospitals[split[1]];
			json h3 = m_hospitals[split[2]];

			ics_TOP.push_back(web::make_IC<emergency_area_defs::out_1, hospital_defs::processor_in>(area.at("dauid").get<string>(), h1.at("index").get<string>()));
			ics_TOP.push_back(web::make_IC<emergency_area_defs::out_2, hospital_defs::processor_in>(area.at("dauid").get<string>(), h2.at("index").get<string>()));
			ics_TOP.push_back(web::make_IC<emergency_area_defs::out_3, hospital_defs::processor_in>(area.at("dauid").get<string>(), h3.at("index").get<string>()));

			ics_TOP.push_back(web::make_IC<hospital_defs::processor_out, emergency_area_defs::rejected_1>(h1.at("index").get<string>(), area.at("dauid").get<string>()));
			ics_TOP.push_back(web::make_IC<hospital_defs::processor_out, emergency_area_defs::rejected_1>(h2.at("index").get<string>(), area.at("dauid").get<string>()));
			ics_TOP.push_back(web::make_IC<hospital_defs::processor_out, emergency_area_defs::rejected_1>(h3.at("index").get<string>(), area.at("dauid").get<string>()));
		}

		// NOTE: SLIGHT DIFFERENCE HERE, THERE'S A SECOND PARAMETER TO THE CALL TO MAKE AN COUPLED/TOP MODEL. THAT PARAMETER IS A STRING IDENTIFYING THE MODEL TYPE.
	    std::shared_ptr<web::coupled<NDTime>> TOP;
		TOP	= web::make_top_model<NDTime>("gis_emergencies_1", "gis_emergencies", submodels_TOP, iports_TOP, oports_TOP, eics_TOP, eocs_TOP, ics_TOP);

		// NOTE: THERE'S A DEFAULT PATH, THIS IS ONLY TO WRITE TO ANOTHER PATH
	    web::out_messages = std::ofstream("../simulation_results/output_messages.txt");
	    web::out_state = std::ofstream("../simulation_results/state_messages.txt");

		// NOTE: CALLING THE RUNNER SAME AS USUAL, DIFFERENT NAMESPACE
	    web::runner<NDTime, web::logger_top> r(TOP, {0});

	    r.run_until(NDTime("2400:00:00:000"));

		// NOTE: CALL TO GENERATE THE OUTPUT FILES IN THE VIEWER FORMAT
	    web::output_results(TOP, "GIS-DEVS", "../simulation_results/");
	}
	catch (const exception& e) {
		cerr << e.what();
	}

    return 0;
}
