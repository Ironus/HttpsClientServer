#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <sstream>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <microhttpd.h>

// port na ktorym nasluchuje serwer
#define PORT 8080
// rozmiar bufora POST
#define POSTBUFFERSIZE 512
// maksymalna dlugosc pola
#define MAXNUMBERSIZE 20
// maksymalna dlugosc odpowiedzi
#define MAXANSWERSIZE 512
// zamiana rodzajow polaczen na odpowiednie INTy
#define GET 0
#define POST 1
//scieka do klucza i certyfikatu rsa
#define SERVERKEYFILE "server.key"
#define SERVERCERTFILE "server.crt"

// strona, ktora zostanie wyslana do klienta przy nawiazaniu polaczenia
const char* askPage = "Serwer obsluguje jedynie zadania POST";
// strona z komunikatem bledu
const char* errorPage = "Cos poszlo nie tak.";
// strona sukcesu
const char* successPage = "Wynik: %s";

// struktura do przechowywania polaczenia i odpowiedzi dla niego
struct connectionInfo {
  int connectionType;
  char* factorial;
  struct MHD_PostProcessor* postprocessor;
};

// funkcja do obliczania silni
char* getFactorial(const char* _factorial) {
  char* result = new char[MAXANSWERSIZE];
  int factorial = 1;
  for(int i = 1; i <= atoi(_factorial); i++) {
    factorial *= i;
  }

  std::stringstream strs;
  strs << factorial;
  std::string tempString;
  tempString = strs.str();
  strcpy(result, tempString.c_str());

  return result;
}

static int sendPage (struct MHD_Connection *connection, const char *page) {
  int ret;
  struct MHD_Response *response;
  response = MHD_create_response_from_buffer (strlen (page), (void *) page, MHD_RESPMEM_PERSISTENT);
  if (!response)
    return MHD_NO;
  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);
  return ret;
}

// funkcja wywolywana po zakonczeniu polaczenia (z sukcesem badz bez) do zwolnienia pamieci
void requestCompleted (void *cls, struct MHD_Connection *connection, void **con_cls, enum MHD_RequestTerminationCode toe) {
  // utworz obiekt struktury do przechowywania polaczenia
  connectionInfo* con_info = static_cast<connectionInfo*>(*con_cls);
  // sprawdz poprawnosc
  if(con_info == NULL)
    return;
  // sprawdz rodzaj polaczenia
  if(con_info->connectionType == POST) {
    // zwolnij postprocesor
    MHD_destroy_post_processor (con_info->postprocessor);
    // sprawdz czy zostala obliczona silnia
    if(con_info->factorial)
      delete(con_info->factorial);
  }
  // zwolnij obiekt struktury polaczenia
  delete(con_info);
  // wpisz NULL do wskaznika
  *con_cls = NULL;
}

// funkcja odpowiedzialna za wczytanie calego polecenia POST
static int iteratePost (void* coninfo_cls, enum MHD_ValueKind kind, const char* key, const char* filename, const char* content_type, const char* transfer_encoding, const char* data, uint64_t off, size_t size) {
  // utworz obiekt struktury connectionInfo
  connectionInfo* con_info = static_cast<connectionInfo*>(coninfo_cls);
  // sprawdz czy zostaly wprowadzone dane do formularza
  if(strcmp (key, "number") == 0) {
    if((size > 0) && (size <= MAXNUMBERSIZE)) {
      // utworz wskaznik do zmiennej przechowujacej silnie
      char* factorial = new char[MAXANSWERSIZE];
      // sprawdz poprawnosc utworzenia wskaznika
      if(!factorial)
        return MHD_NO;
      // oblicz silnie i przepisz do bufora
      snprintf(factorial, MAXANSWERSIZE, successPage, getFactorial(data));
      // przepisz silnie do struktury
      con_info->factorial = factorial;
    } else
      // jesli blad rozmiaru, wpisz NULL do silni
      con_info->factorial = NULL;
    return MHD_NO;
  }
  return MHD_YES;
}

// funkcja odpowiedzialna za wyslanie odpowiedzi do klienta
int answerToConnection(void *cls, MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls) {
  // sprawdz czy odpowiedz jest pusta
  if(*con_cls == NULL) {
    // stworz obiekt stukruty polaczenia
    connectionInfo *con_info;
    // zarezerwuj pamiec na dane w strukturze
    con_info = new connectionInfo;
    // sprawdz czy polaczenie prawidlowe
    if(con_info == NULL)
      // jesli nie, to nie otwieraj polaczenia
      return MHD_NO;
    // wpisz NULL jako silnie
    con_info->factorial = NULL;
    // sprawdz metode polaczenia
    if(strcmp(method, "POST") == 0) {
      // jesli POST, to stworz postprocesor
      con_info->postprocessor = MHD_create_post_processor(connection, POSTBUFFERSIZE, iteratePost, (void*)con_info);
      // sprawdz poprawnosc utworzenia postprocesora
      if (con_info->postprocessor == NULL) {
        // jesli blad, zwolnij obiekt struktury polaczenia
        delete(con_info);
        // i nie otwieraj polaczenia
        return MHD_NO;
      }
      // jesli wszystko ok, to zapisz w strukturze jako typ polaczenia POST
      con_info->connectionType = POST;
    } else if(strcmp(method, "GET") == 0) {
      // jezeli metoda GET, to wpisz GET do obiektu polaczenia
      con_info->connectionType = GET;
    }
    // przepisz obiekt polaczenia jako obiekt wyjsciowy
    *con_cls = (void*)con_info;
    // otworz polaczenie
    return MHD_YES;
  }
  // jesli zapytanie GET, to wyslij formularz
  if(strcmp (method, "GET") == 0) {
    return sendPage(connection, askPage);
  }
  // jesli zapytanie POST
  if(strcmp (method, "POST") == 0) {
    // stworz obiekt polaczenia
    connectionInfo* con_info = static_cast<connectionInfo*>(*con_cls);
    // sprawdz czy sa jakies dane przychodzace
    if(*upload_data_size != 0) {
      // odczytaj dane
      MHD_post_process(con_info->postprocessor, upload_data, *upload_data_size);
      // ustaw na 0, zeby powiadomic, ze odebrano wszystkie dane
      *upload_data_size = 0;
      // otworz polaczenie
      return MHD_YES;
    // sprawdz czy silnia do obliczenia nie jest zerem
    } else if(con_info->factorial != NULL)
    // jesli tak, to wyslij odpowiednia strone
    return sendPage(connection, con_info->factorial);
  }
  // jezeli zadanie ani POST ani GET, to zwroc blad
  return sendPage(connection, errorPage);
}

// funckja do pobierania rozmiaru pliku
static long getFileSize (const char* _filename) {
  // wskaznik na plik
  FILE *fp = NULL;
  // otworz plik
  fp = fopen (_filename, "rb"); // do odczytu binarnie
  // jesli otworzono prawidlowo
  if (fp != NULL) {
    // zmienna na rozmiar pliku
    long size;
    // jesli blad czytania rozmiaru badz koniec pliku
    if ((fseek (fp, 0, SEEK_END) != 0) || ((size = ftell (fp)) == -1))
      // to wpisz jako rozmiar 0
      size = 0;
    // zamknij plik
    fclose (fp);
    // zwroc rozmiar
    return size;
  }
  // jesli blad otwierania to zwroc 0
  return 0;
}

// funkcja do ladowania pliku z certyfikatem
static char* loadFile(const char* _filename) {
  // wskaznik na plik
  FILE* fp = NULL;
  // bufor
  char* buffer = NULL;
  // rozmiar pliku
  long size = getFileSize(_filename);
  // sprawdz czy rozmiar ok
  if (size == 0)
    return NULL;
  // otworz plik
  fp = fopen(_filename, "rb"); // czytaj binarnie
  // sprawdz czy otworzono poprawnie
  if(fp == NULL)
    return NULL;
  // zarezerwuj pamiec na bufor
  buffer = new char[size];
  // sprawdz czy zarezerwowano popranie
  if (buffer == NULL) {
    // jesli nie, to zamknij plik
    fclose(fp);
    return NULL;
  }
  // przeczytaj plik i spradz poprawnosc
  if(size != fread (buffer, 1, size, fp)) {
    // zwolnij bufor
    free (buffer);
    // ustaw wskaznik na NULL
    buffer = NULL;
  }
  // zamknij plik
  fclose (fp);
  // zwroc bufor
  return buffer;
}

int main(int argc, char* argv[]) {
  // utworz obiekt zarzadzajacy polaczeniem
  MHD_Daemon *daemon;
  // utworz zmienne przetrzymujace certyfikat i klucz publiczny
  char* key_pem;
  char* cert_pem;
  // zaladuj certyfikat i klucz publiczny
  key_pem = loadFile(SERVERKEYFILE);
  cert_pem = loadFile(SERVERCERTFILE);
  // sprawdz czy zaladowano poprawnie
  if((key_pem == NULL) || (cert_pem == NULL)) {
    printf ("Blad ladowania certyfikatu/klucza publicznego.\n");
    return 1;
  }
  // wystartuj serwer
  daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY | MHD_USE_SSL, PORT, NULL, NULL, &answerToConnection, NULL, MHD_OPTION_HTTPS_MEM_KEY, key_pem, MHD_OPTION_HTTPS_MEM_CERT, cert_pem, MHD_OPTION_NOTIFY_COMPLETED, &requestCompleted, NULL, MHD_OPTION_END);

  // sprawdz czy poprawnie serwer poprawnie wystartowal
  if(NULL == daemon) {
    // jesli nie, to wywal blad i zwolnij pamiec
    printf("Blad tworzenia serwera.\n");
    free(key_pem);
    free(cert_pem);
    return 1;
  }
  // czekaj na klawisz, aby zatrzymac serwer
  getchar();
  // zatrzymaj serwer
  MHD_stop_daemon(daemon);
  // zwolnij pozostala pamiec
  free(key_pem);
  free(cert_pem);

  return 0;
}
