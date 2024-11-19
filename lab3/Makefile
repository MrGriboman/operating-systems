# Compiler and flags
CC = gcc
CFLAGS = -Wall -I .

# Detect platform
ifeq ($(OS),Windows_NT)
    PLATFORM = WINDOWS
else
    PLATFORM = POSIX
endif

# Default target
all: t_log $(if $(filter WINDOWS, $(PLATFORM)),process)

# Target for t_log
t_log: logger.c
	$(CC) $(CFLAGS) logger.c -o t_log

# Platform-specific rules
ifeq ($(PLATFORM),WINDOWS)
process: process1.exe process2.exe

process1.exe: proc1.c
	$(CC) $(CFLAGS) proc1.c -o process1.exe

process2.exe: proc2.c
	$(CC) $(CFLAGS) proc2.c -o process2.exe
endif


# Debugging platform info
platform:
	@echo "Detected platform: $(PLATFORM)"
