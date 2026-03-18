#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#define MAX_LEN 64

typedef struct {
    char service[MAX_LEN];
    char username[MAX_LEN];
    char password[MAX_LEN];
} Credential;

#endif // CREDENTIALS_H
