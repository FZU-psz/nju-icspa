#include <am.h>
// #include <cstdarg>
#include <klib-macros.h>
#include <klib.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}

int sprintf(char *out, const char *fmt, ...) {
  // panic("Not implemented");
  // write formated str into out buffer
  va_list args;
  va_start(args, fmt); // get args from fmt str
  int len = 0;
  while (*fmt) {
    if (*fmt == '%') {
      fmt++;
      switch (*fmt) {
      case 'd': {
        int num = va_arg(args, int);
        // convert int to str
        char num_str[32];
        int tail=0;
        while(num){ // store num in reverse order
          num_str[tail++] = num%10 + '0';
          num /= 10;
        }
        while(--tail>=0){ // copy str to out
          *out++ = num_str[tail];
          len++; //
        }
        break;
      }
      case 's': {
        char *str = va_arg(args, char *);
        while (*str!='\0'){
          *out++ = *str++; 
          len++;
        };
        break;
      }
      default:
        break;
      }
    } else {
      *out = *fmt;
      out++;
      len++;
    }
    fmt++;
  }
  *out = '\0';
  len++;
  va_end(args);
  return len;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
