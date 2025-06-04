package stud.bratutudor.c03_consumer.services;

import org.springframework.stereotype.Service;
import org.springframework.web.reactive.function.client.WebClient;
import reactor.core.publisher.Mono;
import stud.bratutudor.distributedsystems.dto.ImageProcessingRequestDTO;

import java.util.HashMap;
import java.util.Map;

@Service
public class WebClientService {
    private final WebClient webClient;

    public static String getUuidFromString(String inputString) {
        // Look for "uuid":" and extract the value until the next "
        String searchPrefix = "\"uuid\":\"";
        int startIndex = inputString.indexOf(searchPrefix);

        if (startIndex != -1) {
            // Adjust start index to point to the beginning of the UUID value
            startIndex += searchPrefix.length();
            int endIndex = inputString.indexOf("\"", startIndex);

            if (endIndex != -1) {
                return inputString.substring(startIndex, endIndex);
            }
        }
        // If "uuid":" not found or no closing quote
        return null; // Or throw an IllegalArgumentException
    }

    public WebClientService(WebClient webClient) {
        this.webClient = webClient;
    }

    public Mono<String> storeBlob(byte[] imageData, String originalFileName, String uuid){
        Map<String, Object> blobData = new HashMap<>();
        blobData.put("image_data", imageData);
        blobData.put("file_name", originalFileName);
        blobData.put("content_type","image/jpeg");
        blobData.put("uuid", uuid);
        System.out.println("se executa!!!!!");
        System.out.println(webClient.toString());

        Mono<String> path = webClient.post().uri("http://con05:3001/api/blobs").bodyValue(blobData)
                .retrieve().bodyToMono(String.class).doOnSuccess(response -> System.out.println("Blob storage success: " + response))
                .doOnError(error -> System.err.println("Blob storage error: " + error.getMessage()));;

        return path;
    }

//    public Mono<String> notifyForSuccess(String path){
//
//        webClient.get().uri("http://localhost:8080/notification/" + path).retrieve().bodyToMono(String.class).doOnSuccess(response -> System.out.println("Notification success: " + response));;
//        return Mono.just("success");
//    }

    public Mono<Void> notifyService(String path){

        String uuid = getUuidFromString(path);
        String uri = "http://con01:8080/api/notification/" + uuid;
        System.out.println("Extracted UUID: " + uuid);

        return webClient.get().uri(uri).retrieve().bodyToMono(Void.class);
//        try {
//            String result = webClient.get()
//                    .uri(uri)
//                    .retrieve()
//                    .bodyToMono(String.class);
//                    // This blocks the thread until completion
//
//            System.out.println("Blob storage success (blocking): " + result);
//        } catch (Exception e) {
//            System.err.println("Blob storage error (blocking): " + e.getMessage());
//            e.printStackTrace();
//            throw e; // Re-throw to let caller handle the exception
//        }

    }

    public Mono<byte[]> sendDataForProcessingInC4(byte[] data, ImageProcessingRequestDTO requestDTO){
        requestDTO.setImageData(data);
        return webClient.post().uri("http://con04:8084/sendData").bodyValue(requestDTO).retrieve().bodyToMono(byte[].class);

    }
    /**
     * Blocking version of storeBlob for debugging purposes.
     * This method blocks the current thread until the HTTP request completes.
     *
     * @param imageData The binary image data to store
     * @param originalFileName The original filename of the image
     * @return The path where the blob is stored
     */
    public String storeBlobBlocking(byte[] imageData, String originalFileName) {
        Map<String, Object> blobData = new HashMap<>();
        blobData.put("image_data", imageData);
        blobData.put("file_name", originalFileName);
        blobData.put("content_type", "image/jpeg");

        System.out.println("Making blocking request to: http://localhost:3001/api/blobs");
        System.out.println("Using WebClient: " + webClient.toString());

        try {
            String result = webClient.post()
                    .uri("http://localhost:3001/api/blobs")
                    .bodyValue(blobData)
                    .retrieve()
                    .bodyToMono(String.class)
                    .block(); // This blocks the thread until completion

            System.out.println("Blob storage success (blocking): " + result);
            return result;
        } catch (Exception e) {
            System.err.println("Blob storage error (blocking): " + e.getMessage());
            e.printStackTrace();
            throw e; // Re-throw to let caller handle the exception
        }
    }
}
