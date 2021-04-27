/*
 * APD - Tema 1
 * Octombrie 2020
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>



char *in_filename_julia;
char *in_filename_mandelbrot;
char *out_filename_julia;
char *out_filename_mandelbrot;
int P;  // Numar de thread-uri

// declarare variabila barrier
pthread_barrier_t bariera;

// Variabile globale pentru matricea rezultat si dimensiunile acesteia
int width_julia, width_mandelbrot, height_julia, height_mandelbrot;
int **result_julia, **result_mandelbrot;

// structura pentru un numar complex
typedef struct _complex {
	double a;
	double b;
} complex;

// structura pentru parametrii unei rulari
typedef struct _params {
	int is_julia, iterations;
	double x_min, x_max, y_min, y_max, resolution;
	complex c_julia;
} params;

// Variabila globala pentru parametrii primiti din linia de comanda
params par_julia, par_madelbrot;

//Functie de calcul al minimului dintre 2 numere
int min(int num1, int num2) {
    return (num1 > num2 ) ? num2 : num1;
}

// citeste argumentele programului
void get_args(int argc, char **argv)
{
	if (argc < 6) {
		printf("Numar insuficient de parametri:\n\t"
				"./tema1 fisier_intrare_julia fisier_iesire_julia "
				"fisier_intrare_mandelbrot fisier_iesire_mandelbrot numar thread-uri\n");
		exit(1);
	}

	in_filename_julia = argv[1];
	out_filename_julia = argv[2];
	in_filename_mandelbrot = argv[3];
	out_filename_mandelbrot = argv[4];
	P = atoi(argv[5]);
}

// citeste fisierul de intrare
void read_input_file(char *in_filename, params* par)
{
	FILE *file = fopen(in_filename, "r");
	if (file == NULL) {
		printf("Eroare la deschiderea fisierului de intrare!\n");
		exit(1);
	}

	fscanf(file, "%d", &par->is_julia);
	fscanf(file, "%lf %lf %lf %lf",
			&par->x_min, &par->x_max, &par->y_min, &par->y_max);
	fscanf(file, "%lf", &par->resolution);
	fscanf(file, "%d", &par->iterations);

	if (par->is_julia) {
		fscanf(file, "%lf %lf", &par->c_julia.a, &par->c_julia.b);
	}

	fclose(file);
}

// scrie rezultatul in fisierul de iesire
void write_output_file(char *out_filename, int **result, int width, int height)
{
	int i, j;

	FILE *file = fopen(out_filename, "w");
	if (file == NULL) {
		printf("Eroare la deschiderea fisierului de iesire!\n");
		return;
	}

	fprintf(file, "P2\n%d %d\n255\n", width, height);
	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			fprintf(file, "%d ", result[i][j]);
		}
		fprintf(file, "\n");
	}

	fclose(file);
}

// aloca memorie pentru rezultat
int **allocate_memory(int width, int height)
{
	int **result;
	int i;

	result = malloc(height * sizeof(int*));
	if (result == NULL) {
		printf("Eroare la malloc!\n");
		exit(1);
	}

	for (i = 0; i < height; i++) {
		result[i] = malloc(width * sizeof(int));
		if (result[i] == NULL) {
			printf("Eroare la malloc!\n");
			exit(1);
		}
	}

	return result;
}

// elibereaza memoria alocata
void free_memory(int **result, int height)
{
	int i;

	for (i = 0; i < height; i++) {
		free(result[i]);
	}
	free(result);
}

// ruleaza algoritmul Julia
void run_julia(params *par, int **result, int width, int height, int id)
{
	int w, h, i;
	// Calcul arii de actionare a thread-urilor
	int start = id * (double)width / P;
	int end = min((id + 1) * (double)width / P, width);
	// Paralelizare
	for (w = start; w < end; w++) {
		for (h = 0; h < height; h++) {
			int step = 0;
			complex z = { .a = w * par->resolution + par->x_min,
							.b = h * par->resolution + par->y_min };

			while (sqrt(pow(z.a, 2.0) + pow(z.b, 2.0)) < 2.0 && step < par->iterations) {
				complex z_aux = { .a = z.a, .b = z.b };

				z.a = pow(z_aux.a, 2) - pow(z_aux.b, 2) + par->c_julia.a;
				z.b = 2 * z_aux.a * z_aux.b + par->c_julia.b;

				step++;
			}

			result[h][w] = step % 256;
		}
	}
	// Calcul de actionare a thread-urilor pentru a doua paralelizare
	int start1 = id * (double)height / 2*P;
	int end1 = min((id + 1) * (double)height / 2*P, height/2);
	/*
		Toate thread-urile trebuie sa fi terminat de scris in result deoarece
	matricea este utilizata mai departe si exista posibilitatea ca unele
	threaduri sa nu-si fi terminat treaba, pe cand altele da.
	Solutie: o bariera ce se deblocheaza cand toate thread-urile o ating!
	*/
	pthread_barrier_wait(&bariera);

	// Paralelizare transformare in coordonate ecran
	
	for (i = start1; i < end1; i++) {
		int *aux = result[i];
		result[i] = result[height - i - 1];
		result[height - i - 1] = aux;
	}
	
}

// ruleaza algoritmul Mandelbrot
void run_mandelbrot(params *par, int **result, int width, int height, int id)
{
	int w, h, i;
	// Calcul arii de actionare a thread-urilor
	int start = id * (double)width / P;
	int end = min((id + 1) * (double)width / P, width);

	// Paralelizare
	for (w = start; w < end; w++) {
		for (h = 0; h < height; h++) {
			complex c = { .a = w * par->resolution + par->x_min,
							.b = h * par->resolution + par->y_min };
			complex z = { .a = 0, .b = 0 };
			int step = 0;

			while (sqrt(pow(z.a, 2.0) + pow(z.b, 2.0)) < 2.0 && step < par->iterations) {
				complex z_aux = { .a = z.a, .b = z.b };

				z.a = pow(z_aux.a, 2.0) - pow(z_aux.b, 2.0) + c.a;
				z.b = 2.0 * z_aux.a * z_aux.b + c.b;

				step++;
			}

			result[h][w] = step % 256;
		}
		
	}
	// Calcul de actionare a thread-urilor pentru a doua paralelizare
	int start1 = id * (double)height / 2*P;
	int end1 = min((id + 1) * (double)(height / 2*P), height/2);

	/*
		Toate thread-urile trebuie sa fi terminat de scris in result deoarece
	matricea este utilizata mai departe si exista posibilitatea ca unele
	threaduri sa nu-si fi terminat treaba, pe cand altele da.
	Solutie: o bariera ce se deblocheaza cand toate thread-urile o ating!
	*/
	pthread_barrier_wait(&bariera);
	
	// Paralelizare transformare in coordonate ecran

	for (i = start1; i < end1; i++) {
		int *aux = result[i];
		result[i] = result[height - i - 1];
		result[height - i - 1] = aux;
	}
	
}
 
void *f(void *arg) {
	long id = *(long*) arg;
    // JULIA

	// Apel functie ce implementeaza algoritmul Julia
	run_julia(&par_julia, result_julia, width_julia, height_julia, id);

	/*
		Aceasta bariera are rolul de a impiedica thread-ul 0 sa scrie
	rezultatul in fisier si sa dezaloce memoria inainte ca toate thread-urile
	sa fi terminat de rulat functia Julia.
	*/
    pthread_barrier_wait(&bariera);
	if (id == 0) {
		write_output_file(out_filename_julia, result_julia, width_julia, height_julia);
		free_memory(result_julia, height_julia);

	}
	
	// MANDELBROT

	// Apel functie ce implementeaza algoritmul Mandelbrot
	run_mandelbrot(&par_madelbrot, result_mandelbrot, width_mandelbrot, height_mandelbrot, id);
	/*
		Aceasta bariera are rolul de a impiedica thread-ul 0 sa scrie
	rezultatul in fisier si sa dezaloce memoria inainte ca toate thread-urile
	sa fi terminat de rulat functia Mandelbrot.
	*/
    pthread_barrier_wait(&bariera);
	if (id == 0) {
		write_output_file(out_filename_mandelbrot, result_mandelbrot, width_mandelbrot, height_mandelbrot);
		free_memory(result_mandelbrot, height_mandelbrot);
	}

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	get_args(argc, argv);
	pthread_t threads[P];
    int r;
    long id;
    void *status;
    long arguments[P];
	// initializare variabila barrier
	pthread_barrier_init(&bariera, NULL, P);
	// DATE JULIA
	read_input_file(in_filename_julia, &par_julia);
	width_julia = (par_julia.x_max - par_julia.x_min) / par_julia.resolution;
	height_julia = (par_julia.y_max - par_julia.y_min) / par_julia.resolution;
	result_julia = allocate_memory(width_julia, height_julia);
	
	// DATE MANDELBROT
	read_input_file(in_filename_mandelbrot, &par_madelbrot);
	width_mandelbrot = (par_madelbrot.x_max - par_madelbrot.x_min) / par_madelbrot.resolution;
	height_mandelbrot = (par_madelbrot.y_max - par_madelbrot.y_min) / par_madelbrot.resolution;
	result_mandelbrot = allocate_memory(width_mandelbrot, height_mandelbrot);
	// Creare thread-uri
    for (id = 0; id < P; id++) {
        arguments[id] = id;
        r = pthread_create(&threads[id], NULL, f, (void *) &arguments[id]);
 
        if (r) {
            printf("Eroare la crearea thread-ului %ld\n", id);
            exit(-1);
        }
    }
 
    for (id = 0; id < P; id++) {
        r = pthread_join(threads[id], &status);
 
        if (r) {
            printf("Eroare la asteptarea thread-ului %ld\n", id);
            exit(-1);
        }
    }
	// dezalocare variabila barrier
	pthread_barrier_destroy(&bariera);
    pthread_exit(NULL);
	return 0;
}
