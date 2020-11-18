#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include <../lib/dht/dht.h>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "driver/gpio.h"

#define PORT CONFIG_EXAMPLE_PORT
#define TRUE            1 
#define FALSE           0
#define LED_B           GPIO_NUM_14
#define LED_BIT         BIT0

#define DHT_GPIO 16


static const char *TAG = "TRB_TCP";
static EventGroupHandle_t LED_event_group; //Cria o objeto do grupo de eventos

static const gpio_num_t dht_gpio = DHT_GPIO;

QueueHandle_t bufferValor;


void task_dados(void *pvParameters){

    
    uint16_t temperatura;
    uint32_t umidade;
    uint32_t distancia;
    char stringTemperatura[10];
    char stringUmidade[10];
    char stringDistancia[10];

    while(TRUE){

        xQueueReceive(bufferValor,&temperatura,pdMS_TO_TICKS(2000));
        xQueueReceive(bufferValor,&umidade,pdMS_TO_TICKS(2000));
        xQueueReceive(bufferValor,&distancia,pdMS_TO_TICKS(2000));
        

        
        sprintf(stringTemperatura, "%d", temperatura);
        sprintf(stringUmidade, "%d", umidade);
        sprintf(stringDistancia, "%d", distancia);


        vTaskDelay(2000/portTICK_PERIOD_MS);
    }

}

void task_dht(void *pvParamters)
{
    int16_t temperatura=0;
    int16_t umidade=0;
    int16_t distancia=0;
    gpio_set_pull_mode( dht_gpio , GPIO_PULLUP_ONLY);
    while(1)
    {       umidade = esp_random()/1000000;
            temperatura = esp_random()/1000000;
            distancia = esp_random()/1000000;
        
            //umidade = umidade / 10; //Quem tem sensor
            //temperatura = temperatura / 10; //Quem tem sensor
            xQueueSend(bufferValor,&temperatura,pdMS_TO_TICKS(0)); 
            xQueueSend(bufferValor,&umidade,pdMS_TO_TICKS(0));
            xQueueSend(bufferValor,&distancia,pdMS_TO_TICKS(0));
            
            ESP_LOGI(TAG,"Temperatura %dºC | Umidade %d%% | Distancia %dcm", temperatura, umidade, distancia );
             
        vTaskDelay(2000/portTICK_PERIOD_MS);
    }
}



static void do_retransmit(const int sock)
{
    int len;
    char rx_buffer[128];

    do {
        len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) {
            ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
        } else if (len == 0) {
            ESP_LOGW(TAG, "Connection closed");
        } else {

            rx_buffer[len] = 0; // Null-terminate whatever is received and treat it like a string
            ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);

            // send() can return less bytes than supplied length.
            // Walk-around for robust implementation. 
            /*int to_write = len;
            while (to_write > 0) {
                int written = send(sock, rx_buffer + (len - to_write), to_write, 0);
                if (written < 0) {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                }
                to_write -= written;
            }*/
            


            uint16_t temperatura;
            uint16_t umidade;
            uint32_t distancia;

            xQueueReceive(bufferValor,&temperatura,pdMS_TO_TICKS(2000));
            xQueueReceive(bufferValor,&umidade,pdMS_TO_TICKS(2000));
            xQueueReceive(bufferValor,&distancia,pdMS_TO_TICKS(2000));

            ESP_LOGI(TAG, "Temperatura: %d ºC\n",temperatura);
            ESP_LOGI(TAG, "Umidade: %d %%\n",umidade);
            ESP_LOGI(TAG, "Distancia: %d cm\n", distancia);

            char resposta[50];

            if(rx_buffer[0] == 't'){
                sprintf(resposta, "\n\rTemperatura: %d C", temperatura);
                write(sock, resposta, strlen(resposta));
            }else if(rx_buffer[0] == 'u'){
                sprintf(resposta, "\n\rUmidade: %d  %%", umidade);
                write(sock, resposta , strlen(resposta));
            }else if(rx_buffer[0] == 'd'){
                sprintf(resposta, "\n\rDistancia: %d cm", distancia);
                write(sock, resposta, strlen(resposta));
            }else if(len>=3 && rx_buffer[0]=='L' && rx_buffer[1]=='E' && rx_buffer[2]=='D')
            {
                xEventGroupSetBits(LED_event_group,LED_BIT);
                
            }else{
                sprintf(resposta, "\n\rValor incorreto");
                write(sock, resposta, strlen(resposta));
            }


        }
    } while (len > 0);
}

void task_toogleLED( void *pvParameter )
{
    bool estadoLed=0;
    
    while (TRUE) 
    {           
        xEventGroupWaitBits(LED_event_group, LED_BIT, pdTRUE, pdFALSE, portMAX_DELAY);  
        estadoLed = !estadoLed;
        gpio_set_level(LED_B,estadoLed);
        ESP_LOGI(TAG, "LED invertido!");        
        vTaskDelay( 10/portTICK_PERIOD_MS );
    }
}


static void tcp_server_task(void *pvParameters)
{
    char addr_str[128];
    int addr_family;
    int ip_protocol;


#ifdef CONFIG_EXAMPLE_IPV4
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
#else // IPV6
    struct sockaddr_in6 dest_addr;
    bzero(&dest_addr.sin6_addr.un, sizeof(dest_addr.sin6_addr.un));
    dest_addr.sin6_family = AF_INET6;
    dest_addr.sin6_port = htons(PORT);
    addr_family = AF_INET6;
    ip_protocol = IPPROTO_IPV6;
    inet6_ntoa_r(dest_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        goto CLEAN_UP;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1) {

        ESP_LOGI(TAG, "Socket listening");

        struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
        uint addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        // Convert ip address to string
        if (source_addr.sin6_family == PF_INET) {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
        } else if (source_addr.sin6_family == PF_INET6) {
            inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
        }
        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

        do_retransmit(sock);

        shutdown(sock, 0);
        close(sock);
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}

void app_main()
{
    LED_event_group = xEventGroupCreate(); //Cria o grupo de eventos

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());


    gpio_pad_select_gpio( LED_B ); 
    gpio_set_direction( LED_B, GPIO_MODE_OUTPUT );  
    bufferValor = xQueueCreate(3,sizeof(uint32_t));

    
    xTaskCreate(task_toogleLED, "task_toogleLED", 4096, NULL, 5, NULL);
    xTaskCreate(task_dht, "task_dht",2048,NULL,5,NULL);
    xTaskCreate(task_dados, "task_dados",2048,NULL,1,NULL);
    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
    
}
