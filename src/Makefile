# compiler and compile options
CC=g++ --std=c++20
LIBS= \
	-ldl \
	-legel \
	-lfmt \
	-lstdc++fs \
	$(shell pkg-config --libs --cflags icu-uc icu-io)
CFLAGS=-c -Wall -pedantic $(LIBS)
LDFLAGS=$(LIBS)

# source files and objects
SOURCES= \
	egel-bot.cpp 

OBJECTS=$(SOURCES:.cpp=.o)

# executable
EXECUTABLE=egel-bot

# targets
all: CC=g++ --std=c++20
all: $(SOURCES) $(EXECUTABLE)

O3: CC=g++ --std=c++20 -O3
O3: $(SOURCES) $(EXECUTABLE)

gprof: CC=g++ --std=c++20 -O3 -pg
gprof: $(SOURCES) $(EXECUTABLE)

debug: CC=g++ --std=c++20 -g
debug: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	-rm -f $(OBJECTS) $(EXECUTABLE) gmon.out massive.out out
