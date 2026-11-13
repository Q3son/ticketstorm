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
# En Windows (MinGW/MSYS2) el enlazador añade .exe; se refleja en el nombre
# del objetivo para que make no reenlace en cada invocación.
ifeq ($(OS),Windows_NT)
EXE := .exe
endif
BIN     := ticketstorm$(EXE)
BIN_TSAN := ticketstorm_tsan$(EXE)
UNIT_BIN := unit_tests$(EXE)
TEST_SRCS := $(wildcard tests/*.cpp)
LIB_OBJS := $(filter-out $(OBJ_DIR)/main.o,$(OBJS))

.PHONY: all tsan unit test run clean

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

# Pruebas unitarias de TicketInventory y Logger.
$(UNIT_BIN): $(LIB_OBJS) $(TEST_SRCS)
	$(CXX) $(CXXFLAGS) -Iinclude $(TEST_SRCS) $(LIB_OBJS) $(LDFLAGS) -o $@

unit: $(UNIT_BIN)
	./$(UNIT_BIN)

# Plan de pruebas automatizado (Tabla 4 del documento) + pruebas unitarias.
test: $(BIN) unit
	bash scripts/run_tests.sh ./$(BIN)

run: $(BIN)
	./$(BIN)

clean:
	rm -rf $(OBJ_DIR) $(BIN) $(BIN_TSAN) $(UNIT_BIN)

-include $(DEPS)
