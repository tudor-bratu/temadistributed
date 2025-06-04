package stud.bratutudor.services;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.TimeUnit;

@Service
public class ImageProcessingService {

    private static final Logger logger = LoggerFactory.getLogger(ImageProcessingService.class);

    @Value("${native.image.processor.path}")
    private String nativeImageProcessorPath;

    // Assuming you might want to configure a temporary directory
    @Value("${temp.file.directory:/tmp}") // Default to /tmp if not set
    private String tempFileDirectory;

    public byte[] processImageWithNativeApp(
            byte[] imageData,
            String originalFileName, // You might use this to derive extensions or for logging
            String mode,        // "ECB" or "CBC"
            String operation,   // "encrypt" or "decrypt"
            String aesKey) throws IOException, InterruptedException {

        Path tempInputFile = null;
        Path tempOutputFile = null;

        try {
            // 1. Create temporary directory if it doesn't exist
            logger.info("Attempting to use temp directory: '{}'", tempFileDirectory);
            Path tempDir = Path.of(tempFileDirectory);
            if (!Files.exists(tempDir)) {
                Files.createDirectories(tempDir);
            }

            // 2. Create temporary input file for the image data
            String inputFileName = "input_" + UUID.randomUUID().toString() + ".bmp"; // Assuming BMP
            tempInputFile = Files.createTempFile(tempDir, "input_" + UUID.randomUUID().toString() + "_", ".bmp");
            Files.write(tempInputFile, imageData);
            logger.info("Temporary input file created: {}", tempInputFile.toAbsolutePath());

            // 3. Define temporary output file path
            String outputFileName = "output_" + UUID.randomUUID().toString() + ".bmp";
            tempOutputFile = Path.of(tempDir.toString(), outputFileName); // Define path but don't create file yet
            logger.info("Temporary output file will be at: {}", tempOutputFile.toAbsolutePath());


            // 4. Construct the command for ProcessBuilder
            List<String> command = new ArrayList<>();
            command.add(nativeImageProcessorPath);
            command.add(tempInputFile.toAbsolutePath().toString());
            command.add(aesKey); // The passphrase
            command.add(tempOutputFile.toAbsolutePath().toString());
            command.add(operation); // "encrypt" or "decrypt"
            command.add(mode);      // "ECB" or "CBC"

            ProcessBuilder processBuilder = new ProcessBuilder(command);
            // Optional: Redirect error stream to output stream
            // processBuilder.redirectErrorStream(true);

            logger.info("Executing native command: {}", String.join(" ", command));

            // 5. Execute the process
            Process process = processBuilder.start();

            // 6. Read standard output and error streams (important!)
            //    Using separate threads for stdout and stderr is more robust for long-running processes
            //    or processes with lots of output. For simplicity here, a basic read.
            StringBuilder processOutput = new StringBuilder();
            try (BufferedReader stdInputReader = new BufferedReader(new InputStreamReader(process.getInputStream()));
                 BufferedReader stdErrorReader = new BufferedReader(new InputStreamReader(process.getErrorStream()))) {

                String s;
                logger.info("Native Process Standard Output:");
                while ((s = stdInputReader.readLine()) != null) {
                    logger.info(s);
                    processOutput.append(s).append(System.lineSeparator());
                }

                logger.info("Native Process Standard Error:");
                while ((s = stdErrorReader.readLine()) != null) {
                    logger.error(s); // Log errors specifically
                    processOutput.append("ERROR: ").append(s).append(System.lineSeparator());
                }
            }

            // 7. Wait for the process to complete (with a timeout)
            boolean finished = process.waitFor(60, TimeUnit.SECONDS); // Example: 60-second timeout

            if (!finished) {
                process.destroyForcibly();
                throw new InterruptedException("Native image processing timed out.");
            }

            int exitCode = process.exitValue();
            logger.info("Native process exited with code: {}", exitCode);

            // 8. Check exit code and read output file
            if (exitCode == 0) {
                if (Files.exists(tempOutputFile) && Files.size(tempOutputFile) > 0) {
                    return Files.readAllBytes(tempOutputFile);
                } else {
                    logger.error("Native process reported success (exit code 0), but output file is missing or empty: {}", tempOutputFile.toAbsolutePath());
                    throw new IOException("Native process failed to produce output file, even with exit code 0. Output: " + processOutput);
                }
            } else {
                return new byte[0];
//                throw new IOException("Native image processing failed with exit code " + exitCode + ". Output: " + processOutput);
            }

        } finally {
            // 9. Clean up temporary files
            if (tempInputFile != null) {
                try {
                    Files.deleteIfExists(tempInputFile);
                    logger.info("Temporary input file deleted: {}", tempInputFile.toAbsolutePath());
                } catch (IOException e) {
                    logger.warn("Could not delete temporary input file: {}", tempInputFile.toAbsolutePath(), e);
                }
            }
            if (tempOutputFile != null) {
                try {
                    Files.deleteIfExists(tempOutputFile);
                    logger.info("Temporary output file deleted: {}", tempOutputFile.toAbsolutePath());
                } catch (IOException e) {
                    logger.warn("Could not delete temporary output file: {}", tempOutputFile.toAbsolutePath(), e);
                }
            }
        }
    }
}