CXX = g++ -std=c++17
CXXFLAGS = -O3 -march=native -DNDEBUG -I/usr/include/SFML
LDFLAGS = -L/usr/lib/x86_64-linux-gnu -lsfml-graphics -lsfml-window -lsfml-system

SRC = $(wildcard src/*.cpp)
OBJ = $(SRC:.cpp=.o)
TARGET = ./bin/Mandelbrot

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) -o $@ -c $< $(CXXFLAGS)

clean:
	rm -f $(OBJ) $(TARGET)

discard:
	rm -f frames/*.png frames/*.jpg