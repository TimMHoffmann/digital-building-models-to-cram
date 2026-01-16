FROM osrf/ros:noetic-desktop-full
LABEL maintainer="ifc-ros-developer"

ENV DEBIAN_FRONTEND=noninteractive
ENV ROS_DISTRO=noetic
ENV CRAM_WS=/root/cram_ws 
WORKDIR $CRAM_WS

RUN apt-get update && \
    apt-get install -y flex bison git cmake libspdlog-dev libfmt-dev libeigen3-dev \
                       aptitude libboost-all-dev python3 python3-dev python3-distutils \
                       python3-venv python3-pip python3-setuptools libmongoc-dev libjson-glib-dev \
                       software-properties-common libdb-dev build-essential autoconf automake \
                       libtool pkg-config libcurl4-openssl-dev libxml2-dev libssl-dev \
                       libsqlite3-dev libpq-dev wget unzip libncurses5-dev libedit-dev  libgsl-dev\
                       libgoogle-perftools-dev libgmp-dev unixodbc-dev zlib1g-dev python3-networkx python3-shapely\
                       libarchive-dev uuid-dev libpcre2-dev libyaml-dev openjdk-11-jdk junit4 ros-noetic-roslisp-repl\
                       ninja-build doxygen  ros-noetic-moveit  python3-wstool ros-noetic-map-server ros-noetic-pr2-kinematics ros-noetic-pr2-common  ros-noetic-geometric-shapes && \
    rm -rf /var/lib/apt/lists/*

RUN pip3 install --no-cache-dir \
    "numpy>=1.20,<1.24" \
    "scipy>=1.7" \
    "networkx>=2.6" \
    "trimesh>=3.9" \
    "shapely>=1.7"


RUN git clone https://github.com/SWI-Prolog/swipl-devel.git /tmp/swipl-devel && \
    cd /tmp/swipl-devel && \
    git checkout V8.4.3 && \
    git submodule update --init && \
    mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release -G Ninja .. && \
    ninja && ninja install && \
    ldconfig && \
    rm -rf /tmp/swipl-devel

RUN echo "/usr/local/lib/swipl/lib/x86_64-linux" > /etc/ld.so.conf.d/swipl.conf && \
    ldconfig

ENV LD_LIBRARY_PATH=/usr/local/lib/swipl/lib/x86_64-linux:$LD_LIBRARY_PATH

ENV PKG_CONFIG_PATH=/usr/local/lib/swipl/lib/x86_64-linux/pkgconfig:/usr/local/lib/pkgconfig:/usr/lib/pkgconfig

# Raptor2, Rasqal and Redland
RUN wget https://download.librdf.org/source/raptor2-2.0.15.tar.gz -O /tmp/raptor2.tar.gz && \
    cd /tmp && tar xzf raptor2.tar.gz && cd raptor2-2.0.15 && \
    ./configure --prefix=/usr && make -j$(nproc) && make install && cd / && rm -rf /tmp/raptor2*

RUN wget https://download.librdf.org/source/rasqal-0.9.33.tar.gz -O /tmp/rasqal.tar.gz && \
    cd /tmp && tar xzf rasqal.tar.gz && cd rasqal-0.9.33 && \
    ./configure --prefix=/usr && make -j$(nproc) && make install && cd / && rm -rf /tmp/rasqal*

RUN wget https://download.librdf.org/source/redland-1.0.17.tar.gz -O /tmp/redland.tar.gz && \
    cd /tmp && tar xzf redland.tar.gz && cd redland-1.0.17 && \
    ./configure --prefix=/usr --without-sqlite --without-postgresql --without-virtuoso --without-mysql && \
    make -j$(nproc) && make install && cd / && rm -rf /tmp/redland*

# Create catkin workspace
RUN /bin/bash -c "source /opt/ros/$ROS_DISTRO/setup.bash && mkdir -p src"

COPY docker/ros1/src/ /root/cram_ws/src/

RUN /bin/bash -c "source /opt/ros/$ROS_DISTRO/setup.bash && \
    cd src && \
    git clone https://github.com/knowrob/rosprolog.git && \
    git clone https://github.com/knowrob/knowrob.git && \
    git clone https://github.com/knowrob/knowrob_ros.git && \
    wstool update -j4"

RUN /bin/bash -c "source /opt/ros/$ROS_DISTRO/setup.bash && \
    rosdep update"

# Catkin build (with the fix for SWI-Prolog)
RUN /bin/bash -c "source /opt/ros/$ROS_DISTRO/setup.bash && \
    export LD_LIBRARY_PATH=/usr/local/lib/swipl/lib/x86_64-linux:$LD_LIBRARY_PATH && \
    catkin_make -DPython3_EXECUTABLE=/usr/bin/python3 \
                -DCMAKE_EXE_LINKER_FLAGS='-L/usr/local/lib/swipl/lib/x86_64-linux -Wl,-rpath,/usr/local/lib/swipl/lib/x86_64-linux' \
                -DCMAKE_INCLUDE_DIRS='/usr/local/include/swipl'"

ENTRYPOINT ["/ros_entrypoint.sh"]
CMD ["/bin/bash", "-c", " \
      source /root/cram_ws/devel/setup.bash; \
      roscore & sleep 3; \
      exec /bin/bash \
"]
