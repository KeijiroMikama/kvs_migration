CXX = g++

CXXFLAGS += -Wall -g -DON_DCPMM

ifdef ON_RELOCATION
CXXFLAGS += -DRELOCATION
endif

ifdef ON_ONLY_PMEM
CXXFLAGS += -DONLY_PMEM
else
ifdef ON_ONLY_DISK
CXXFLAGS += -DONLY_DISK
else
CXXFLAGS += -DMIXING
endif
endif

ifdef ON_DCPMM
CXXFLAGS += -DDCPMM
endif

ifdef ON_IDLE
CXXFLAGS += -DIDLE
endif

TARGET = main

SRCS = main.cc db.cc sst.cc workload.cc socket.cc debug.cc

LDFLAGS = -L/usr/local/lib -lrocksdb -lpthread -lz -lsnappy -lzstd -llz4 -lbz2 -lpmem -lpmemobj -ldl

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS) $(LDFLAGS)

clean:
	rm -rf $(TARGET)
