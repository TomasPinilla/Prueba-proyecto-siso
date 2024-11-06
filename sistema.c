#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define BUFFER_SIZE 256
#define MAX_SUSCRIPTORES 100

typedef struct {
    char topicos[BUFFER_SIZE];
    char pipe_nombre[BUFFER_SIZE];
    int pipe_fd;
} Suscriptor;

void crear_pipe_si_no_existe(const char *nombre_pipe) {
    printf("Creando pipe: %s\n", nombre_pipe);
    fflush(stdout);
    
    if (access(nombre_pipe, F_OK) == -1) {
        if (mkfifo(nombre_pipe, 0666) == -1) {
            perror("Error al crear el pipe");
            exit(1);
        }
        printf("Pipe creado: %s\n", nombre_pipe);
    } else {
        printf("Pipe ya existe: %s\n", nombre_pipe);
    }
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    char *pipePSC = NULL;
    char *pipeSSC = NULL;
    int timeF = 0;

    // Procesar argumentos
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0)
            pipePSC = argv[++i];
        else if (strcmp(argv[i], "-s") == 0)
            pipeSSC = argv[++i];
        else if (strcmp(argv[i], "-t") == 0)
            timeF = atoi(argv[++i]);
    }
    fflush(stdout);

    if (!pipePSC || !pipeSSC || timeF <= 0) {
        fprintf(stderr, "Uso: %s -p <pipePSC> -s <pipeSSC> -t <timeF>\n", argv[0]);
        return 1;
    }

    // Open pipes in blocking mode
    int pub_fd = open(pipePSC, O_RDONLY);
    if (pub_fd == -1) {
        perror("Error al abrir pipePSC");
        return 1;
    }
    printf("PipePSC abierto (fd=%d)\n", pub_fd);

    int sub_fd = open(pipeSSC, O_RDONLY);
    if (sub_fd == -1) {
        perror("Error al abrir pipeSSC");
        close(pub_fd);
        return 1;
    }
    printf("PipeSSC abierto (fd=%d)\n", sub_fd);
    
    printf("Sistema: Esperando conexiones...\n");
    fflush(stdout);

    Suscriptor suscriptores[MAX_SUSCRIPTORES];
    int num_suscriptores = 0;

    char buffer[BUFFER_SIZE];
    int publicadores_activos = 0;
    time_t tiempo_sin_publicadores = 0;

    while (1) {
        // Leer mensajes de los suscriptores
        int bytes_read = read(sub_fd, buffer, sizeof(buffer));
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';

            // Parsear el mensaje del suscriptor: "PID|topicos"
            char *pid_str = strtok(buffer, "|");
            char *topicos = strtok(NULL, "|");

            if (pid_str && topicos && num_suscriptores < MAX_SUSCRIPTORES) {
                // Crear un pipe único para el suscriptor
                char pipe_suscriptor[BUFFER_SIZE];
                snprintf(pipe_suscriptor, sizeof(pipe_suscriptor), "pipeS_%s", pid_str);
                crear_pipe_si_no_existe(pipe_suscriptor);

                // Abrir el pipe para escritura
                int fd_suscriptor = open(pipe_suscriptor, O_WRONLY);
                if (fd_suscriptor == -1) {
                    perror("Error al abrir el pipe del suscriptor");
                    continue;
                }

                // Almacenar información del suscriptor
                strcpy(suscriptores[num_suscriptores].topicos, topicos);
                strcpy(suscriptores[num_suscriptores].pipe_nombre, pipe_suscriptor);
                suscriptores[num_suscriptores].pipe_fd = fd_suscriptor;
                num_suscriptores++;

                printf("Sistema: Nuevo suscriptor (%s) registrado con topicos: %s\n", pid_str, topicos);
            }
        }

        // Leer noticias de los publicadores
        bytes_read = read(pub_fd, buffer, sizeof(buffer));
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("Sistema: Noticia recibida: %s\n", buffer);
            publicadores_activos = 1;
            tiempo_sin_publicadores = 0;

            // Enviar la noticia a los suscriptores correspondientes
            for (int i = 0; i < num_suscriptores; i++) {
                if (strstr(buffer, suscriptores[i].topicos)) {
                    write(suscriptores[i].pipe_fd, buffer, strlen(buffer) + 1);
                }
            }
        } else {
            // No hay noticias; verificar si debemos esperar o finalizar
            if (publicadores_activos) {
                tiempo_sin_publicadores += 1;
                if (tiempo_sin_publicadores >= timeF) {
                    // Enviar mensaje de fin a los suscriptores
                    for (int i = 0; i < num_suscriptores; i++) {
                        write(suscriptores[i].pipe_fd, "FIN", strlen("FIN") + 1);
                        close(suscriptores[i].pipe_fd);
                        unlink(suscriptores[i].pipe_nombre);
                    }
                    printf("Sistema: Tiempo de espera excedido. Finalizando.\n");
                    break;
                }
            }
        }

        // Pequeño descanso para evitar alto uso de CPU
        sleep(1);
    }

    // Cerrar y eliminar pipes
    close(pub_fd);
    close(sub_fd);
    unlink(pipePSC);
    unlink(pipeSSC);

    return 0;
}