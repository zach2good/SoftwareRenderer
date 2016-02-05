SYSCONF_LINK = g++
CPPFLAGS     = -std=c++14  $(SDL_INCLUDE)
LDFLAGS      = -lm -lmingw32 -mwindows -mconsole  $(SDL_LIB)
LIBS         =
SDL_LIB		 = -LC:/SDL2-2.0.4/i686-w64-mingw32/lib -lSDL2main -lSDL2
SDL_INCLUDE  = -IC:/SDL2-2.0.4/include

DESTDIR = ./
TARGET  = main

OBJECTS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))

all: $(DESTDIR)$(TARGET)

$(DESTDIR)$(TARGET): $(OBJECTS)
	$(SYSCONF_LINK) -Wall $(LDFLAGS) -o $(DESTDIR)$(TARGET) $(OBJECTS) $(LIBS) $(SDL_INCLUDE) $(SDL_LIB)

$(OBJECTS): %.o: %.cpp
	$(SYSCONF_LINK) -Wall $(CPPFLAGS) -c $(CFLAGS) $< -o $@

clean:
	-rm -f $(OBJECTS)
	-rm -f $(TARGET)
	-rm -f *.tga