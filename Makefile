.PHONY: clean all examples
.SECONDARY:

top:=$(dir $(realpath $(lastword $(MAKEFILE_LIST))))

examples:=ex1-parse ex1-run ex2-parse ex2-run ex3-parse ex3-run ex4-run ex5-run
all:: unit $(examples)

test-src:=unit.cc test_sink.cc test_maybe.cc test_option.cc test_state.cc test_parse.cc test_parsers.cc test_saved_options.cc test_run.cc

all-src:=$(test-src) $(patsubst %, %.cc, $(examples))
all-obj:=$(patsubst %.cc, %.o, $(all-src))

gtest-top:=$(top)test/googletest/googletest
gtest-inc:=$(gtest-top)/include
gtest-src:=$(gtest-top)/src/gtest-all.cc

vpath %.cc $(top)test
vpath %.cc $(top)ex

OPTFLAGS?=-O2 -fsanitize=address -march=native
CXXFLAGS+=$(OPTFLAGS) -MMD -MP -std=c++14 -pedantic -Wall -Wextra -g -pthread
CPPFLAGS+=-isystem $(gtest-inc) -I $(top)include

depends:=$(patsubst %.cc, %.d, $(all-src)) gtest.d
-include $(depends)

gtest.o: CPPFLAGS+=-I $(gtest-top)
gtest.o: ${gtest-src}
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c $<

test-obj:=$(patsubst %.cc, %.o, $(test-src))
unit: $(test-obj) gtest.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

ex1-parse: ex1-parse.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

ex1-run: ex1-run.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

ex2-parse: ex2-parse.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

ex2-run: ex2-run.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

ex3-parse: ex3-parse.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

ex3-run: ex3-run.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

ex4-run: ex4-run.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

ex5-run: ex5-run.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(all-obj)

realclean: clean
	rm -f unit $(examples) gtest.o $(depends)
