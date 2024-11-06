
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 256

int main(int argc, char *argv[]) {
    if (argc != 7) {  // Cambiado a 7 para aceptar tres pares de argumentos
        fprintf(stderr, "Uso: %s -p <pipe> -f <archivo_not> -t <intervalo>\n", argv[0]);
        return 1;
    }

    char *pipe = NULL;
    char *archivo_not = NULL;
    int intervalo = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0) {
            pipe = argv[++i];
        } else if (strcmp(argv[i], "-f") == 0) {
            archivo_not = argv[++i];
        } else if (strcmp(argv[i], "-t") == 0) {
            intervalo = atoi(argv[++i]);
        }
    }
    int fd = open(pipe, O_WRONLY);
    if (fd == -1) {
        perror("Error al abrir el pipe del publicador");
        return 1;
    }

    FILE *file = fopen(archivo_not, "r");
    if (!file) {
        perror("Error al abrir el archivo de noticias");
        close(fd);
        return 1;
    }

    char buffer[BUFFER_SIZE];
    while (fgets(buffer, sizeof(buffer), file)) {
        write(fd, buffer, strlen(buffer) + 1);
        printf("Publicador: Noticia enviada: %s\n", buffer);
        sleep(intervalo);  // Si se requiere un intervalo entre noticias
    }

    // Opcional: permitir al usuario ingresar más noticias
    while (1) {
        printf("Ingrese una nueva noticia o 'FIN' para terminar: ");
        fgets(buffer, sizeof(buffer), stdin);
        if (strcmp(buffer, "FIN\n") == 0) break;
        write(fd, buffer, strlen(buffer) + 1);
        printf("Publicador: Noticia enviada: %s\n", buffer);
    }

    fclose(file);
    // No cerrar el pipe aquí
    // close(fd);
    return 0;
}


