# Stage 1: Builder for C++ Native Executable
FROM ubuntu:22.04 AS builder

# Install C++ build tools and OpenSSL/OpenMP development libraries
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        g++ \
        make \
        libssl-dev \
        pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Set working directory for the build
WORKDIR /build

# Copy the C++ source code into the builder stage
# This assumes your C++ file is named image_processor_ssl.cpp
COPY image_processor_ssl.cpp .

# Compile the C++ application
# -Wall: Enable all warnings
# -O2: Optimization level 2
# -std=c++17: Use C++17 standard
# `pkg-config --cflags --libs openssl`: Get compiler and linker flags for OpenSSL
# -fopenmp: Enable OpenMP support
# -o image_processor_ssl: Output executable name
# CORRECTED LINE: Changed image_processor.cpp to image_processor_ssl.cpp
RUN g++ -o image_processor_ssl image_processor_ssl.cpp \
    -Wall -O2 -std=c++17 \
    $(pkg-config --cflags --libs openssl) \
    -fopenmp

# Stage 2: Define the Java runtime environment
FROM eclipse-temurin:22-jre

# Set the working directory inside the container
WORKDIR /app

# Install runtime dependencies for the C++ native binary
# libssl3 for OpenSSL runtime (OpenSSL 3.x), libgomp1 for OpenMP runtime
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        libssl3 \
        libgomp1 \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Copy the pre-built JAR file from your host machine (build context)
# Ensure app.jar is in the same directory as your Dockerfile when building
COPY app.jar app.jar

# Copy the compiled C++ executable from the builder stage
COPY --from=builder /build/image_processor_ssl /app/image_processor_ssl

# Make the C++ binary executable
RUN chmod +x /app/image_processor_ssl

# Expose the port your Spring Boot application listens on
EXPOSE 8080

# Command to run the application when the container starts
ENTRYPOINT ["java", "-jar", "app.jar"]
