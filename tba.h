#ifndef TBA_H
#define TBA_H

#include "input_data.h"
#include "../tba/db.h"

struct TBA_setup{
        std::string auth_key_path="../tba/auth_key";
        std::string cache_path="../tba/cache.db";
};

tba::Cached_fetcher tba_fetcher(TBA_setup const&);

std::variant<std::map<Team,Robot_capabilities>,std::string> from_tba(
	TBA_setup const&,
	tba::Event_key const&
);

#define TBA_EXPLICIT(X)\
	X(Team,team)\
	X(int,match)\
	X(tba::Alliance_color,alliance)\
	X(bool,taxi)\
	X(Endgame,endgame)

struct TBA_explicit{
	TBA_EXPLICIT(INST)
};

std::ostream& operator<<(std::ostream&,TBA_explicit const&);

std::vector<TBA_explicit> tba_by_match(TBA_setup const&,tba::Event_key const&);

Team team_number(tba::Team_key const&);

#endif
