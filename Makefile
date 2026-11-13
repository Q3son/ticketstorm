# TicketStorm - Sistema de sincronización de procesos (C++17, pthreads)
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic -pthread
CPPFLAGS += -Iinclude -MMD -MP
LDFLAGS  += -pthread

SRC_DIR := src
OBJ_DIR := build
SRCS    := $(wildcard $(SRC_DIR)/*.cpp)
OBJS    := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPS    := $(OBJS:.o=.d)
BIN     := ticketstorm
BIN_TSAN := ticketstorm_tsan

.PHONY: all tsan test run clean

all: $(BIN)

$(BIN): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Binario con ThreadSanitizer (detecta la carrera de la versión --no-sync).
tsan: $(BIN_TSAN)

$(BIN_TSAN): $(SRCS)
	$(CXX) -std=c++17 -g -O1 -Wall -Wextra -pthread -fsanitize=thread -Iinclude $(SRCS) -o $@

# Plan de pruebas automatizado (Tabla 4 del documento).
test: $(BIN)
	bash scripts/run_tests.sh ./$(BIN)

run: $(BIN)
	./$(BIN)

clean:
	rm -rf $(OBJ_DIR) $(BIN) $(BIN_TSAN)

-include $(DEPS)
