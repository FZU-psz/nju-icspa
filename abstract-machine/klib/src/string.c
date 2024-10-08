#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  // panic("Not implemented");
  size_t len = 0;
  while (s[len] != '\0') {
    len++;
  }
  return len;
}

char *strcpy(char *dst, const char *src) {
  // panic("Not implemented");
  char * ret = dst;
  while ((*dst++ = *src++) != '\0');
  *dst = '\0';
  return ret;
}

char *strncpy(char *dst, const char *src, size_t n) {
  // panic("Not implemented");
  size_t i = 0;
  while (src[i] != '\0' && i < n) {
    dst[i] = src[i];
    i++;
  }
  while (i < n) {
    dst[i] = '\0';
    i++;
  }
  return dst;
}

char *strcat(char *dst, const char *src) {
  // panic("Not implemented");
  size_t i = 0;
  while (dst[i] != '\0') {
    i++;
  }
  size_t j = 0;
  while (src[j] != '\0') {
    dst[i] = src[j];
    i++;
    j++;
  }
  dst[i] = '\0';
  return dst;
}

int strcmp(const char *s1, const char *s2) {
  // panic("Not implemented");
  size_t i = 0;
  while (s1[i] != '\0' && s2[i] != '\0') {
    if (s1[i] < s2[i]) {
      return -1;
    } else if (s1[i] > s2[i]) {
      return 1;
    }
    i++;
  }
  if (s1[i] == '\0' && s2[i] == '\0') {
    return 0;
  } else if (s1[i] == '\0') {
    return -1;
  } else {
    return 1;
  }
}

int strncmp(const char *s1, const char *s2, size_t n) {
  // panic("Not implemented");
  size_t i = 0;
  while (s1[i] != '\0' && s2[i] != '\0' && i < n) {
    if (s1[i] < s2[i]) {
      return -1;
    } else if (s1[i] > s2[i]) {
      return 1;
    }
    i++;
  }
  if (i == n) {
    return 0;
  } else if (s1[i] == '\0') {
    return -1;
  } else {
    return 1;
  }
}

void *memset(void *s, int c, size_t n) {
  // panic("Not implemented");
  unsigned char *p = s; //unsingned char is used to avoid overflow 
  while(n--) {
    *p++ = (unsigned char)c;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  // panic("Not implemented");
  size_t i = 0;
  if (dst < src) { // normal copy 
    while (i < n) {
      ((char *)dst)[i] = ((char *)src)[i];
      i++;
    }
  } else { // avoid overlap
    i = n;
    while (i > 0) { //copy from tail to head 
      ((char *)dst)[i - 1] = ((char *)src)[i - 1];
      i--;
    }
  }
  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  // panic("Not implemented");
  size_t i = 0;
  while (i < n) {
    ((char *)out)[i] = ((char *)in)[i];
    i++;
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  // panic("Not implemented");
  size_t i = 0;
  while (i < n) {
    if (((char *)s1)[i] < ((char *)s2)[i]) {
      return -1;
    } else if (((char *)s1)[i] > ((char *)s2)[i]) {
      return 1;
    }
    i++;
  }
  return 0;
}

#endif
