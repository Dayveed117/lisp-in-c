# -------------------------------------------------- #
# --------------------- MACROS --------------------- #
# -------------------------------------------------- #

# https://stackoverflow.com/questions/30573481/how-to-write-a-makefile-with-separate-source-and-header-directories
# https://stackoverflow.com/questions/1950926/create-directories-using-make-file
# https://www.gnu.org/software/make/manual/make.html#Overview
# https://makefiletutorial.com/#getting-started

# Define variables with
# '='  -> redefined when called
# ':=' -> defined only once
# '?=' -> define if not set
# '+=' -> append

# Built-in functions $(fn, arguments)
# subst
# patsubst or $(text:pattern=replacement)
# foreach
# filter
# wildcard
# if
# call

SRC_DIR := ./src
OBJ_DIR := ./obj
BIN_DIR := ./bin

CC 	   := cc -std=c99
LIBS   := -ledit -lm
CFLAGS := -Wall

# match all .c files inside src/
SRCS := $(wildcard $(SRC_DIR)/*.c)

# mapping over SRCS and its contents to build obj files
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# -------------------------------------------------- #
# --------------------- RULES ---------------------- #
# -------------------------------------------------- #

# Topmost rule is considered the default

# Prefix action with @ to hide the command in cli

# target: dependencies
# 	action

# targets...: target-pattern: prereq-patterns ...
# 	commands

# Some Automatic Variables
# $@ -> recipe target
# $? -> recipe all dependencies more recent than target
# $^ -> recipe all dependencies
# $< -> recipe first dependency

# https://stackoverflow.com/questions/12605051/how-to-check-if-a-directory-doesnt-exist-in-make-and-create-it
# order-only-prerequisites with | (pipe)

all: bin/parsing bin/doge bin/doge_grammar

bin/parsing: obj/mpc.o obj/lib.o obj/eval.o obj/parsing.o | bin
	$(CC) $(CLAGS) $^ $(LIBS) -o $@

bin/doge: obj/mpc.o obj/doge.o | bin
	$(CC) $(CLAGS) $^ $(LIBS) -o $@

bin/doge_grammar: obj/mpc.o obj/doge_grammar.o | bin
	$(CC) $(CLAGS) $^ $(LIBS) -o $@

obj/mpc.o: src/mpc.c src/mpc.h | obj
	$(CC) $(CLAGS) -c $< -o $@

obj/lib.o: src/lib.c src/lib.h src/mpc.h | obj
	$(CC) $(CLAGS) -c $< -o $@

obj/eval.o: src/eval.c src/eval.h src/mpc.h | obj
	$(CC) $(CLAGS) -c $< -o $@

obj/parsing.o: src/parsing.c src/mpc.h src/lib.h src/eval.h | obj
	$(CC) $(CLAGS) -c $< -o $@

obj/doge.o: src/doge.c src/mpc.h | obj
	$(CC) $(CLAGS) -c $< -o $@

obj/doge_grammar.o: src/doge_grammar.c src/mpc.h | obj
	$(CC) $(CLAGS) -c $< -o $@

bin:
	mkdir -p $@

obj:
	mkdir -p $@

# $(BIN_DIR)/%: obj/mpc.o $(OBJ_DIR)/%.o | bin
# 	$(CC) $(CLAGS) $^ $(LIBS) -o $@

# pattern-specific variable
# %.c: mpc = src/mpc.h

# $(OBJ_DIR)/%.o: $(SRC_DIR)/%.c src/mpc.h | obj
# 	$(CC) $(CLAGS) -c $< -o $@

run:
	$(BIN_DIR)/parsing

.PHONY: clean

clean:
	rm -rfd $(BIN_DIR) $(OBJ_DIR)
