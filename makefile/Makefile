CC=gcc

SUBDIRS = f1 \
         f2 \
         main \
         obj 
OBJS = f1.o f2.o main.o
BIN=myapp
OBJS_DIR =obj
BIN_DIR=bin
LIBS=-lpthread
LDFLAGS=-L/usr/lib/

export CC OBJS BIN OBJS_DIR BIN_DIR  LIBS  LDFLAGS

all:CHECK_DIR $(SUBDIRS)
CHECK_DIR:
	@mkdir -p $(BIN_DIR)
$(SUBDIRS):ECHO
	@make -C $@

ECHO:
	@echo $(SUBDIRS)
	@echo begin compile
clean:
	@$(RM)  $(OBJS_DIR)/*.o
	@rm -rf $(BIN_DIR)







