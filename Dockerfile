FROM ubuntu:20.04

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    gcc \
    libgsl-dev \
    python3 \
    python3-pip \
    make \
    && rm -rf /var/lib/apt/lists/*

# Install Python dependencies
RUN pip3 install numpy scipy

# Set working directory
WORKDIR /app

# Copy project files to the container
COPY . /app

# Build the project
RUN make build

# Set default command
CMD ["/bin/bash"]
