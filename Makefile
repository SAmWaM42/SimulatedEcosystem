cmp := g++
cflags := -Wall -Wextra -std=c++17 -MMD -MP
flags := -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl 
output := main

src := $(shell find . -type f -name "*.c++" -not -path "./mapmaker/*")
src := $(patsubst ./%,%,$(src))
src += ../glad/glad.c


objs := $(src:.c++=.o)
objs := $(objs:.c=.o)

deps := $(src:.c++=.d)
deps := $(deps:.c=.d)

all: $(output)

$(output): $(objs)
	$(cmp) $(objs) $(flags) -o $(output)

%.o: %.c++
	$(cmp) $(cflags) -c $< -o $@

%.o: %.c
	gcc -MMD -MP -c $< -o $@

-include $(deps)

.PHONY: clean
clean:
	rm -f $(objs) $(output) $(deps)