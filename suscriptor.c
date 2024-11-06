#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFFER_SIZE 256
#define MAX_PID_LENGTH 20
#define MAX_TOPIC_LENGTH 256
#define MENSAJE_SIZE (MAX_PID_LENGTH + MAX_TOPIC_LENGTH + 2)

int main(int argc, char *argv[]) {
    char *pipeSSC = NULL;

    // Process arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0)
            pipeSSC = argv[++i];
    }

    if (!pipeSSC) {
        fprintf(stderr, "Uso: %s -s <pipeSSC>\n", argv[0]);
        return 1;
    }

    // Request topics from user
    char topicos[MAX_TOPIC_LENGTH];
    printf("Ingrese los tópicos de interés (separados por comas): ");
    if (!fgets(topicos, sizeof(topicos), stdin)) {
        fprintf(stderr, "Error leyendo los tópicos\n");
        return 1;
    }
    topicos[strcspn(topicos, "\n")] = '\0';

    if (strlen(topicos) == 0) {
        fprintf(stderr, "Debe ingresar al menos un tópico.\n");
        return 1;
    }

    // Validate topic length
    if (strlen(topicos) >= MAX_TOPIC_LENGTH - 1) {
        fprintf(stderr, "Los tópicos ingresados son demasiado largos\n");
        return 1;
    }

    // Send PID and topics to system
    int fd_ssc = open(pipeSSC, O_WRONLY);
    if (fd_ssc == -1) {
        perror("Error al abrir pipeSSC en suscriptor");
        return 1;
    }

    char mensaje[MENSAJE_SIZE];
    int mensaje_len = snprintf(mensaje, MENSAJE_SIZE, "%d|%s", getpid(), topicos);
    if (mensaje_len >= MENSAJE_SIZE) {
        fprintf(stderr, "Error: El mensaje es demasiado largo\n");
        close(fd_ssc);
        return 1;
    }

    write(fd_ssc, mensaje, mensaje_len + 1);
    close(fd_ssc);

    // Create and open pipe for receiving news
    char pipe_suscriptor[BUFFER_SIZE];
    snprintf(pipe_suscriptor, sizeof(pipe_suscriptor), "pipeS_%d", getpid());
    mkfifo(pipe_suscriptor, 0666);

    int fd_sub = open(pipe_suscriptor, O_RDONLY);
    if (fd_sub == -1) {
        perror("Error al abrir el pipe del suscriptor");
        unlink(pipe_suscriptor);
        return 1;
    }

    printf("Suscriptor: Esperando noticias sobre: %s\n", topicos);

    // Read news loop
    char buffer[BUFFER_SIZE];
    while (1) {
        int bytes_read = read(fd_sub, buffer, sizeof(buffer));
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            if (strcmp(buffer, "FIN") == 0) {
                printf("Suscriptor: Fin de la emisión de noticias.\n");
                break;
            }
            printf("Suscriptor: Noticia recibida: %s\n", buffer);
        }
        sleep(1); // Avoid busy waiting
    }

    // Cleanup
    close(fd_sub);
    unlink(pipe_suscriptor);

    return 0;
}