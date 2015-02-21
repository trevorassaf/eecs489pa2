CC = g++
P2PDB_OBJS = main.o \
			 ServiceBuilder.o \
			 Service.o \
			 ServerBuilder.o \
			 Connection.o \
			 P2pTable.o \
			 ImageNetwork.o \
			 ltga.o \
			 imgdb.o \
			 SocketException.o
P2PDB_HEADERS = ServiceBuilder.h \
			 Service.h \
			 ServerBuilder.h \
			 Connection.h \
			 P2pTable.h \
			 ImageNetwork.h \
			 ltga.h \
			 packets.h \
			 SocketException.h
P2PDB_EXE = p2pdb
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

all: $(P2PDB_EXE) $(NETIMG_EXE)

p2pdb: $(P2PDB_OBJS) 
	$(CC) $(LFLAGS) -o $(P2PDB_EXE) $(P2PDB_OBJS)

main.o: main.cpp $(P2PDB_HEADERS)
	$(CC) $(CXXFLAGS) -c main.cpp

ServiceBuilder.o: ServiceBuilder.h Service.h SocketException.h
	$(CC) $(CXXFLAGS) -c ServiceBuilder.cpp

Service.o: Service.h Connection.h SocketException.h
	$(CC) $(CXXFLAGS) -c Service.cpp

ServerBuilder.o: ServerBuilder.h Connection.h SocketException.h
	$(CC) $(CXXFLAGS) -c ServerBuilder.cpp

Connection.o: Connection.h SocketException.h
	$(CC) $(CXXFLAGS) -c Connection.cpp

P2pTable.o: P2pTable.h Connection.h
	$(CC) $(CXXFLAGS) -c P2pTable.cpp

ImageNetwork.o: ImageNetwork.h Service.h Connection.h P2pTable.h
	$(CC) $(CXXFLAGS) -c ImageNetwork.cpp

ltga.o: ltga.h
	$(CC) $(CXXFLAGS) -c ltga.cpp

imgdb.o: ltga.h packets.h
	$(CC) $(CXXFLAGS) -c imgdb.cpp

SocketException.o: SocketException.h
	$(CC) $(CXXFLAGS) -c SocketException.cpp

netimg: $(NETIMG_OBJS) 
	$(CC) $(LFLAGS) -o $(NETIMG_EXE) $(NETIMG_OBJS) $(NETIMG_LIBS)

netimg.o: packets.h
	$(CC) $(CXXFLAGS) -c netimg.cpp

netimglut.o: packets.h
	$(CC) $(CXXFLAGS) -c netimglut.cpp

clean:
	\rm *.o $(P2PDB_EXE) $(NETIMG_EXE)
