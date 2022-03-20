#ifndef TBA_H
#define TBA_H

#include "input_data.h"

std::variant<std::map<Team,Robot_capabilities>,std::string> from_tba(
	std::string const&,
	std::string const&,
	tba::Event_key const&
);

#endif
