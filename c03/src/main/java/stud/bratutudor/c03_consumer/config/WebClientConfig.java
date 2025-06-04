package stud.bratutudor.c03_consumer.config;


import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.util.unit.DataSize;
import org.springframework.web.reactive.function.client.ExchangeStrategies;
import org.springframework.web.reactive.function.client.WebClient;

@Configuration
public class WebClientConfig {

    @Value("${BLOB_API_URL:http://localhost:3001}")
    private String BASE_URL;

    @Bean
    public WebClient webClient() {
//        System.out.println(BASE_URL);
//        return WebClient.create();

        final int size = (int) DataSize.ofMegabytes(25).toBytes();
        final ExchangeStrategies strategies = ExchangeStrategies.builder()
                .codecs(codecs -> codecs.defaultCodecs().maxInMemorySize(size))
                .build();
        return WebClient.builder()
                .exchangeStrategies(strategies)
                .build();
    }



}
