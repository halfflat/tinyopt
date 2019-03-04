.PHONY: clean all examples
.SECONDARY:

top:=$(dir $(realpath $(lastword $(MAKEFILE_LIST))))

examples:=ex1-tiny ex1-smol ex2-tiny ex2-smol ex4-smol
all:: unit $(examples)

test-src:=unit.cc test_sink.cc test_maybe.cc test_option.cc test_state.cc test_parse.cc test_parsers.cc test_saved_options.cc test_run.cc

all-src:=$(test-src) $(patsubst %, %.cc, $(examples))
all-obj:=$(patsubst %.cc, %.o, $(all-src))

gtest-top:=$(top)test/googletest/googletest
gtest-inc:=$(gtest-top)/include
gtest-src:=$(gtest-top)/src/gtest-all.cc

vpath %.cc $(top)test
vpath %.cc $(top)ex

#OPTFLAGS?=-O3 -march=native
OPTFLAGS?=-O0 -fsanitize=address
CXXFLAGS+=$(OPTFLAGS) -MMD -MP -std=c++14 -g -pthread
CPPFLAGS+=-isystem $(gtest-inc) -I $(top)include

depends:=$(patsubst %.cc, %.d, $(all-src)) gtest.d
-include $(depends)

gtest.o: CPPFLAGS+=-I $(gtest-top)
gtest.o: ${gtest-src}
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c $<

test-obj:=$(patsubst %.cc, %.o, $(test-src))
unit: $(test-obj) gtest.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

ex1-tiny: ex1-tiny.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

ex1-smol: ex1-smol.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

ex2-tiny: ex2-tiny.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

ex2-smol: ex2-smol.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

ex4-smol: ex4-smol.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(all-obj)

realclean: clean
	rm -f demo-tinyopt demo-miniopt unit $(examples) gtest.o $(depends)
