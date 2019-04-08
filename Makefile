TARGETS = controller tempsensor aircon

all: $(TARGETS)

controller: controller.cpp
	g++ -std=c++14 $< -o $@ -Wall -ggdb -O0 -lndn-cxx -lboost_system

tempsensor: tempsensor.cpp
	g++ -std=c++14 $< -o $@ -Wall -ggdb -O0 -lndn-cxx -lboost_system

aircon: aircon.cpp
	g++ -std=c++14 $< -o $@ -Wall -ggdb -O0 -lndn-cxx -lboost_system

clean:
	rm -rf *.dSYM
	rm -rf $(TARGETS)
