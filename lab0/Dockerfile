FROM debian:bookworm

# locale
ENV LC_ALL C
ENV DEBIAN_FRONTEND noninteractive

# install packages
RUN apt-get update && apt-get install -y tini locales \
	iproute2 iputils-ping net-tools netcat-openbsd openssh-server \
	sudo vim grep gawk rsync tmux diffutils file less tcpdump \
	man manpages manpages-dev \
	gcc gdb make yasm nasm libcapstone-dev \
	python3 python3-pip python3-virtualenv python3-pwntools \
	libc6-dbg dpkg-dev \
	wget curl git zsh htop && \
	(((uname -a | grep x86 >/dev/null) && apt-get install -y gcc-multilib g++-multilib) || /bin/true)
#RUN apt-get install -y qemu-user-static gcc-mips64-linux-gnuabi64

# install nano, cross-compiler and qemu systems # modification for OSC
RUN apt-get install -y nano build-essential git gcc-aarch64-linux-gnu qemu-system-arm screen

# /var/run/sshd: required on debian/ubuntu
RUN mkdir /var/run/sshd

# locale
ENV LANGUAGE en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LC_ALL en_US.UTF-8
RUN echo "en_US.UTF-8 UTF-8" > /etc/locale.gen
RUN /usr/sbin/locale-gen

## allow empty password
#RUN echo 'PermitEmptyPasswords yes' >> /etc/ssh/sshd_config
#RUN sed -i 's/nullok_secure/nullok/' /etc/pam.d/common-auth

# add user/group, empty password, allow sudo
RUN groupadd -g 1000 alan
# password = sense
RUN useradd --uid 1000 --gid 1000 --groups root,sudo,adm,users --create-home --password "`openssl passwd -6 -salt XX sense`" --shell /bin/bash alan
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

# run the service
EXPOSE 22
ENTRYPOINT ["/usr/bin/tini", "--"]
CMD ["/usr/sbin/sshd", "-D"]

