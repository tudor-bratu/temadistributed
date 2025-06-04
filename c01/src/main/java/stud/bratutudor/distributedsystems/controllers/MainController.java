package stud.bratutudor.distributedsystems.controllers;

import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;


@Controller
public class MainController {

    @RequestMapping(value = "/")
    public String mainView() {
        return "WelcomeScreen";
    }
}
