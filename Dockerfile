# Use a Debian-based image as the base
FROM debian:bullseye

WORKDIR /home/laxiflora/osc2024

RUN apt-get update


# # Copy the archive into the Docker image
# RUN apt-get install -y xz-utils
# COPY gcc-arm-10.3-2021.07-x86_64-aarch64-none-elf.tar.xz /tmp/
# RUN tar -xvf /tmp/gcc-arm-10.3-2021.07-x86_64-aarch64-none-elf.tar.xz -C /usr/local/
# ENV PATH="/usr/local/gcc-arm-10.3-2021.07-x86_64-aarch64-none-elf/bin:${PATH}"

# Install QEMU for emulation
RUN apt-get install -y \
    qemu-system-arm \
    gdb \
    sudo

# Install the AArch64 cross-compiler toolchain
RUN apt-get install -y \
    gcc-aarch64-linux-gnu
# aarch64-linux-gnu-gcc
# Install Vim and Make
RUN apt-get install -y \
    vim \
    make

# make gcc alia to aarch64-linux-gnu-gcc
RUN echo "alias gcc='aarch64-linux-gnu-gcc'" >> ~/.bashrc

# clean up package caches
RUN rm -rf /var/lib/apt/lists/*



# Set the default command to run when the container starts
CMD ["bash"]