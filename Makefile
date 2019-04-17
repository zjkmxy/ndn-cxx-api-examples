TARGETS = controller tempsensor aircon
CFLAGS = `pkg-config --cflags --libs libndn-cxx`

all: $(TARGETS)

controller: controller.cpp
	g++ -std=c++14 $< -o $@ -Wall -ggdb -O0 $(CFLAGS)

tempsensor: tempsensor.cpp
	g++ -std=c++14 $< -o $@ -Wall -ggdb -O0 $(CFLAGS)

aircon: aircon.cpp
	g++ -std=c++14 $< -o $@ -Wall -ggdb -O0 $(CFLAGS)

clean:
	rm -rf *.dSYM
	rm -rf $(TARGETS)
