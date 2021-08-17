//Cadmium Simulator headers
#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/dynamic_model.hpp>
#include <cadmium/modeling/dynamic_model_translator.hpp>
#include <cadmium/engine/pdevs_dynamic_runner.hpp>
#include <cadmium/logger/common_loggers.hpp>
#include <cadmium/basic_model/pdevs/iestream.hpp>
#include <NDTime.hpp>
#include <iostream>
#include <string>

#include "../data_structures/emergency.hpp"
#include "../atomics/hospital.hpp"

using namespace std;
using namespace cadmium;
using namespace cadmium::basic_models::pdevs;

using TIME = NDTime;

/***** Define input port for coupled models *****/
// none for this test

/***** Define output ports for coupled model *****/
struct processor_out: public out_port<Emergency_t>{};

/****** Emergency Input Reader atomic model declarations *******************/
template<typename T>
class InputReader_t : public iestream_input<Emergency_t,T> {
public:
    InputReader_t () = default;
    InputReader_t (const char* file_path) : iestream_input<Emergency_t, T>(file_path) {}
};

int main(){
    /****** Emergency Input Reader atomic model instantiation *******************/
    const char * i_input_data = "../input_data/Hospital_Test.txt";
    shared_ptr<dynamic::modeling::model> input_reader = dynamic::translate::make_dynamic_atomic_model<InputReader_t, TIME, const char*>("input_reader", move(i_input_data));

    /****** Emergency Processor atomic model instantiation *******************/
	struct Hospital_params p;

	p.id = 1;
	p.name = "hospital_1";
	p.capacity = 10;
	p.rate = 3;

	shared_ptr<dynamic::modeling::model> processor = dynamic::translate::make_dynamic_atomic_model<Hospital, TIME, Hospital_params>("hospital_1", move(p));

    /*******TOP MODEL********/
    dynamic::modeling::Ports iports_TOP = {};
    dynamic::modeling::Ports oports_TOP = {typeid(processor_out)};
    dynamic::modeling::Models submodels_TOP = {input_reader, processor};
    dynamic::modeling::EICs eics_TOP = {};

    dynamic::modeling::EOCs eocs_TOP = {
         dynamic::translate::make_EOC<Hospital_defs::processor_out, processor_out>("hospital_1")
    };

    dynamic::modeling::ICs ics_TOP = {
         dynamic::translate::make_IC<iestream_input_defs<Emergency_t>::out, Hospital_defs::processor_in>("input_reader","hospital_1"),
    };

    shared_ptr<dynamic::modeling::coupled<TIME>> TOP;

    TOP = make_shared<dynamic::modeling::coupled<TIME>>("TOP", submodels_TOP, iports_TOP, oports_TOP, eics_TOP, eocs_TOP, ics_TOP);

    /*************** Loggers *******************/
    static ofstream out_messages("../simulation_results/Hospital_Test_Outputs.txt");
    struct oss_sink_messages{
        static ostream& sink(){
            return out_messages;
        }
    };
    static ofstream out_state("../simulation_results/Hospital_Test_State.txt");
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
    r.run_until(NDTime("312:00:00:000"));
    return 0;
}
