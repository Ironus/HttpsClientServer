#include <stdio.h>
#include <string>
#include <curl/curl.h>

int main(int argc, char* argv[]) {
  // utworz zmienne do przechowywania polaczenia i odpowiedzi
  CURL *curl;
  CURLcode res;
  // zainicjuj cUrl (dodaje tez rzeczy odpowiedzialne za sockety)
  curl_global_init(CURL_GLOBAL_DEFAULT);
  // zacznij sesje polaczenia i zwroc uchwyt
  curl = curl_easy_init();
  // sprawdz poprawnosc
  if(curl != NULL) {
    // jesli ok, to ustaw adres serwera
    curl_easy_setopt(curl, CURLOPT_URL, "https://localhost:8080/");
    // przygotuj zapytanie
    std::string postLeft = "number=";
    std::string postRight = argv[1];
    std::string postBody = postLeft + postRight;
    // ustaw metode na POST
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postBody.c_str());
    // wylacz sprawdzanie certyfikatu SSL (usunac jesli certyfikat ma podpis CA)
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    // wyslij zapytanie i odbierz kod bledu
    res = curl_easy_perform(curl);
    // sprawdz czy wszystko ok
    if(res != CURLE_OK)
      // jesli nie, to zwroc blad
      fprintf(stderr, "Blad zapytania: %s\n", curl_easy_strerror(res));
    // zwolnij pamiec na uchwycie
    curl_easy_cleanup(curl);
  }
  // zwolnij cala pamiec
  curl_global_cleanup();

  return 0;
}
