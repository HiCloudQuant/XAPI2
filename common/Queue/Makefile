include ../../Makefile.global

LFLAGS=-Wl,-rpath,\$$ORIGIN -ldl
ALL=libQueue.so 

all: $(ALL)

libQueue.so: main.o  MsgQueue.o  RemoteQueue.o  $(DEPS)
	$(CXX) --shared $(IFLAGS) $(CFLAGS) -O3 -I../CTP_Quote -o $@ $^


