package stud.bratutudor.c03_consumer.config;

import org.springframework.amqp.core.Binding;
import org.springframework.amqp.core.BindingBuilder;
import org.springframework.amqp.core.DirectExchange;
import org.springframework.amqp.core.Queue;
import org.springframework.amqp.support.converter.Jackson2JsonMessageConverter;
import org.springframework.amqp.support.converter.MessageConverter;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

@Configuration
public class RabbitMQConfig {

    public static final String EXCHANGE_NAME = "bratu-exchange";
    public static final String ROUTING_KEY = "bratu-routing-key";
    public static final String QUEUE_NAME = "bratu-queue"; // Define a queue to receive messages

    // 1. Declare the Direct Exchange
    @Bean
    public DirectExchange directExchange() {
        // DirectExchange(name, durable, autoDelete)
        // durable: true means the exchange will survive a broker restart
        // autoDelete: false means the exchange won't be deleted even if no queues are bound to it
        return new DirectExchange(EXCHANGE_NAME, true, false);
    }

    // 2. Declare a Queue
    @Bean
    public Queue myQueue() {
        // Queue(name, durable, exclusive, autoDelete)
        // durable: true means the queue will survive a broker restart
        // exclusive: false means it's not exclusive to this connection
        // autoDelete: false means it won't be deleted when consumers disconnect
        return new Queue(QUEUE_NAME, true, false, false);
    }

    // 3. Bind the Queue to the Exchange with the Routing Key
    // This tells RabbitMQ to send messages from 'my-direct-exchange' with 'my-routing-key'
    // to 'my-queue'.
    @Bean
    public Binding binding(Queue myQueue, DirectExchange directExchange) {
        return BindingBuilder.bind(myQueue).to(directExchange).with(ROUTING_KEY);
    }

    // Configure the consumer to expect JSON and trust your DTO package
    @Bean
    public MessageConverter jsonMessageConverter() {
        Jackson2JsonMessageConverter converter = new Jackson2JsonMessageConverter();
        // This tells Jackson to trust classes from your DTO package when deserializing.
        // The package string MUST match the package of the DTO that the PRODUCER sent.
        return converter;
    }
}
