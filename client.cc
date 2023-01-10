/* minimal CoAP client
 *
 * Copyright (C) 2018-2021 Olaf Bergmann <bergmann@tzi.org>
 */

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>

#include "common.hh"

static int have_response = 0;
uint8_t *data = NULL;
size_t data_len = 0;
const char* payload = "1";

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

int main(){
  while(1){
    payload = "1";
    blink();
    usleep(500000);
    payload = "0";
    blink();
    usleep(500000);    
  }
}
