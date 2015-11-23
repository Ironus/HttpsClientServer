#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <microhttpd.h>

// port na ktorym nasluchuje serwer
#define PORT 8080

// funkcja odpowiedzialna za wyslanie odpowiedzi do klienta
int answer_to_connection(void *cls, struct MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls) {
  // strona, ktora zostanie wyslana do klienta
  const char *page = "<html><body>Hello, browser!</body></html>";
  struct MHD_Response *response;
  int ret;

  // zmienna do przetrzymania klienta
  static int aptr;

  // sprawdzenie czy metoda to GET czy POST
  if (0 != strcmp (method, "GET"))
  // jesli POST zwroc blad
    return MHD_NO;
  // przetrzymaj klienta
  if (&aptr != *con_cls) {
    *con_cls = &aptr;
    return MHD_YES;
  }
  // wyzeruj wskaznik con_cls
  con_cls = NULL;

  // przygotuj odpowiedz
  response = MHD_create_response_from_buffer(strlen (page), (void*) page, MHD_RESPMEM_PERSISTENT);
  ret = MHD_queue_response(connection, MHD_HTTP_OK, response);

  // zwolnij pamiec
  MHD_destroy_response(response);
  // zwroc odpowiedz
  return ret;
}

int main(int argc, char* argv[]) {
  struct MHD_Daemon *daemon;
  daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL, &answer_to_connection, NULL, MHD_OPTION_END);

  if(NULL == daemon)
    return 1;

  getchar();
  MHD_stop_daemon(daemon);

  return 0;
}
