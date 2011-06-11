# Makefile for MAR Library

# Compilation Variables
CC=gcc

# Documentation
DOXYGEN=doxygen
DOXYFILE=Doxyfile

# Directories
SRC_DIR=src
BIN_DIR=bin

# MAR Library
MAR_LFLAGS=-shared -fPIC -Wl,-soname,$(SO_NAME)
MAR_LDFLAGS=-lvl -lconfig -larmadillo -lblas -llapack
MAR_CFLAGS=-c -Wall -pedantic -g -std=c99 -fPIC -O3 -D_XOPEN_SOURCE=700
MAR_CPPFLAGS=-c -Wall -pedantic -g -fPIC -O3 -D_XOPEN_SOURCE=700
MAR_SOURCES=camera/mar_camera.c camera/mar_v4l2_mmap_camera.c common/mar_common.c common/mar_error.c vision/mar_mser.c vision/mar_sift.c
MAR_CPP_SOURCES=augment/mar_augment.cpp
MAR_OBJECTS=$(addprefix $(BIN_DIR)/, $(MAR_SOURCES:.c=.o))
MAR_CPP_OBJECTS=$(addprefix $(BIN_DIR)/, $(MAR_CPP_SOURCES:.cpp=.o))
MAR_LIBRARY=mar
MAR_MAJOR_VERS=1
MAR_MINOR_VERS=0.1

# Install Variables
LIB_PATH=/usr/lib
INC_PATH=/usr/include/mar
INCLUDE=camera common vision augment

# Lighthouse
LIGHTHOUSE_LDFLAGS=-lGL -lGLU -lglut -L$(BIN_DIR) -lmar
LIGHTHOUSE_CFLAGS=-c -Wall -pedantic -g -std=c99 -O3
LIGHTHOUSE_SOURCES=visualizer/lighthouse.c
LIGHTHOUSE_OBJECTS=$(addprefix $(BIN_DIR)/, $(LIGHTHOUSE_SOURCES:.c=.o))
LIGHTHOUSE_EXECUTABLE=lighthouse

all: $(TARGETS) $(MAR_LIBRARY) $(LIGHTHOUSE_EXECUTABLE)

$(MAR_LIBRARY): $(MAR_OBJECTS) $(MAR_CPP_OBJECTS)
	 $(CC) $(MAR_LFLAGS) $(MAR_LDFLAGS) $(addprefix $(BIN_DIR)/, $(notdir $(MAR_OBJECTS))) $(addprefix $(BIN_DIR)/, $(notdir $(MAR_CPP_OBJECTS))) -o $(BIN_DIR)/lib$(MAR_LIBRARY).so.$(MAR_MAJOR_VERS).$(MAR_MINOR_VERS)

$(MAR_OBJECTS): $(BIN_DIR)/%.o : $(SRC_DIR)/%.c
	 $(CC) $(MAR_CFLAGS) $< -o $(BIN_DIR)/$(notdir $@)

$(MAR_CPP_OBJECTS): $(BIN_DIR)/%.o : $(SRC_DIR)/%.cpp
	 $(CC) $(MAR_CPPFLAGS) $< -o $(BIN_DIR)/$(notdir $@)

install: $(MAR_LIBRARY)
	 cp $(BIN_DIR)/lib$(MAR_LIBRARY).so.$(MAR_MAJOR_VERS).$(MAR_MINOR_VERS) $(LIB_PATH)/lib$(MAR_LIBRARY).so.$(MAR_MAJOR_VERS).$(MAR_MINOR_VERS)
	 rm -f $(LIB_PATH)/lib$(MAR_LIBRARY).so
	 ln -s $(LIB_PATH)/lib$(MAR_LIBRARY).so.$(MAR_MAJOR_VERS).$(MAR_MINOR_VERS) $(LIB_PATH)/lib$(MAR_LIBRARY).so
	 ldconfig
	 mkdir -p $(INC_PATH)
	 cp -r $(addprefix $(SRC_DIR)/, $(INCLUDE)) $(INC_PATH)

uninstall:
	 rm -f $(LIB_PATH)/lib$(MAR_LIBRARY).so.$(MAR_MAJOR_VERS).$(MAR_MINOR_VERS)
	 rm -f $(LIB_PATH)/lib$(MAR_LIBRARY).so
	 rm -rf $(addprefix $(INC_PATH)/, $(INCLUDE))

$(LIGHTHOUSE_EXECUTABLE): $(LIGHTHOUSE_OBJECTS)
	 $(CC) $(LIGHTHOUSE_LDFLAGS) $(addprefix $(BIN_DIR)/, $(notdir $(LIGHTHOUSE_OBJECTS))) -o $@

$(LIGHTHOUSE_OBJECTS): $(BIN_DIR)/%.o : $(SRC_DIR)/%.c
	 $(CC) $(LIGHTHOUSE_CFLAGS) $< -o $(BIN_DIR)/$(notdir $@)

test:	lighthouse
	./$(LIGHTHOUSE_EXECUTABLE)

documentation:
	$(DOXYGEN) $(DOXYFILE)

clean:
	rm -rf $(BIN_DIR)/*.o $(BIN_DIR)/lib$(MAR_LIBRARY).so.$(MAR_MAJOR_VERS).$(MAR_MINOR_VERS) $(LIGHTHOUSE_EXECUTABLE)

