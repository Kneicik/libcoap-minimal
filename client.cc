/* minimal CoAP client
 *
 * Copyright (C) 2018-2021 Olaf Bergmann <bergmann@tzi.org>
 */

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <iostream>
#include "common.hh"

static int have_response = 0;
uint8_t *data = NULL;
size_t data_len = 0;
const char* payload = "1";

//XBOX

int read_event(int fd, struct js_event *event)
{
    ssize_t bytes;

    bytes = read(fd, event, sizeof(*event));

    if (bytes == sizeof(*event))
        return 0;

    /* Error, could not read full event. */
    return -1;
}

size_t get_button_count(int fd)
{
    __u8 buttons;
    if (ioctl(fd, JSIOCGBUTTONS, &buttons) == -1)
        return 0;

    return buttons;
}

// CoAP

int blink(void) {
  coap_context_t  *ctx = nullptr;
  coap_session_t *session = nullptr;
  coap_address_t dst;
  coap_pdu_t *pdu = nullptr;
  int result = EXIT_FAILURE;;

  coap_startup();

  /* Set logging level */
  coap_set_log_level(LOG_WARNING);

  /* resolve destination address where server should be sent */
  if (resolve_address("192.168.2.169", "5683", &dst) < 0) {
    coap_log(LOG_CRIT, "failed to resolve address\n");
    goto finish;
  }

  /* create CoAP context and a client session */
  if (!(ctx = coap_new_context(nullptr))) {
    coap_log(LOG_EMERG, "cannot create libcoap context\n");
    goto finish;
  }
  /* Support large responses */
  coap_context_set_block_mode(ctx,
                  COAP_BLOCK_USE_LIBCOAP | COAP_BLOCK_SINGLE_BODY);

  if (!(session = coap_new_client_session(ctx, nullptr, &dst,
                                                  COAP_PROTO_UDP))) {
    coap_log(LOG_EMERG, "cannot create client session\n");
    goto finish;
  }

  /* coap_register_response_handler(ctx, response_handler); */
  coap_register_response_handler(ctx, [](auto, auto,
                                         const coap_pdu_t *received,
                                         auto) {
                                        have_response = 1;
                                        coap_show_pdu(LOG_WARNING, received);
                                        return COAP_RESPONSE_OK;
                                      });
  /* construct CoAP message */
  pdu = coap_pdu_init(COAP_MESSAGE_CON,
                      COAP_REQUEST_CODE_PUT,
                      coap_new_message_id(session),
                      coap_session_max_pdu_size(session));

  coap_add_token(pdu, 1, reinterpret_cast<const uint8_t *>("1"));
  



  if (!pdu) {
    coap_log( LOG_EMERG, "cannot create PDU\n" );
    goto finish;
  }

  /* add a Uri-Path option */
  coap_add_option(pdu, COAP_OPTION_URI_PATH, 3,
                  reinterpret_cast<const uint8_t *>("led"));
  

coap_add_data(pdu, 1, reinterpret_cast<const uint8_t *>(payload));

  coap_show_pdu(LOG_WARNING, pdu);
  /* and send the PDU */
    coap_send(session, pdu);
    return result;
    while (have_response == 0)
      coap_io_process(ctx, COAP_IO_WAIT);

    result = EXIT_SUCCESS;
  finish:

    coap_session_release(session);
    coap_free_context(ctx);
    coap_cleanup();
  
}

int main(int argc, char *argv[]){
    const char *device;
    int js;
    struct js_event event;
  if (argc > 1)
        device = argv[1];
  else
        device = "/dev/input/js2";

  js = open(device, O_RDONLY);

  if (js == -1)
        perror("Could not open joystick");

  while (read_event(js, &event) == 0)
    {
      if(event.number == 0 && event.value == 1){
        payload = "1";
        blink();
      }
      else {if(event.number == 0 && event.value == 0){
        payload = "0";
        blink();
      }
      }

    }
        
      fflush(stdout);
    

    close(js);
    return 0;

  
}
