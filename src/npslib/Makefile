# NPS Library Build System
# For Windows with Visual C++

# Compiler
CC = cl.exe
LINK = link.exe

# Flags
CFLAGS = /EHsc /W4 /nologo /D_CRT_SECURE_NO_WARNINGS
LDFLAGS = /nologo /DEBUG

# Directories
SRC_DIR = .
OUT_DIR = .\build

# Targets
CASTANET_SRC = castanet.c
NPS_SRC = nps.c
LIB_OBJ = $(OUT_DIR)\castanet.obj $(OUT_DIR)\nps.obj
TEST_SRC = test.c

# Default target: build library
all: lib

# Create output directory
$(OUT_DIR):
    if not exist "$(OUT_DIR)" mkdir "$(OUT_DIR)"

# Build library
lib: $(OUT_DIR) $(LIB_OBJ)
    lib /OUT:$(OUT_DIR)\npslib.lib $(LIB_OBJ)
    echo Library built: $(OUT_DIR)\npslib.lib

# Compile CASTANET
$(OUT_DIR)\castanet.obj: $(SRC_DIR)\castanet.c $(SRC_DIR)\castanet.h
    $(CC) /c $(CFLAGS) /Fo:$@ $(SRC_DIR)\castanet.c

# Compile NPS
$(OUT_DIR)\nps.obj: $(SRC_DIR)\nps.c $(SRC_DIR)\nps.h $(SRC_DIR)\castanet.h
    $(CC) /c $(CFLAGS) /Fo:$@ $(SRC_DIR)\nps.c

# Build test executable
test: lib
    $(CC) $(CFLAGS) /Fe:$(OUT_DIR)\nps_test.exe $(SRC_DIR)\$(TEST_SRC) /link $(OUT_DIR)\npslib.lib ws2_32.lib

# Clean
clean:
    if exist $(OUT_DIR)\*.obj del $(OUT_DIR)\*.obj
    if exist $(OUT_DIR)\*.lib del $(OUT_DIR)\*.lib
    if exist $(OUT_DIR)\*.exe del $(OUT_DIR)\*.exe
    echo Clean complete.

# Run test
runtest: test
    $(OUT_DIR)\nps_test.exe

.PHONY: all lib test clean runtest
