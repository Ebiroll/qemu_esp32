#if 0
static void sd_get_task(void *pvParameters)
{
    web_radio_t *radio_conf = pvParameters;

    /* configure callbacks */
    http_parser_settings callbacks = { 0 };
    callbacks.on_body = on_body_cb;
    callbacks.on_header_field = on_header_field_cb;
    callbacks.on_header_value = on_header_value_cb;
    callbacks.on_headers_complete = on_headers_complete_cb;
    callbacks.on_message_complete = on_message_complete_cb;

    // blocks until end of stream
    int result = st_file_get("/sdcard/test_file.mp3",&callbacks,radio_conf->player_config);

    if (result != 0) {
        ESP_LOGE(TAG, "sd_client_get error");
    } else {
        ESP_LOGI(TAG, "sd_client_get completed");
    }

    vTaskDelete(NULL);
}

void web_radio_start(web_radio_t *config)
{
    // start reader task
    //xTaskCreatePinnedToCore(&http_get_task, "http_get_task", 2560, config, 20,NULL, 0);
    xTaskCreatePinnedToCore(&sd_get_task, "sd_get_task", 2560, config, 20,NULL, 0);
}

char header[]="HTTP/1.0 200 OK\nContent-Type: audio/mpeg\n\n";
FILE* st_read_file;

int st_file_get(const char *file_name, http_parser_settings *callbacks, void *user_data) {

    /* Read HTTP response */
    char recv_buf[64];
    bzero(recv_buf, sizeof(recv_buf));
    ssize_t recved;

    /* parse response */
    http_parser parser;
    http_parser_init(&parser, HTTP_RESPONSE);
    parser.data = user_data;

    esp_err_t nparsed = 0;
    ESP_LOGI(TAG,"%s size:%d",header,strlen(header));
    nparsed = http_parser_execute(&parser, callbacks, header, strlen(header));


    st_open_file("","test_file.mp3");
    fseek(st_file,400000,SEEK_SET);

    nparsed=0;
    do {
        recved = fread(recv_buf,sizeof(char),sizeof(recv_buf)-1,st_read_file);
       // ESP_LOGI(TAG,"write mp3 bytes:%d",recved);

        // using http parser causes stack overflow somtimes - disable for now
        nparsed = http_parser_execute(&parser, callbacks, recv_buf, recved);

    } while(recved > 0 && nparsed >= 0);

    st_read_close_file();

    return 0;
}
void st_read_open_file(const char *dir_name,const char *file_name) {
    char path[100];
    create_file_path(path,dir_name,file_name);
    ESP_LOGI(TAG,"st_open_file path:%s",path);
    st_read_file = fopen(path, "r");
    if (st_read_file == NULL) {
        ESP_LOGE(TAG, "Failed to open file %s",path);
        return;
    }
    ESP_LOGI(TAG,"File open OK\n");
}
void st_read_close_file() {
    fclose(st_read_file);
    ESP_LOGI(TAG,"File closed\n");
}
#endif