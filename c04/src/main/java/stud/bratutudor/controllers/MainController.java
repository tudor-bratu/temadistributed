package stud.bratutudor.controllers;


import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RestController;
import reactor.core.publisher.Mono;
import stud.bratutudor.distributedsystems.dto.ImageProcessingRequestDTO;
import stud.bratutudor.services.ImageProcessingService;

import java.io.IOException;

@RestController
public class MainController {
private final ImageProcessingService imageProcessingService;

    public MainController(ImageProcessingService imageProcessingService) {
        this.imageProcessingService = imageProcessingService;
    }

    @PostMapping("/sendData")
public Mono<byte[]> test(@RequestBody ImageProcessingRequestDTO inp){
        System.out.println("Saluuuuuuut!!!");
        return Mono.fromCallable( () ->
            imageProcessingService.processImageWithNativeApp(inp.getImageData(),
                    "copy2_" + inp.getOriginalFileName(),
                    inp.getMode(),
                    inp.getOperation(),
                    inp.getAes_key())
        );


//    byte[] result = imageProcessingService.processImageWithNativeApp(inp.getImageData(),
//            "copy2_" + inp.getOriginalFileName(),inp.getMode(),inp.getOperation(),inp.getAes_key());
//
//        return Mono.just(result);
}
    @GetMapping("/")
    public String index() {
        return "Hello World!";
    }

}
