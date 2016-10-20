#-------------------------------------------------
cflags=-std=c++11 -Wall -O2
libs=/usr/lib/libcurl.dylib

#-------------------------------------------------
exec=ethrpc
product=objs/$(exec)
dest=./$(exec)

#-------------------------------------------------
src= \
ethrpc.cpp

#-------------------------------------------------
all:
	@make $(product)
	mv $(product) $(dest)

#-------------------------------------------------
# You don't have to change below this line

objects = $(patsubst %.cpp,objs/%.o,$(src))

$(product): $(objects) $(libs)
	g++ -o $(product) $(objects) $(libs)
	@strip $(product)

$(objects): | objs

objs:
	@mkdir -p $@

objs/%.o : %.cpp
	$(CXX) $(CXXFLAGS) $(cflags) -c $< -o $@

clean:
	-@$(RM) -f $(product) objs/* 2> /dev/null
