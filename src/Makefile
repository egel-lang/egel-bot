# compiler and compile options
CC=g++ --std=c++0x
EGEL_SRC_DIR=../../egel/src/
LIBS= \
	-ldl \
	-licudata \
	-licui18n \
	-licuio \
	-licule \
	-liculx \
	-licutest \
	-licutu \
	-licuuc \
	-legel
CFLAGS=-c -Wall -pedantic -I$(EGEL_SRC_DIR)
LDFLAGS=$(LIBS) -L$(EGEL_SRC_DIR)

# source files and objects
SOURCES= \
	egel-bot.cpp 

OBJECTS=$(SOURCES:.cpp=.o)

# executable
EXECUTABLE=egel-bot

# targets
all: CC=g++ --std=c++0x
all: $(SOURCES) $(EXECUTABLE)

O3: CC=g++ --std=c++0x -O3
O3: $(SOURCES) $(EXECUTABLE)

gprof: CC=g++ --std=c++0x -O3 -pg
gprof: $(SOURCES) $(EXECUTABLE)

debug: CC=g++ --std=c++0x -g
debug: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) $(EGEL_SRC_DIR)libegel.a -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	-rm -f $(OBJECTS) $(EXECUTABLE) gmon.out massive.out out
