//
// Created by mostafa on 17/11/2019.
//
#include <stdio.h>
#include <stdlib.h>
void DieWithError(char *errorMessage)
{
    perror (errorMessage) ;
    exit(1);
}
