package stud.bratutudor.distributedsystems.controllers;

import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;
import stud.bratutudor.distributedsystems.dto.UserRequestDTO;
import stud.bratutudor.distributedsystems.services.MessageSenderService;

@RestController
@RequestMapping("/test")
public class TestController {
private final MessageSenderService messageSenderService;

    public TestController(MessageSenderService messageSenderService) {
        this.messageSenderService = messageSenderService;
    }

    @RequestMapping("/1")
    public void testView() {
        messageSenderService.sendSimpleMessage("Hello World!");
        System.out.printf("Salut am trimis un mesaj");
    }

//    @RequestMapping("/2")
//    public void testView2() {
//        UserRequestDTO userRequestDTO = new UserRequestDTO(null, "operator", "CBC", "121313412");
//        messageSenderService.sendImageMessage(userRequestDTO);
//
//    }

    @RequestMapping("/3")
    public void testView3() {
        messageSenderService.sendSimpleMessage("Hello World!");
        System.out.printf("Salut am trimis un mesaj");
    }
}
