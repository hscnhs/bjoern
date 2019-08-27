SOURCE_DIR	= bjoern
BUILD_DIR	= build
PYTHON	?= python3
LIBEV_INCLUDE ?= /usr/include
LIBEV_LIB ?= /usr/lib/libev.a

PYTHON_INCLUDE	= $(shell ${PYTHON}-config --includes)
PYTHON_LDFLAGS	= $(shell ${PYTHON}-config --ldflags)

LLHTTP_DIR	= llhttp
LLHTTP_OBJ = $(LLHTTP_DIR)/llhttp.o $(LLHTTP_DIR)/http.o $(LLHTTP_DIR)/api.o $(LLHTTP_DIR)/url_parser.o
LLHTTP_SRC = $(LLHTTP_DIR)/llhttp.c $(LLHTTP_DIR)/http.c $(LLHTTP_DIR)/api.c $(LLHTTP_DIR)/url_parser.c

objects		= $(LLHTTP_OBJ) \
		  $(patsubst $(SOURCE_DIR)/%.c, $(BUILD_DIR)/%.o, \
		             $(wildcard $(SOURCE_DIR)/*.c))

CPPFLAGS	+= $(PYTHON_INCLUDE) -I . -I $(SOURCE_DIR) -I $(LLHTTP_DIR) -I$(LIBEV_INCLUDE)
CFLAGS		+= $(FEATURES) -std=c99 -fno-strict-aliasing -fcommon  -Wall
LDFLAGS		+= $(PYTHON_LDFLAGS) $(LIBEV_LIB) -fcommon

ifneq ($(WANT_SENDFILE), no)
FEATURES	+= -D WANT_SENDFILE
endif

ifneq ($(WANT_SIGINT_HANDLING), no)
FEATURES	+= -D WANT_SIGINT_HANDLING
endif

ifneq ($(WANT_SIGNAL_HANDLING), no)
FEATURES	+= -D WANT_SIGNAL_HANDLING
endif

ifndef SIGNAL_CHECK_INTERVAL
FEATURES	+= -D SIGNAL_CHECK_INTERVAL=0.1
endif

all: prepare-build $(LLHTTP_OBJ) $(objects) bjoernexe

print-env:
	@echo CFLAGS=$(CFLAGS)
	@echo CPPFLAGS=$(CPPFLAGS)
	@echo LDFLAGS=$(LDFLAGS)
	@echo args=$(HTTP_PARSER_SRC) $(wildcard $(SOURCE_DIR)/*.c)

opt: clean
	CFLAGS='-O3' make

small: clean
	CFLAGS='-Os' make

bjoernexe: $(objects)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $^ -o $(BUILD_DIR)/bjoern

again: clean all

debug:
	CFLAGS+='-D DEBUG -g' make

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c
	@echo ' -> ' $(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(LLHTTP_DIR)/%.o: $(LLHTTP_DIR)/%.c
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

prepare-build:
	@mkdir -p $(BUILD_DIR)

clean:
	@rm -rf $(BUILD_DIR)/*

memwatch:
	watch -n 0.5 \
	  'cat /proc/$$(pgrep -n ${PYTHON})/cmdline | tr "\0" " " | head -c -1; \
	   echo; echo; \
	   tail -n +25 /proc/$$(pgrep -n ${PYTHON})/smaps'
