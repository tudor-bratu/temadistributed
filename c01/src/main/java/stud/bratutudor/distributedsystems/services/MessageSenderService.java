package stud.bratutudor.distributedsystems.services;

import org.springframework.amqp.rabbit.core.RabbitTemplate;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import stud.bratutudor.distributedsystems.configuration.RabbitMQConfig;
import stud.bratutudor.distributedsystems.dto.ImageProcessingRequestDTO;
import stud.bratutudor.distributedsystems.dto.UserRequestDTO;

@Service
public class MessageSenderService {

    private final RabbitTemplate rabbitTemplate;
    private static final String EXCHANGE_NAME = RabbitMQConfig.EXCHANGE_NAME; // Example exchange
    private static final String ROUTING_KEY = RabbitMQConfig.ROUTING_KEY;    // Example routing key


    @Autowired
    public MessageSenderService(RabbitTemplate rabbitTemplate) {
        this.rabbitTemplate = rabbitTemplate;
    }

    public void sendSimpleMessage(String message) {
        // This will send the message to the specified exchange with the specified routing key.
        // RabbitMQ will then route it to any queues bound to this exchange with this routing key.
        rabbitTemplate.convertAndSend(EXCHANGE_NAME, ROUTING_KEY, message);
        System.out.println("Sent message: '" + message + "' to exchange '" + EXCHANGE_NAME + "' with routing key '" + ROUTING_KEY + "'");
    }

    public void sendImageMessage(ImageProcessingRequestDTO imageProcessingRequestDTO){

        rabbitTemplate.convertAndSend(EXCHANGE_NAME, ROUTING_KEY, imageProcessingRequestDTO);
        System.out.println("Sent message: '" + imageProcessingRequestDTO + "' to exchange '" + EXCHANGE_NAME + "' with routing key '" + ROUTING_KEY + "'");
    }
}
