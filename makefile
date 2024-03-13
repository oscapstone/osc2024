# Define variables
IMAGE_NAME = osdi
CONTAINER_NAME = my_osdi
CURRENT_DIR = $(shell pwd)

.PHONY: build run rm compile assemble

# Default target
default: run

# Target to build the Docker image if it doesn't exist
build:
	if [ -z "$$(docker images -q $(IMAGE_NAME) 2> /dev/null)" ]; then \
		echo "Docker image '$(IMAGE_NAME)' not found. Building..."; \
		docker build -t $(IMAGE_NAME) .; \
	fi

# Target to run the Docker container
run: build
	sudo docker run -it --name $(CONTAINER_NAME) -v $(CURRENT_DIR):/home/laxiflora/osc2024 $(IMAGE_NAME)

rm:
	docker rm $(CONTAINER_NAME)

compile: test.c
	aarch64-linux-gnu-gcc test.c -o test
	aarch64-linux-gnu-gcc test.c -c

assemble: test.c
	aarch64-linux-gnu-gcc -T linker.ld -o test.s test.c
