int send_string(int sockfd, char *buffer, int length) {
	int sent_bytes, bytes_to_send;
  unsigned char *ptr = (unsigned char *)buffer;

    if (length == 0)
       bytes_to_send = strlen(buffer);
   else
    bytes_to_send = length;

       while(bytes_to_send > 0) {
          sent_bytes = send(sockfd, ptr, bytes_to_send, 0);

          if(sent_bytes == -1)
             return 0; // Вернуть 0 при ошибке передачи.

           bytes_to_send -= sent_bytes;
           ptr += sent_bytes;
	}
        return 1; // Вернуть 1 при успехе.
}

int recv_line(int sockfd, const unsigned char *dest_buffer) {

   #define EOL "\r\n" // Байты, завершающие строку
   #define EOL_SIZE 2

        unsigned char *ptr;
        int eol_matched = 0;

        ptr = (unsigned char *)dest_buffer;

        while(recv(sockfd, ptr, 1, 0) != -1) { // Прочитать один байт.

           if(*ptr == EOL[eol_matched]) {      // Входит ли он в EOL?
              eol_matched++;

              if (*ptr == '\0')
                return strlen((const char *)dest_buffer);

              if(eol_matched == EOL_SIZE) {    // Если все байты входят в EOL,
                 *(ptr+1-EOL_SIZE) = '\0';     // записать конец строки.

                 return strlen((const char *)dest_buffer);   // Вернуть кол-во принятых байтов
              }

           } else {
              eol_matched = 0;
}
           ptr++; // Установить указатель на следующий байт.
        }
        return 0; // Признак конца строки не найден.
}



// int recv_line(int sockfd, const unsigned char *dest_buffer) {
//      char eol[3] = "\r\n";
//         unsigned char *ptr;
//         int eol_matched = 0;
//         ptr = (unsigned char *)dest_buffer;
//         while(recv(sockfd, ptr, 1, 0) != -1) {
//             if(*ptr == eol[eol_matched]) {
//              eol_matched++;
//              if (*ptr == '\0')
//                  return strlen((const char *)dest_buffer);
//                 if(eol_matched == 2) {
//                  *(ptr-1) = '\0';
//                  return strlen((const char *)dest_buffer);
//               }
//            }
//            else
//               eol_matched = 0;
//            ptr++;
//         }
//         return 0;
// }
