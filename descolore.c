#include <png.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

png_bytep *rows;
png_infop info_ptr;
png_uint_32 width;
png_uint_32 height;

typedef struct no *ptno;
typedef struct no
{
    int i, j;
    ptno next;
} no;

struct fila
{
    int tamanho;
    no *inicio;
    no *fim;
};
typedef struct fila fila;

void inicializar(fila *f)
{
    f->tamanho = 0;
    f->inicio = NULL;
    f->fim = NULL;
}

int filaVazia(fila *f)
{
    return (f->fim == NULL || f->inicio == NULL);
}

void insereFila(fila *f, int i,int j)
{
    no *tmp;
    tmp = malloc(sizeof(no));
    tmp->i = i;
    tmp->j = j;
    tmp->next = NULL;
    if(!filaVazia(f))
    {
        f->fim->next = tmp;
        f->fim = tmp;
    }
    else
    {
        f->inicio = f->fim = tmp;
    }
    f->tamanho++;
}

int removeFila(fila *f,int *i,int *j)
{
    if(filaVazia(f)){
        return 1;
    }
    no *tmp;
    tmp = f-> inicio;
    *i = f->inicio->i;
    *j = f->inicio->j;
    f->inicio = f->inicio->next;
    f->tamanho--;
    free(tmp);
    return 0;
}

/*-----------------------------------------------
 *  init   *Q      new     init         *Q
 *  [a:]->[b:]-+   [c:]  ->[a:]->[b:]->[c:]-+
 *   ^---------+             ^--------------+
 *----------------------------------------------*/
void insQ(ptno *Q, int i, int j)
{
    ptno new = malloc(sizeof(no));
    new->i = i;
    new->j = j;
    if (!(*Q))
        new->next = new;
    else
    {
        new->next = (*Q)->next;
        (*Q)->next = new;
    }
    *Q = new;
}

/*-----------------------------------------------
 *  init         *Q                    *Q
 *  [a:]->[b:]->[c:]-+    [a:]  [b:]->[c:]-+
 *    ^--------------+            ^--------+
 *----------------------------------------------*/
void remQ(ptno *Q, int *i, int *j)
{
    ptno init = (*Q)->next;
    *i = init->i;
    *j = init->j;
    if (*Q == init)
        *Q = NULL;
    else
        (*Q)->next = init->next;
    free(init);
}

int isEmpty(ptno *Q)
{
    return *Q == NULL;
}

void initQPrior(ptno *QPrior, int mn)
{
    int i;
    for (i = 0; i < mn; i++)
        QPrior[i] = NULL;
}

void insert(ptno *QPrior, int i, int j, int p)
{
    insQ(QPrior + p, i, j);
}

int pop(ptno *QPrior, int *i, int *j, int *maxPrior, int mn)
{
    while (*maxPrior < mn && isEmpty(QPrior + *maxPrior))
        (*maxPrior)++;
    if (*maxPrior == mn)
        return 1;
    remQ(QPrior + *maxPrior, i, j);
    return 0;
}

typedef int *image;

image img_alloc()
{
    return (image)malloc(height * width * sizeof(int));
}

int img_free(image Img)
{
    free(Img);
}

static void
fatal_error(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    vfprintf(stderr, message, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

void read_png_file(char *filename)
{
    FILE *fp;
    int bit_depth;
    int color_type;
    int interlace_method;
    int compression_method;
    int filter_method;
    fp = fopen(filename, "rb");
    if (!fp)
    {
        fatal_error("Cannot open '%s': %s\n", filename, strerror(errno));
    }
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
    {
        fatal_error("Cannot create PNG read structure");
    }
    info_ptr = png_create_info_struct(png_ptr);
    if (!png_ptr)
    {
        fatal_error("Cannot create PNG info structure");
    }
    png_init_io(png_ptr, fp);
    png_read_png(png_ptr, info_ptr, 0, 0);
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
                 &color_type, &interlace_method, &compression_method,
                 &filter_method);
    rows = png_get_rows(png_ptr, info_ptr);
    fclose(fp);
}

void write_png_file(char *filename)
{
    FILE *fp;
    int length = strlen(filename) + 4;
    char new_filename[length];
    int end = strlen(filename) - 4;
    strncpy(new_filename, filename, end);
    new_filename[end] = 0;
    strcat(new_filename, "_new.png");
    fp = fopen(new_filename, "wb");
    if (!fp)
    {
        fatal_error("Cannot open '%s': %s\n", filename, strerror(errno));
    }
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png)
    {
        fatal_error("Cannot create PNG read structure");
    }
    png_init_io(png, fp);
    png_write_info(png, info_ptr);
    png_write_image(png, rows);
    png_write_end(png, NULL);
    fclose(fp);
}

void gradient(image In, image Out, int raio)
{
    int i, j, y, x, max, min;
    for (i = 0; i < height; i++)
        for (j = 0; j < width; j++)
        {
            max = -1;
            min = 255 + 1;
            for (y = -raio; y <= raio; y++)     // -1 0 1
                for (x = -raio; x <= raio; x++) // -1 0 1
                {
                    int pi = i + y;
                    int pj = j + x;
                    if (pi >= 0 && pi < height && pj >= 0 && pj < width)
                    {
                        if (abs(x) + abs(y) <= raio && In[pi * width + pj] > max)
                            max = In[pi * width + pj];
                        if (abs(x) + abs(y) <= raio && In[pi * width + pj] < min)
                            min = In[pi * width + pj];
                    }
                }
            Out[i * width + j] = max - min;
        }
    img_free(In);
}

/*------------------------------------------------
 * Calcula o Watershed da imagem
 *------------------------------------------------*/
void watershed(image In, image Out, int y, int x)
{

    int i, j, k, maxPrior = 0, stop = 0;
    ptno qPrior[255];
    image mark = img_alloc();
    enum
    {
        NONE,
        QUEUE,
        WSHED,
        MARK1,
        MARK2,
    };
    struct
    {
        int i, j;
    } n4[4] = {0, 1, 1, 0, 0, -1, -1, 0};
    initQPrior(qPrior, 255);
    // Inicialização dos marcadores
    //-----------------------------
    for (i = 0; i < height * width; i++)
        mark[i] = NONE;
    // MARK1 = (y, x) - centro da região

    int raio = 10;
    for (i = -raio; i <= raio; i++)
        for (j = -raio; j <= raio; j++)
        {
            int pi = i + y;
            int pj = j + x;
            if (abs(i) + abs(j) <= raio)
                mark[pi * width + pj] = MARK1;
        }

    //mark[y * width + x] = MARK1;

    // MARK2 = borda da imagem
    for (i = 0; i < height; i++)
    {
        mark[i * width] = MARK2;
        mark[i * width + width - 1] = MARK2;
    }
    for (j = 0; j < width; j++)
    {
        mark[j] = MARK2;
        mark[(height - 1) * width + j] = MARK2;
    }

    // Inicialização
    //--------------
    // Todos os vizinhos dos marcadores são colocados na fila
    // A prioridade é o nível de cinza do pixel
    for (i = 1; i < height - 1; i++)
        for (j = 1; j < width - 1; j++)
            if (mark[i * width + j] == NONE)
            {
                int isAdj = 0;
                for (k = 0; k < 4; k++)
                {
                    int pi = i + n4[k].i;
                    int pj = j + n4[k].j;
                    int m = mark[pi * width + pj];
                    if (m == MARK1 || m == MARK2)
                        isAdj = 1;
                }
                if (isAdj)
                {
                    mark[i * width + j] = QUEUE;
                    insert(qPrior, i, j, In[i * width + j]);
                }
            }

    // Crescimento dos Marcadores
    //---------------------------
    // Enquanto a fila de prioridade não está vazia faça
    // 1. retira um pixel da fila
    // 2. Se o pixel eh vizinho de UMA marca, recebe essa marca.
    //    Seus vizinhos sem rotulo são colocados na fila
    // 3. Se o pixel eh vizinho de DUAS marcas, ele eh WATERSHED
    while (!stop)
    {
        int m = NONE;
        int isWshed = 0;
        stop = pop(qPrior, &i, &j, &maxPrior, 255);
        if (!stop)
        {
            for (k = 0; k < 4; k++)
            {
                int pi = i + n4[k].i;
                int pj = j + n4[k].j;
                if (pi >= 0 && pi < height && pj >= 0 && pj < width)
                {
                    int mAdj = mark[pi * width + pj];
                    if (mAdj == MARK1 || mAdj == MARK2)
                    {
                        if (m == NONE)
                            m = mAdj;
                        else if (m != mAdj)
                            isWshed = 1;
                    }
                }
            }
            if (isWshed)
                mark[i * width + j] = WSHED;
            else
            {
                mark[i * width + j] = m;
                for (k = 0; k < 4; k++)
                {
                    int pi = i + n4[k].i;
                    int pj = j + n4[k].j;
                    if (pi >= 0 && pi < height && pj >= 0 && pj < width)
                        if (mark[pi * width + pj] == NONE)
                        {
                            int prior, px;
                            mark[pi * width + pj] = QUEUE;
                            px = In[pi * width + pj];
                            prior = (px < maxPrior) ? maxPrior : px;
                            insert(qPrior, pi, pj, prior);
                        }
                }
            }
        }
    }


    fila *f;
    f = malloc(sizeof(fila));
    inicializar(f);
    i = height / 2;
    j = width / 2;
    insereFila(f,i, j);
    mark[i * width + j] = WSHED;
    stop = 0;
    while (!stop)
    {
        int isWshed = 0;
        stop = removeFila(f,&i,&j);
        if (!stop)
        {
            for (k = 0; k < 4; k++)
            {
                int pi = i + n4[k].i;
                int pj = j + n4[k].j;
                if (pi >= 0 && pi < height && pj >= 0 && pj < width)
                {
                    int mAdj = mark[pi * width + pj];
                    if (mAdj != WSHED)
                    {
                        mark[pi * width + pj] = WSHED;
                        insereFila(f,pi,pj);
                    }
                }
            }
        }
    }

    for (i = 0; i < height * width; i++)
        Out[i] = (mark[i] == WSHED) ? 255 : 0;
    img_free(mark);
}

void getGreyscale(image Out)
{
    for (int j = 0; j < height; j++)
    {
        int i;
        png_bytep row;
        row = rows[j];
        for (i = 0; i < width; i++)
        {
            png_bytep px = &(row[i * 3]);
            double r = px[0], g = px[1], b = px[2];
            double grey = ((0.3 * r) + (0.59 * g) + (0.11 * b));
            Out[j * width + i] = grey;
        }
    }
}

void getPng(image Out)
{
    for (int j = 0; j < height; j++)
    {
        int i;
        png_bytep row;
        row = rows[j];
        for (i = 0; i < width; i++)
        {
            if (Out[j * width + i] > 0)
            {
                png_bytep px = &(row[i * 3]);
                double r = px[0], g = px[1], b = px[2];
                double grey = ((0.3 * r) + (0.59 * g) + (0.11 * b));
                for (int x = 0; x < 3; x++)
                {
                    px[x] = grey;
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    if(!argv[1]){
        printf("\
Watershed\n\
---------\n\
    USO: ./descolore <img.png>\n\
    ONDE: <img.png> = Arquivo de imagem em formato PNG.\n");
        return 0;
    }
    char *filename = argv[1];
    read_png_file(filename);
    printf("Width is %d, height is %d\n", width, height);
    image Grey,Grd,Out;
    Grey = img_alloc();
    Grd = img_alloc();
    getGreyscale(Grey);
    gradient(Grey, Grd, 1);
    Out = img_alloc();
    watershed(Grd, Out, height / 2, width / 2);
    getPng(Out);
    img_free(Grd);
    img_free(Out);
    write_png_file(filename);
    return 0;
}