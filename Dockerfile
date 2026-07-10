FROM osrf/ros:jazzy-desktop

# Resolve o alerta do GID da GPU (Silencia o erro do grupo 992)
RUN getent group 992 || groupadd -g 992 host_992

# Automação do ROS 2
RUN echo "source /opt/ros/jazzy/setup.bash" >> /root/.bashrc
RUN echo "source /workspace/install/setup.bash" >> /root/.bashrc

# Autocompletar (com aspas simples para não quebrar no build)
RUN apt-get update && apt-get install -y python3-argcomplete
RUN echo 'eval "$(register-python-argcomplete ros2)"' >> /root/.bashrc
RUN echo 'eval "$(register-python-argcomplete colcon)"' >> /root/.bashrc

# Build dependencies for conectivity_check
RUN apt-get update && apt-get install -y \
    libnl-3-dev \
    libnl-genl-3-dev \
    libmm-glib-dev \
    libyaml-cpp-dev \
    modemmanager \
    iw \
    iproute2 \
    speedtest-cli \
    && rm -rf /var/lib/apt/lists/*

# Se precisar adicionar bibliotecas futuras
# (ex: drivers para o UGV), faça aqui.