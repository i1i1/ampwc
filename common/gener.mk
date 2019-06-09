# in order to preserve user variables
_SRC = $(SRC) $(wildcard src/*.c)
_HDR = $(HDR) $(wildcard include/*.h)
_OBJ = $(subst src, build, $(_SRC:.c=.o))
DEP =  $(subst src, build, $(_SRC:.c=.d))

all: $(PRE) $(DEP) $(_OBJ) $(OUT)

ifneq ($(OUT),)
$(OUT): $(_OBJ) $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
endif

$(DEP): $(PRE)

build/%.d: src/%.c
	$(CC) $(CFLAGS) -MM $< | sed -e "s!^[^ \t]\+\.o:!build/&!" > $@

build/%.o: src/%.c build/%.d
	$(CC) $(CFLAGS) -c $< -o $@

clean: userclean
	rm -f $(_OBJ) $(OUT)

fullclean: clean
	rm -f $(DEP)

.PHONY: clean userclean fullclean all
.SECONDARY: $(DEP)

ifeq "$(MAKECMDGOALS)" "clean"
else
 ifeq "$(MAKECMDGOALS)" "fullclean"
 else
  -include $(DEP)
 endif
endif

