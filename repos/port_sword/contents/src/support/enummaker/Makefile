TARGET = enummaker

OFILES = enummaker.o

.cpp.o:
	g++ -O3 -c $< -o $@

all: $(TARGET)

$(TARGET): $(OFILES)
	g++ -O3 -o $(TARGET) $(OFILES)

clean:
	rm -f *.o
	rm -f $(TARGET)
