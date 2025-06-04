package stud.bratutudor.distributedsystems.dto;

import java.io.Serializable;

public class ImageProcessingRequestDTO implements Serializable { // If using SimpleMessageConverter
// public class ImageProcessingRequestDTO { // If using Jackson2JsonMessageConverter (preferred)

    private static final long serialVersionUID = 1L; // Good practice if Serializable

    private byte[] imageData;
    private String originalFileName; // It's good to keep the original file name
    private String mode;
    private String operation;
    private String aes_key;
    private final String uuid = java.util.UUID.randomUUID().toString();

    // No-argument constructor (needed for Jackson and often for Serializable)
    public ImageProcessingRequestDTO() {
    }

    public ImageProcessingRequestDTO(byte[] imageData, String originalFileName, String mode, String operation, String aes_key) {
        this.imageData = imageData;
        this.originalFileName = originalFileName;
        this.mode = mode;
        this.operation = operation;
        this.aes_key = aes_key;
    }

    // Getters and Setters for all fields
    public byte[] getImageData() {
        return imageData;
    }

    public void setImageData(byte[] imageData) {
        this.imageData = imageData;
    }

    public String getOriginalFileName() {
        return originalFileName;
    }

    public void setOriginalFileName(String originalFileName) {
        this.originalFileName = originalFileName;
    }

    public String getMode() {
        return mode;
    }

    public void setMode(String mode) {
        this.mode = mode;
    }

    public String getOperation() {
        return operation;
    }

    public void setOperation(String operation) {
        this.operation = operation;
    }

    public String getAes_key() {
        return aes_key;
    }

    public void setAes_key(String aes_key) {
        this.aes_key = aes_key;
    }

    @Override
    public String toString() {
        return "ImageProcessingRequestDTO{" +
                "originalFileName='" + originalFileName + '\'' +
                ", mode='" + mode + '\'' +
                ", operation='" + operation + '\'' +
                ", aes_key='" + (aes_key != null ? "******" : "null") + '\'' + // Avoid logging key
                ", imageData.length=" + (imageData != null ? imageData.length : "null") +
                ", uuid='" + uuid + '\'' +
                '}';
    }

    public String getUuid() {
        return uuid;
    }
}