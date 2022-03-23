CXXFLAGS=-std=c++20 -Wall -Wextra -Werror

all: 1

1: scout.o 1.o ../tba/util.o ../tba/db.o ../tba/curl.o ../tba/data.o ../tba/rapidjson.o opr.o qr_solve.o r8lib.o util.o input_data.o valor.o tba.o
	$(CXX) $(CXXFLAGS) $^ -lsqlite3 -lcurl -o $@

.PHONY: clean
clean:
	rm -f 1 *.o *.html
