CC      = $(CROSS_COMPILE)g++
CFLAGS  +=  -c -Wall $(EXTRAFLAGS)
LDFLAGS +=  -lcurl
DZ_OBJ  = $(patsubst %.cpp,%.o,$(wildcard domoticzclient/*.cpp)) $(patsubst %.cpp,%.o,$(wildcard domoticzclient/jsoncpp/*.cpp))
DEPS    = $(wildcard app/*.h) $(wildcard domoticzclient/*.h) $(wildcard domoticzclient/jsoncpp/*.h)


all: dzEvo

dzEvo: app/dzEvo.o  $(DZ_OBJ)
	$(CC) app/dzEvo.o  $(DZ_OBJ) $(LDFLAGS) -o dzEvo


%.o: %.cpp $(DEP)
	$(CC) $(CFLAGS) $(EXTRAFLAGS) $< -o $@

distclean: clean

clean:
	rm -f  $(DZ_OBJ) $(wildcard app/*.o)  dzEvo

