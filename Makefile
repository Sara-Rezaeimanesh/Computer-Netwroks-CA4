CC = g++ -pthread
BUILD_DIR = build
SRC_DIR = src
INCLUDE_DIR = include
CONFIGURATION_DIR = configuration
CFLAGS = -std=c++11 -Wall -Werror -I$(INCLUDE_DIR)

# OBJECTS = \
# 	$(BUILD_DIR)/pc_A.o \
# 	$(BUILD_DIR)/pc_B.o \
# 	$(BUILD_DIR)/router.o \

PC_A_SensitivityList = \
	$(SRC_DIR)/pc_A.cpp \
	# $(INCLUDE_DIR)/User.hpp \
	# $(INCLUDE_DIR)/IncludeAndDefine.hpp \

PC_B_SensitivityList = \
	$(SRC_DIR)/pc_B.cpp \
	# $(INCLUDE_DIR)/Parser.hpp \

Router_SensitivityList = \
	$(SRC_DIR)/router.cpp \
	# $(INCLUDE_DIR)/Server.hpp \
	# $(INCLUDE_DIR)/User.hpp \
	# $(INCLUDE_DIR)/IncludeAndDefine.hpp \

Driver_SensitivityList = \
	$(SRC_DIR)/driver.cpp \

# NetworkSensitivityList = \
# 	$(SRC_DIR)/Network.cpp \
# 	$(INCLUDE_DIR)/Server.hpp \
# 	$(INCLUDE_DIR)/Parser.hpp \
# 	$(INCLUDE_DIR)/IncludeAndDefine.hpp \
# 	$(CONFIGURATION_DIR)/config.json \

# ClientSensitivityList = \
# 	$(SRC_DIR)/Client.cpp \
# 	$(INCLUDE_DIR)/Parser.hpp \
# 	$(INCLUDE_DIR)/IncludeAndDefine.hpp \
# 	$(CONFIGURATION_DIR)/config.json \

all: $(BUILD_DIR) r.out b.out a.out d.out

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

d.out : $(BUILD_DIR)/d.o $(OBJECTS)
	$(CC) -o d.out $(BUILD_DIR)/d.o $(OBJECTS)

a.out : $(BUILD_DIR)/a.o $(OBJECTS)
	$(CC) -o a.out $(BUILD_DIR)/a.o $(OBJECTS)

b.out : $(BUILD_DIR)/b.o $(OBJECTS)
	$(CC) -o b.out $(BUILD_DIR)/b.o $(OBJECTS)

r.out : $(BUILD_DIR)/r.o $(OBJECTS)
	$(CC) -o r.out $(BUILD_DIR)/r.o $(OBJECTS)

$(BUILD_DIR)/d.o: $(Driver_SensitivityList)
	$(CC) -c $(SRC_DIR)/driver.cpp -o $(BUILD_DIR)/d.o

$(BUILD_DIR)/a.o: $(PC_A_SensitivityList)
	$(CC) -c $(SRC_DIR)/pc_A.cpp -o $(BUILD_DIR)/a.o

$(BUILD_DIR)/b.o: $(PC_B_SensitivityList)
	$(CC) -c $(SRC_DIR)/pc_B.cpp -o $(BUILD_DIR)/b.o

$(BUILD_DIR)/r.o: $(Router_SensitivityList)
	$(CC) -c $(SRC_DIR)/router.cpp -o $(BUILD_DIR)/r.o

# $(BUILD_DIR)/Network.o: $(NetworkSensitivityList)
# 	$(CC) -c $(SRC_DIR)/Network.cpp -o $(BUILD_DIR)/Network.o

# $(BUILD_DIR)/Client.o: $(ClientSensitivityList)
# 	$(CC) -c $(SRC_DIR)/Client.cpp -o $(BUILD_DIR)/Client.o

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) *.o *.out