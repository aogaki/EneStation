COMP_NAME = Monitor

all: $(COMP_NAME)Comp

SRCS += $(COMP_NAME).cpp
SRCS += $(COMP_NAME)Comp.cpp

LDFLAGS += $(shell root-config --glibs) -lRHTTP -lSpectrum -lmongocxx -lbsoncxx
CPPFLAGS += -I$(shell root-config --incdir) -std=c++11 -I/usr/local/include/mongocxx/v_noabi -I/usr/local/include/bsoncxx/v_noabi

# sample install target
#
# MODE = 0755
# BINDIR = /tmp/mybinary
#
# install: $(COMP_NAME)Comp
#	mkdir -p $(BINDIR)
#	install -m $(MODE) $(COMP_NAME)Comp $(BINDIR)

include /usr/share/daqmw/mk/comp.mk
