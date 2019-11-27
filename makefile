APPNAME=geiger.elf
INCL=-I/usr/include -I/usr/lib/arm-linux-gnueabihf
LINK=-L/usr/lib -L/usr/local/lib -ludev -lpthread -lmysqlclient -lcurl -lz
CXX=g++ 
CXXFLAGS = -pthread -DAPP_VERSION="4.0" $(INCL)

#debugging or not (make DEBUG=1)
ifeq (1,$(DEBUG))
  CXXFLAGS += -g -DDEBUG -O0
else
  CXXFLAGS += -O3
endif

#list all source files and create object files for them
SRCS := ${wildcard src/*.cpp}
OBJS := $(subst src/,obj/,${SRCS:.cpp=.o})

#create application
all: $(APPNAME)

# link
$(APPNAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LINK) $(OBJS) -o bin/$(APPNAME)

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

# compile and generate dependency info;
#thanks to http://scottmcpeak.com/autodepend/autodepend.html
obj/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c  src/$*.cpp -o obj/$*.o
	$(CXX) $(CXXFLAGS) -MM -MT obj/$*.o src/$*.cpp -MF obj/$*.d
	@cp -f obj/$*.d obj/$*.d.tmp
	@sed -e 's/.*://' -e 's/\\$$//' < obj/$*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> obj/$*.d
	@rm -f obj/$*.d.tmp
	
# remove compilation files
clean:
	rm -f bin/$(APPNAME) obj/*.o obj/*.d