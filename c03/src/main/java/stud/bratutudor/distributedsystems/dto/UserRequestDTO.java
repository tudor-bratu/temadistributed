//package stud.bratutudor.distributedsystems.dto;
//
//
//import org.springframework.web.multipart.MultipartFile;
//
//public class UserRequestDTO {
//
//    private MultipartFile bmp_image;
//    private String mode;
//    private String operation;
//    private String aes_key;
//
//
//    public UserRequestDTO(MultipartFile bmp_image, String mode, String operation, String aes_key) {
//        this.bmp_image = bmp_image;
//        this.mode = mode;
//        this.operation = operation;
//        this.aes_key = aes_key;
//    }
//
//
//
//
//
//    public MultipartFile getBmp_image() {
//        return bmp_image;
//    }
//
//    public void setBmp_image(MultipartFile bmp_image) {
//        this.bmp_image = bmp_image;
//    }
//
//    public String getMode() {
//        return mode;
//    }
//
//    public void setMode(String mode) {
//        this.mode = mode;
//    }
//
//    public String getOperation() {
//        return operation;
//    }
//
//    public void setOperation(String operation) {
//        this.operation = operation;
//    }
//
//    public String getAes_key() {
//        return aes_key;
//    }
//
//    public void setAes_key(String aes_key) {
//        this.aes_key = aes_key;
//    }
//
//    @Override
//    public String toString() {
//        return "UserRequestDTO{" + "bitmapImage=" + bmp_image.getOriginalFilename() + ", option='" + mode + '\'' + ", operation='" + operation + '\'' + ", aesKey='" + aes_key + '\'' + '}';
//    }
//}
