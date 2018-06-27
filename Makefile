.PHONY: clean all
.SECONDARY:

top:=$(dir $(realpath $(lastword $(MAKEFILE_LIST))))

all:: demo unit

demo-src:=demo.cc
demo-obj:=$(patsubst %.cc, %.o, $(demo-src))

test-src:=test_adaptor.cc
test-obj:=$(patsubst %.cc, %.o, $(test-src))

depends:=$(patsubst %.cc, %.d, $(demo-src) $(test-src))

gtest-inc:=$(top)test/googletest/googletest/include
gtest-src:=$(top)test/googletest/googletest/src/gtest-all.cc

vpath %.cc $(top)test
vpath %.cc $(top)demo

OPTFLAGS?=-O3 -march=native
CXXFLAGS+=$(OPTFLAGS) -MMD -MP -std=c++14 -g
CPPFLAGS+=-isystem ${gtest-inc} -I $(top)include

-include $(depends)

gtest.o: ${gtest-src}

unit: $(test-obj) gtest.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

demo: $(demo-obj)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(demo-obj) $(test-obj) gtest.o

realclean: clean
	rm -f demo unit $(depends)
