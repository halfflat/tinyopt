.PHONY: clean all
.SECONDARY:

top:=$(dir $(realpath $(lastword $(MAKEFILE_LIST))))

all:: demo unit

demo-src:=demo.cc
demo-obj:=$(patsubst %.cc, %.o, $(demo-src))

test-src:=unit.cc test_sink.cc test_maybe.cc
test-obj:=$(patsubst %.cc, %.o, $(test-src))

depends:=$(patsubst %.cc, %.d, $(demo-src) $(test-src))

gtest-top:=$(top)test/googletest/googletest
gtest-inc:=$(gtest-top)/include
gtest-src:=$(gtest-top)/src/gtest-all.cc

vpath %.cc $(top)test
vpath %.cc $(top)demo

OPTFLAGS?=-O3 -march=native
CXXFLAGS+=$(OPTFLAGS) -MMD -MP -std=c++14 -g -pthread
CPPFLAGS+=-isystem $(gtest-inc) -I $(top)include

-include $(depends)

gtest.o: CPPFLAGS+=-I $(gtest-top)
gtest.o: ${gtest-src}
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c $<

unit: $(test-obj) gtest.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

demo: $(demo-obj)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(demo-obj) $(test-obj)

realclean: clean
	rm -f demo unit gtest.o $(depends)
