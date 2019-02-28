.PHONY: clean all
.SECONDARY:

top:=$(dir $(realpath $(lastword $(MAKEFILE_LIST))))

all:: demo-tinyopt demo-miniopt unit

demo-tinyopt-src:=demo-tinyopt.cc
demo-tinyopt-obj:=$(patsubst %.cc, %.o, $(demo-tinyopt-src))

demo-miniopt-src:=demo-miniopt.cc
demo-miniopt-obj:=$(patsubst %.cc, %.o, $(demo-miniopt-src))

test-src:=unit.cc test_sink.cc test_maybe.cc test_option.cc test_state.cc test_parse.cc
test-obj:=$(patsubst %.cc, %.o, $(test-src))

depends:=$(patsubst %.cc, %.d, $(demo-miniopt-src) $(demo-tinyopt-src) $(test-src)) gtest.d

gtest-top:=$(top)test/googletest/googletest
gtest-inc:=$(gtest-top)/include
gtest-src:=$(gtest-top)/src/gtest-all.cc

vpath %.cc $(top)test
vpath %.cc $(top)demo

#OPTFLAGS?=-O3 -march=native
OPTFLAGS?=-O0 -fsanitize=address
CXXFLAGS+=$(OPTFLAGS) -MMD -MP -std=c++14 -g -pthread
CPPFLAGS+=-isystem $(gtest-inc) -I $(top)include

-include $(depends)

gtest.o: CPPFLAGS+=-I $(gtest-top)
gtest.o: ${gtest-src}
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c $<

unit: $(test-obj) gtest.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

demo-miniopt: $(demo-miniopt-obj)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

demo-tinyopt: $(demo-tinyopt-obj)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(demo-tinyopt-obj) $(demo-miniopt-obj) $(test-obj)

realclean: clean
	rm -f demo-tinyopt demo-miniopt unit gtest.o $(depends)
