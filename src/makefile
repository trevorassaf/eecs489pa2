CC = g++

DHTDB_OBJS = main.o \
			 ServiceBuilder.o \
			 Service.o \
			 ServerBuilder.o \
			 Connection.o \
			 ltga.o \
			 imgdb.o \
			 hash.o \
			 DhtNode.o \
			 Selector.o \
			 ImageDb.o \
			 SocketException.o
DHTDB_HEADERS = ServiceBuilder.h \
			 Service.h \
			 ServerBuilder.h \
			 Connection.h \
			 ltga.h \
			 hash.h \
			 DhtNode.h \
			 Selector.h \
			 dht_packets.h \
			 ImageDb.h \
			 netimg_packets.h \
			 SocketException.h
DHTDB_EXE = dhtdb

NETIMG_OBJS = netimg.o netimglut.o
NETIMG_HEADERS = packets.h
NETIMG_EXE = netimg

CXXFLAGS = -Wall -Wno-deprecated -std=c++11
LFLAGS = $(CXXFLAGS) 

OS := $(shell uname)
ifeq ($(OS), Darwin)
  NETIMG_LIBS = -framework OpenGL -framework GLUT -lc
else
  NETIMG_LIBS = -lGL -lGLU -lglut
endif

CRYPTO_LIBS = -lssl -lcrypto
DHTDB_LIBS = $(CRYPTO_LIBS)

all: $(DHTDB_EXE) $(NETIMG_EXE)

dhtdb: $(DHTDB_OBJS) 
	$(CC) $(LFLAGS) -o $(DHTDB_EXE) $(DHTDB_OBJS) $(DHTDB_LIBS) $(NETIMG_LIBS)

main.o: main.cpp $(DHTDB_HEADERS)
	$(CC) $(CXXFLAGS) -c main.cpp

ServiceBuilder.o: ServiceBuilder.h Service.h SocketException.h
	$(CC) $(CXXFLAGS) -c ServiceBuilder.cpp

Service.o: Service.h Connection.h SocketException.h
	$(CC) $(CXXFLAGS) -c Service.cpp

ServerBuilder.o: ServerBuilder.h Connection.h SocketException.h
	$(CC) $(CXXFLAGS) -c ServerBuilder.cpp

Connection.o: Connection.h SocketException.h
	$(CC) $(CXXFLAGS) -c Connection.cpp

ltga.o: ltga.h
	$(CC) $(CXXFLAGS) -c ltga.cpp

imgdb.o: ltga.h packets.h
	$(CC) $(CXXFLAGS) -c imgdb.cpp

hash.o: hash.h netimg.h
	$(CC) $(CXXFLAGS) -c hash.cpp

DhtNode.o: DhtNode.h ServerBuilder.h ServiceBuilder.h Service.h Connection.h SocketException.h hash.h dht_packets.h netimg_packets.h Selector.h ImageDb.h ltga.h
	$(CC) $(CXXFLAGS) -c DhtNode.cpp

Selector.o: Selector.h
	$(CC) $(CXXFLAGS) -c Selector.cpp

ImageDb.o: ImageDb.h hash.h netimg_packets.h
	$(CC) $(CXXFLAGS) -c ImageDb.cpp

SocketException.o: SocketException.h
	$(CC) $(CXXFLAGS) -c SocketException.cpp

netimg: $(NETIMG_OBJS) 
	$(CC) $(LFLAGS) -o $(NETIMG_EXE) $(NETIMG_OBJS) $(NETIMG_LIBS)

netimg.o: packets.h
	$(CC) $(CXXFLAGS) -c netimg.cpp

netimglut.o: packets.h
	$(CC) $(CXXFLAGS) -c netimglut.cpp

clean:
	\rm *.o $(DHTDB_EXE) $(NETIMG_EXE)
