#ifndef BOOST_SIMULATION_MESSAGE_HPP
#define BOOST_SIMULATION_MESSAGE_HPP

#include <assert.h>
#include <iostream>
#include <string>

#include <web/output/message_type.hpp>

using namespace std;
using namespace cadmium;

/*******************************************/
/**************** Message_t ****************/
/*******************************************/
struct Message_t{
  Message_t(): packet(0), bit(0){}

  Message_t(int i_packet, int i_bit)
   :packet(i_packet), bit(i_bit){}

  	int   packet;
  	int   bit;

	static cadmium::web::output::message_type get_message_type() {
		vector<string> fields({ "packet", "bit" });
		string description = "packet is ??? and bit is ???";

		return cadmium::web::output::message_type("o_packet", fields, description);
	}
};

ostream& operator<<(ostream& os, const Message_t& msg) {
  os << msg.packet << "," << msg.bit;
  return os;
}

istream& operator>> (istream& is, Message_t& msg) {
  is >> msg.packet;
  is >> msg.bit;
  return is;
}

#endif // BOOST_SIMULATION_MESSAGE_HPP
