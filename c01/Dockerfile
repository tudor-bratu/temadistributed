# Stage 1: Define the Java runtime environment
# Using OpenJDK 17 JRE slim version as a base
FROM eclipse-temurin:22-jre

# Set the working directory inside the container
WORKDIR /app

# Copy the pre-built JAR file from your host machine into the container
# IMPORTANT:
# Before building this Docker image, ensure 'your-app.jar' exists
# in the same directory as this Dockerfile (the build context).
# You would have built this JAR using './gradlew bootJar' locally.
# For example, if your JAR is 'build/libs/my-service-0.0.1.jar',
# you could copy it here and rename it to 'your-app.jar',
# or change the line below to:
# COPY build/libs/my-service-0.0.1.jar app.jar
COPY app.jar app.jar

# Expose the port your Spring Boot application listens on (default is 8080)
EXPOSE 8080

# Command to run the application when the container starts
ENTRYPOINT ["java", "-jar", "app.jar"]