package stud.bratutudor.c03_consumer.consumer;

import org.springframework.amqp.rabbit.annotation.RabbitListener;
import org.springframework.stereotype.Component;
import reactor.core.publisher.Mono;
import reactor.core.scheduler.Schedulers;
import stud.bratutudor.c03_consumer.config.RabbitMQConfig;
import stud.bratutudor.c03_consumer.services.ImageProcessingService;
import stud.bratutudor.c03_consumer.services.WebClientService;
import stud.bratutudor.distributedsystems.dto.ImageProcessingRequestDTO;

import java.io.IOException;

@Component
public class RabbitMQConsumer {

    private final ImageProcessingService imageProcessingService;
    private final WebClientService webClientService;

    public RabbitMQConsumer(ImageProcessingService imageProcessingService, WebClientService webClientService) {
        this.imageProcessingService = imageProcessingService;
        this.webClientService = webClientService;
    }

    @RabbitListener(queues = RabbitMQConfig.QUEUE_NAME)
    public void receiveMessage(ImageProcessingRequestDTO message) throws IOException, InterruptedException {
        System.out.println("Received <" + message + ">");
        if(message.getImageData() == null)
            return;

        byte[] fullData = message.getImageData();
        int mid = (fullData.length + 1) / 2; // ensures first half gets the extra byte if odd length
        byte[] data_process1 = new byte[mid];
        byte[] data_process2 = new byte[fullData.length - mid];

// Copy the first half
        System.arraycopy(fullData, 0, data_process1, 0, data_process1.length);
// Copy the second half
        System.arraycopy(fullData, mid, data_process2, 0, data_process2.length);

// Example: To join them back again into one array:

// Now combined[] is identical to fullData[]
//        Mono<byte[]> process2Mono = webClientService.sendDataForProcessingInC4(data_process2,message);
//        process2Mono.subscribe();
//
//        Mono<byte[]> processMono1 = Mono.fromCallable(() ->
//                imageProcessingService.processImageWithNativeApp(
//                        data_process1,
//                        message.getOriginalFileName(),
//                        message.getMode(),
//                        message.getOperation(),
//                        message.getAes_key()
//                )
//        );
//
//        processMono1.subscribe();
////        byte[] result1 = imageProcessingService.processImageWithNativeApp(data_process1,
////                message.getOriginalFileName(), message.getMode(), message.getOperation(), message.getAes_key());
//
//        Mono.zip(processMono1, process2Mono).subscribe(tuple -> {
//
//            byte[] combined = new byte[data_process1.length + data_process2.length];
//            System.arraycopy(data_process1, 0, combined, 0, data_process1.length);
//            System.arraycopy(data_process2, 0, combined, data_process1.length, data_process2.length);
//        })
//
//        webClientService.storeBlob(result1, message.getOriginalFileName(), message.getUuid()).flatMap(res_1 ->{
//            return webClientService.notifyService(res_1);
//        }).doOnSuccess(System.out::println).subscribe();

//        webClientService.storeBlobBlocking(result, message.getOriginalFileName());

        Mono<byte[]> processMono1 = Mono.fromCallable(() ->
                imageProcessingService.processImageWithNativeApp(
                        fullData,
                        message.getOriginalFileName(),
                        message.getMode(),
                        message.getOperation(),
                        message.getAes_key()
                )
        ).subscribeOn(Schedulers.boundedElastic());  // Non-blocking

        Mono<byte[]> processMono2 = webClientService.sendDataForProcessingInC4(fullData, message);

        // Zip and process the pipeline
        Mono<Void> pipeline = Mono.zip(processMono1, processMono2)
                .map(tuple -> {  // tuple is Tuple2<byte[], byte[]>
                    byte[] result1 = tuple.getT1();  // Processed result from processMono1
                    byte[] result2 = tuple.getT2();  // Processed result from processMono2

                    // Combine the processed results into a single byte array
                    byte[] combinedResult = new byte[result1.length + result2.length];
                    System.arraycopy(result1, 0, combinedResult, 0, result1.length);
                    System.arraycopy(result2, 0, combinedResult, result1.length, result2.length);

                    return result1;  // Return the combined byte array
                })
                .flatMap(combinedResult ->
                        webClientService.storeBlob(combinedResult, message.getOriginalFileName(), message.getUuid())
                )
                .flatMap(res_1 -> webClientService.notifyService(res_1))
                .doOnSuccess(result -> System.out.println("Processing successful: " + result))  // Log success
                .doOnError(ex -> System.err.println("Error during processing: " + ex.getMessage()));  // Log errors
                  // Handle errors

        // Subscribe and block to ensure completion
        try {
            pipeline.block();  // Wait for the pipeline to complete
            System.out.println("Pipeline ran: ");
        } catch (Exception ex) {
            System.err.println("Pipeline failed: " + ex.getMessage());
            // Add any additional error handling, like logging or retries
        }
    }
}
