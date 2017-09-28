UNAME := $(shell uname)

all: AsciiImageServer

AsciiImageServer: server_comms.c image_server.c
ifeq ($(UNAME), Linux)
	echo "Building for linux"
	gcc -w -fno-stack-protector server_comms.c image_server.c -O0 -fno-common -o AsciiImageServer
endif
ifeq ($(UNAME), Darwin)
	echo "Building for macOS"
	clang -Wno-everything -fno-pie -fno-stack-protector server_comms.c image_server.c -O0 -fno-common -o AsciiImageServer
endif

clean:
	rm AsciiImageServer