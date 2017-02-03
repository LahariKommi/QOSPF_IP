.PHONY: all dijkstra congestion-reporter

all: dijkstra congestion_reporter

congestion_reporter: congestion_reporter.c
	gcc congestion_reporter.c -o congestion_reporter

dijkstra: dijkstra.cpp
	g++ dijkstra.cpp -o dijkstra

clean:
	-rm -f congestion_reporter
	-rm -f dijkstra
 
