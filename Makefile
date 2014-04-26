TARGET   := ./dstatus
CXXFLAGS := -std=c++11 -pedantic-errors -ggdb -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-used-but-marked-unused -Wno-exit-time-destructors -Wno-padded -Wno-missing-prototypes -Wno-global-constructors
CXX      := clang++
LIBS     := -lc++ -lX11 -lsensors
EXT      := cpp
BUILDDIR := build

override BUILDDIR := $(strip $(BUILDDIR))
SOURCES  := $(wildcard *.$(EXT))
OBJECTS  := $(patsubst %.$(EXT), $(BUILDDIR)/%.o, $(SOURCES))
DEPS     := $(patsubst %.$(EXT), $(BUILDDIR)/%.dep, $(SOURCES))

.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJECTS) $(DEPS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(LIBS)

ifneq ($(MAKECMDGOALS), clean)
-include $(DEPS)
endif

$(OBJECTS): $(BUILDDIR)/%.o: %.$(EXT) $(BUILDDIR)/%.dep $(BUILDDIR)/.tag
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(DEPS): $(BUILDDIR)/%.dep: %.$(EXT) $(BUILDDIR)/.tag
	mkdir -p $(dir $(@))
	$(CXX) $(CXXFLAGS) -MM $< -MT $@ -MT $(<:.$(EXT)=.o) -o $@

%.tag:
	mkdir -p $(dir $(@))
	touch $@

.PHONY: clean
clean:
	$(RM) -r $(BUILDDIR) $(TARGET)
