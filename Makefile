TARGET				=	plazza


CXX					=	g++
CXXFLAGS			=	-Wall -Wextra -std=gnu++20
LDFLAGS				=
INCLUDES			=	-IPlazza
FLAGS				=	$(CXXFLAGS) $(LDFLAGS) $(INCLUDES)

SOURCES				=	$(shell find Plazza -type f -iname "*.cpp")
OBJECTS				=	$(SOURCES:.cpp=.o)

all: $(TARGET)

%.o: %.cpp
	$(CXX) -c $< -o $@ $(FLAGS)

$(TARGET): $(OBJECTS)
	$(CXX) -o $(TARGET) $(OBJECTS) $(FLAGS)

bonus: LDFLAGS += -lsfml-graphics -lsfml-window -lsfml-system -DPLAZZA_BONUS
bonus: re

clean:
	find -type f -iname "*.o" -delete
	find -type f -iname "*.d" -delete

fclean: clean
	rm -f $(TARGET)

re: fclean all
