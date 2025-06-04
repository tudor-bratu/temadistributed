## Use the teacher's provided base image
#FROM critoma/amd64_u24_noble_ism_security:latest
#
## Set DEBIAN_FRONTEND to noninteractive to avoid prompts during installation
#ENV DEBIAN_FRONTEND=noninteractive
#
## Install prerequisites for RabbitMQ installation (some might already be in the base image)
## sudo is often not present or needed in Dockerfiles as commands run as root by default.
#RUN apt-get update && \
#    apt-get install -y wget gnupg apt-transport-https ca-certificates curl lsb-release && \
#    rm -rf /var/lib/apt/lists/*
#
## Add RabbitMQ and Erlang Solutions repositories
## (RabbitMQ requires a supported Erlang version)
#
## 1. Add Erlang Solutions GPG key and repository
#RUN wget -O- https://packages.erlang-solutions.com/ubuntu/erlang_solutions.asc | apt-key add - && \
#    echo "deb https://packages.erlang-solutions.com/ubuntu $(lsb_release -sc) contrib" | tee /etc/apt/sources.list.d/erlang_solutions.list
#
## 2. Add RabbitMQ GPG key and repository (using Cloudsmith for modern RabbitMQ versions)
#RUN curl -fsSL https://packagecloud.io/rabbitmq/rabbitmq-server/gpgkey | gpg --dearmor -o /usr/share/keyrings/rabbitmq-archive-keyring.gpg && \
#    echo "deb [signed-by=/usr/share/keyrings/rabbitmq-archive-keyring.gpg] https://packagecloud.io/rabbitmq/rabbitmq-server/ubuntu/ $(lsb_release -sc) main" | tee /etc/apt/sources.list.d/rabbitmq.list
#
## Install Erlang and RabbitMQ Server
#RUN apt-get update && \
#    apt-get install -y erlang \
#                       rabbitmq-server && \
#    rm -rf /var/lib/apt/lists/*
#
## Enable RabbitMQ Management Plugin
#RUN rabbitmq-plugins enable rabbitmq_management
#
## Expose RabbitMQ ports
#EXPOSE 5672
#EXPOSE 15672
#
## Define environment variables for RabbitMQ (optional, can be set in docker-compose)
#ENV RABBITMQ_DEFAULT_USER=guest
#ENV RABBITMQ_DEFAULT_PASS=guest
#
## Set the default command to start RabbitMQ server
## This command keeps the container running in the foreground
#CMD ["rabbitmq-server"]