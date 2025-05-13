TARGET				=	plazza


CXX					=	g++
CXXFLAGS			=	-Wall -Wextra -std=gnu++20
LDFLAGS				=
INCLUDES			=	-ISource
FLAGS				=	$(CXXFLAGS) $(LDFLAGS) $(INCLUDES)

SOURCES				=	$(shell find Source -type f -iname "*.cpp")
OBJECTS				=	$(SOURCES:.cpp=.o)

all: $(TARGET)

%.o: %.cpp
	$(CXX) -c $< -o $@ $(FLAGS)

$(TARGET): $(OBJECTS)
	$(CXX) -o $(TARGET) $(OBJECTS) $(FLAGS)

clean:
	find -type f -iname "*.o" -delete
	find -type f -iname "*.d" -delete

fclean: clean
	rm -f $(TARGET)

re: fclean all
