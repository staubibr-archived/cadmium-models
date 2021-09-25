//Cadmium Simulator headers
#include <iostream>
#include <string>
#include <vector>
#include <NDTime.hpp>
#include <chrono>

#include <web/web_ie_stream.hpp>
#include <web/web_logger.hpp>
#include <web/web_ports.hpp>
#include <web/web_model.hpp>
#include <web/web_results.hpp>

//Messages structures, Atomic model headers
#include "../data_structures/message.hpp"
#include "../atomics/subnet.hpp"
#include "../atomics/sender.hpp"
#include "../atomics/receiver.hpp"

using TIME = NDTime;

/***** Define input port for coupled models *****/
struct inp_control : public web::in_port<int>{};
struct inp_1 : public web::in_port<Message_t>{};
struct inp_2 : public web::in_port<Message_t>{};

/***** Define output ports for coupled model *****/
struct outp_ack : public web::out_port<int>{};
struct outp_1 : public web::out_port<Message_t>{};
struct outp_2 : public web::out_port<Message_t>{};
struct outp_pack : public web::out_port<int>{};

/****** Input Reader atomic model declaration *******************/
template<typename T>
class InputReader_Int : public web::iestream_input<int,T> {
public:
    InputReader_Int() = default;
    InputReader_Int(const char* file_path) : web::iestream_input<int,T>(file_path) {}
};



int main(int argc, char ** argv) {

	if (argc < 2) {
		cout << "Program used with wrong parameters. The program must be invoked as follow:";
		cout << argv[0] << " path to the input file " << endl;
		return 1;
	}

	/****** Input Reader atomic model instantiation *******************/
	string input = argv[1];
	const char * i_input = input.c_str();
	shared_ptr<web::model> input_reader = web::make_atomic_model<InputReader_Int, TIME, const char* >("input_reader_1", "input_reader",  move(i_input));

	/****** Sender atomic model instantiation *******************/
	shared_ptr<web::model> sender1 = web::make_atomic_model<Sender, TIME>("sender1", "sender");

	/****** Receiver atomic model instantiation *******************/
	shared_ptr<web::model> receiver1 = web::make_atomic_model<Receiver, TIME>("receiver1", "receiver");

	/****** Subnet atomic models instantiation *******************/
	shared_ptr<web::model> subnet1 = web::make_atomic_model<Subnet, TIME>("subnet1", "subnet");
	shared_ptr<web::model> subnet2 = web::make_atomic_model<Subnet, TIME>("subnet2", "subnet");

	/*******NETWORKS COUPLED MODEL********/
	web::Ports iports_Network = {typeid(inp_1),typeid(inp_2)};
	web::Ports oports_Network = {typeid(outp_1),typeid(outp_2)};
	web::Models submodels_Network = {subnet1, subnet2};

	web::EICs eics_Network = {
		web::make_EIC<inp_1, Subnet_defs::in>("subnet1"),
		web::make_EIC<inp_2, Subnet_defs::in>("subnet2")
	};

	web::EOCs eocs_Network = {
		web::make_EOC<Subnet_defs::out,outp_1>("subnet1"),
		web::make_EOC<Subnet_defs::out,outp_2>("subnet2")
	};

	web::ICs ics_Network = {};

	shared_ptr<web::coupled<TIME>> NETWORK;
	NETWORK = make_shared<web::coupled<TIME>>(
		"Network1", "Network", submodels_Network, iports_Network, oports_Network, eics_Network, eocs_Network, ics_Network
	);

	/*******ABP SIMULATOR COUPLED MODEL********/
	web::Ports iports_ABP = {typeid(inp_control)};
	web::Ports oports_ABP = {typeid(outp_ack),typeid(outp_pack)};
	web::Models submodels_ABP = {sender1, receiver1, NETWORK};

	web::EICs eics_ABP = {
		web::make_EIC<inp_control, Sender_defs::controlIn>("sender1")
	};

	web::EOCs eocs_ABP = {
		web::make_EOC<Sender_defs::packetSentOut,outp_pack>("sender1"),
		web::make_EOC<Sender_defs::ackReceivedOut,outp_ack>("sender1")
	};

	web::ICs ics_ABP = {
		web::make_IC<Sender_defs::dataOut, inp_1>("sender1","Network1"),
		web::make_IC<outp_2, Sender_defs::ackIn>("Network1","sender1"),
		web::make_IC<Receiver_defs::out, inp_2>("receiver1","Network1"),
		web::make_IC<outp_1, Receiver_defs::in>("Network1","receiver1")
	};

	shared_ptr<web::coupled<TIME>> ABP = make_shared<web::coupled<TIME>>(
		"ABP", "ABP", submodels_ABP, iports_ABP, oports_ABP, eics_ABP, eocs_ABP, ics_ABP
	);

	/*******TOP COUPLED MODEL********/
	web::Ports iports_TOP = {};
	web::Ports oports_TOP = {typeid(outp_pack),typeid(outp_ack)};
	web::Models submodels_TOP = {input_reader, ABP};
	web::EICs eics_TOP = {};

	web::EOCs eocs_TOP = {
		web::make_EOC<outp_pack,outp_pack>("ABP"),
		web::make_EOC<outp_ack,outp_ack>("ABP")
	};

	web::ICs ics_TOP = {
		web::make_IC<web::iestream_input_defs<int>::out, inp_control>("input_reader_1","ABP")
	};

	shared_ptr<web::coupled<TIME>> TOP = web::make_top_model<TIME>("TOP", "top", submodels_TOP, iports_TOP, oports_TOP, eics_TOP, eocs_TOP, ics_TOP);

	web::runner<TIME, web::logger_top> r(TOP, {0});

	r.run_until_passivate();

	web::output_results(TOP, "DEVS", "../simulation_results/");

	return 0;
}
