package stud.bratutudor.distributedsystems.controllers;


import netscape.javascript.JSObject;
import org.springframework.http.MediaType;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.servlet.mvc.method.annotation.SseEmitter;
import stud.bratutudor.distributedsystems.dto.ImageProcessingRequestDTO;
import stud.bratutudor.distributedsystems.dto.UserRequestDTO;
import stud.bratutudor.distributedsystems.services.MessageSenderService;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

@RestController
@RequestMapping("/api")
public class ProcessingController {

    private final MessageSenderService messageSenderService;
    private final Map<String, SseEmitter> emitters = new ConcurrentHashMap<>();

    public ProcessingController(MessageSenderService messageSenderService) {
        this.messageSenderService = messageSenderService;
    }

    @PostMapping(value = "/upload", consumes = MediaType.MULTIPART_FORM_DATA_VALUE)
    public Map<String,Object> uploadFile(@ModelAttribute UserRequestDTO userRequestDTO) throws IOException {

        ImageProcessingRequestDTO imageProcessingRequestDTO = new ImageProcessingRequestDTO();
        imageProcessingRequestDTO.setMode(userRequestDTO.getMode());
        imageProcessingRequestDTO.setOperation(userRequestDTO.getOperation());
        imageProcessingRequestDTO.setAes_key(userRequestDTO.getAes_key());
        imageProcessingRequestDTO.setImageData(userRequestDTO.getBmp_image().getBytes());
        imageProcessingRequestDTO.setOriginalFileName(userRequestDTO.getBmp_image().getOriginalFilename());

//        System.out.println(imageProcessingRequestDTO);
//        System.out.printf(userRequestDTO.toString());

        messageSenderService.sendImageMessage(imageProcessingRequestDTO);

        Map<String,Object> map = new HashMap<>();
        map.put("message", "File uploaded!");
        map.put("uuid", imageProcessingRequestDTO.getUuid());

        return map;
    }

    @GetMapping("/notification/{id}")
    public void getNotification(@PathVariable("id") String id) {
        String endpoint_url = "http://localhost:3001/api/blobs/" + id + "/file";
        System.out.println(endpoint_url);

        try {
            SseEmitter myemitter = emitters.get(id);
            myemitter.send(SseEmitter.event()
                    .data("{\"path\": \"" + endpoint_url + "\", \"status\": \"Connected\"}")
                    .name("init"));
            myemitter.send(endpoint_url);
            System.out.println("Am ajuns aici la emitters");
            myemitter.complete();
            emitters.remove(id);
        }catch (Exception e){
            e.printStackTrace();
        }

        System.out.println("Hopefully Sent Something!");



    }


    @GetMapping("/events")
    public SseEmitter subscribe(@RequestParam("jobid") String jobid) {
        SseEmitter emitter = new SseEmitter(Long.MAX_VALUE);

        emitters.put(jobid, emitter);
        emitter.onCompletion(() -> emitters.remove(jobid));
        emitter.onTimeout(() -> emitters.remove(jobid));
        emitter.onError((error) -> emitters.remove(jobid));

        return emitter;

    }



}
