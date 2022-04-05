CXXFLAGS=-std=c++20 -Wall -Wextra -Ofast

all: 1 pic

TBA_FILES = ../tba/util.o ../tba/db.o ../tba/curl.o ../tba/data.o ../tba/rapidjson.o 


1: strategy.o scout.o 1.o $(TBA_FILES) opr.o qr_solve.o r8lib.o util.o input_data.o valor.o tba.o
	$(CXX) $(CXXFLAGS) $^ -lsqlite3 -lcurl -o $@

pic: pic.o $(TBA_FILES) util.o tba.o opr.o qr_solve.o r8lib.o scout.o input_data.o strategy.o
	$(CXX) $(CXXFLAGS) $^ -lsqlite3 -lcurl -o $@

.PHONY: clean
clean:
	rm -f 1 pic *.o *.html $(TBA_FILES)
