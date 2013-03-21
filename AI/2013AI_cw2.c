/*
 * This is 2013AI_cw2.c by Pengyu CHEN(cpy.prefers.you@gmail.com)
 * As the coursework of Artificial Intelligence, 2013.02-2013.04
 * COPYLEFT, ALL WRONGS RESERVED.
 */

#include <unistd.h>
#include <GL/freeglut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <FreeImage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef unsigned long long u64_t;
typedef unsigned long u32_t;
typedef unsigned short u16_t;
typedef unsigned char u8_t;
typedef int i32_t;
typedef short i16_t;
typedef float f32_t;

typedef struct _confit_t
{
    // int population_size; // hardcoded as 2
    // int polygon_n; // hardcoded as 3
    int polygon_size; 
    char *log_path_pref;
    int log_interval;
}   config_t;

static config_t config = {
    .polygon_size = 256,
    .log_path_pref = "/var/tmp/",
    .log_interval = 10,
};

typedef struct _gene_t
{
    f32_t (*vertex)[2];
    u8_t (*color)[4];
    u8_t *pixel;
    u64_t fitness;
    u64_t mutation_cnt;
    u64_t evolution_cnt;
}   gene_t;

static gene_t gene_best;
static gene_t gene_curr;

typedef struct misc_t
{
    char *img_filename;
    FREE_IMAGE_FORMAT fmt;
    FIBITMAP *img;
    u32_t img_w;
    u32_t img_h;
    int win_orig;
    int win_best;
    int win_curr;
    char *log_path;
    FILE *log_file;
}   misc_t;

static misc_t misc;

static inline void log_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char *tmp;
    vasprintf(&tmp, fmt, args);
    fputs(tmp, misc.log_file);
    fputs(tmp, stdout);
    free(tmp);
    return;
}

static void draw_win_orig()
{
    void *ptr = FreeImage_GetBits(misc.img);
//    glutSetWindow(misc.win_orig);
    glDrawPixels(misc.img_w, misc.img_h, GL_BGR, GL_UNSIGNED_BYTE, ptr);
    glutSwapBuffers();
    return;
}

static void draw_win_best()
{
    glDrawPixels(misc.img_w, misc.img_h, GL_BGR, GL_UNSIGNED_BYTE, gene_best.pixel);
    glutSwapBuffers();
    return;
}

static void draw_win_curr()
{
    glutSetWindow(misc.win_curr);

    struct timespec tm0;
    struct timespec tm1;
    clock_gettime(CLOCK_MONOTONIC, &tm0);
    // inline mutation
    u32_t rmode = mrand48();
    static u32_t rindex = 0;//mrand48() % (config.polygon_size * 3);
    rindex = (rindex + 1) % (config.polygon_size * 3);
    u64_t backup;
    if (rmode & (1 << 31)) // mutate color
    {
        u32_t cchan = rmode & 3; // color channel
        u8_t *p = gene_curr.color[rindex / 3 * 3];
        u8_t rvalue = mrand48() & 255;
        backup = p[cchan];
        p[cchan] = rvalue;
        p[cchan + 4] = rvalue;
        p[cchan + 8] = rvalue;
    }
    else // mutate vertex
    {
        backup = *(u64_t*)gene_curr.vertex[rindex];
        gene_curr.vertex[rindex][0] = (f32_t)((drand48() - 0.5) * 2);
        gene_curr.vertex[rindex][1] = (f32_t)((drand48() - 0.5) * 2);    
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST); 
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glVertexPointer(2, GL_FLOAT, 0, gene_curr.vertex);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, gene_curr.color);
    glDrawArrays(GL_TRIANGLES, 0, config.polygon_size * 3);

    glutSwapBuffers();
    glutPostRedisplay();
    // inline fitness calculation
    glReadPixels(0, 0, misc.img_w, misc.img_h, GL_BGR, GL_UNSIGNED_BYTE, gene_curr.pixel);
    u8_t *orig_img = FreeImage_GetBits(misc.img);
    u64_t tmp_fitness = 0;
    int i;
    for (i = 0; i < misc.img_w * misc.img_h * 3; i++)
    {
        u32_t diff = orig_img[i] - gene_curr.pixel[i];
        tmp_fitness += diff * diff;
    }
    //printf("%lld %lld\n", tmp_fitness, gene_curr.fitness);
    gene_curr.mutation_cnt += 1;
    if (tmp_fitness < gene_curr.fitness) // better solution
    {
        gene_curr.evolution_cnt += 1;
        gene_curr.fitness = tmp_fitness;
        if (tmp_fitness < gene_best.fitness) // best solution so far
        {
            int vcount = config.polygon_size * 3;
            int pcount = misc.img_w * misc.img_h;
            memcpy(gene_best.vertex, gene_curr.vertex, vcount * 2 * sizeof(f32_t));
            memcpy(gene_best.color, gene_curr.color, vcount * 4 * sizeof(u8_t));
            memcpy(gene_best.pixel, gene_curr.pixel, pcount * 3 * sizeof(u8_t));
            gene_best.fitness = tmp_fitness;
            glutSetWindow(misc.win_best);
            glutPostRedisplay();
        }
    }
    else // restore value
    {
        if (rmode & (1 << 31)) // mutate color
        {
            u32_t cchan = rmode & 3; // color channel
            u8_t *p = gene_curr.color[rindex / 3 * 3];
            p[cchan] = backup;
            p[cchan + 4] = backup;
            p[cchan + 8] = backup;
        }
        else // mutate vertex
        {
            *(u64_t*)gene_curr.vertex[rindex] = backup;
        }
    }

    static int log_status;
    if (!rindex && !log_status--) // time to log
    {
        clock_gettime(CLOCK_MONOTONIC, &tm1);
        log_status = config.log_interval;
        log_printf("Mutation count:  %d\n", gene_curr.mutation_cnt);
        log_printf("Evolution count: %d\n", gene_curr.evolution_cnt);
        f32_t tdiff = (tm1.tv_sec - tm0.tv_sec) * 1000.0 + (tm1.tv_nsec - tm0.tv_nsec) / 1000000.0;
        log_printf("Current fps: %.2f\n", 1000 / tdiff);
        log_printf("Current fitness: %lld\n", gene_curr.fitness);
        log_printf("\n");
        fflush(misc.log_file);

        char *log_img_filename;
        asprintf(&log_img_filename, "%s/%d_%s", misc.log_path, gene_curr.mutation_cnt, misc.img_filename);
        FIBITMAP *log_img = FreeImage_ConvertFromRawBits(
            gene_curr.pixel, 
            misc.img_w, misc.img_h,
            misc.img_w * 3 * sizeof(u8_t),
            24,
            FI_RGBA_RED_MASK,
            FI_RGBA_GREEN_MASK,
            FI_RGBA_BLUE_MASK,
            FALSE
        );

        FreeImage_Save(misc.fmt, log_img, log_img_filename, 0);

        free(log_img_filename);
    }
    return;
}

static void reshape(GLsizei w, GLsizei h)
{
    glutReshapeWindow(misc.img_w, misc.img_h);
    return;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: ./a.out filename\n");
        goto EXIT;
    }
    else
        misc.img_filename = argv[1];
    srand48(time(NULL));

    // init image
    misc.fmt = FreeImage_GetFileType(misc.img_filename, 0);
    if (misc.fmt == FIF_UNKNOWN)
    {
        fprintf(stderr, "unknown file type: %s\n", misc.img_filename);
        goto EXIT;
    }
    FIBITMAP *img = FreeImage_Load(misc.fmt, misc.img_filename, 0);
    if (!img)
    {
        fprintf(stderr, "failed to load image: %s\n", misc.img_filename);
    }
    misc.img = FreeImage_ConvertTo24Bits(img);
    FreeImage_Unload(img);

    misc.img_w = FreeImage_GetWidth(misc.img);
    misc.img_h = FreeImage_GetHeight(misc.img);

    // init log
    time_t t = time(NULL);
    asprintf(&misc.log_path, "%s%x_%s", config.log_path_pref, time(NULL), misc.img_filename);
    char *mkdir;
    asprintf(&mkdir, "mkdir %s", misc.log_path);
    system(mkdir);
    char *cp;
    asprintf(&cp, "cp %s %s/original_%s", misc.img_filename, misc.log_path, misc.img_filename);
    system(cp);
    char *log_filename;
    asprintf(&log_filename, "%s/%s.log", misc.log_path, misc.img_filename);
    misc.log_file = fopen(log_filename, "w");

    // init gene
    gene_best.vertex = malloc(config.polygon_size * 3 * 2 * sizeof(f32_t));
    gene_best.color = malloc(config.polygon_size * 3* 4 * sizeof(u8_t));
    gene_best.pixel = malloc(misc.img_w * misc.img_h * 3 * sizeof(u8_t));
    gene_best.fitness = 1llu << 60;

    gene_curr.vertex = malloc(config.polygon_size * 3 * 2 * sizeof(f32_t));
    gene_curr.color = calloc(config.polygon_size * 3 * 4, sizeof(u8_t));
    gene_curr.pixel = malloc(misc.img_w * misc.img_h * 3 * sizeof(u8_t));
    gene_curr.fitness = 1llu << 60;

    int i;
    f32_t *p = (f32_t*)gene_curr.vertex;
    for (i = 0; i < config.polygon_size * 3 * 2; i++)
        p[i] = (drand48() - 0.5) * 2;
    for (i = 0; i < config.polygon_size * 3; i++)
    {
        u8_t *p = gene_curr.color[i];
        p[0] = p[1] = p[2] = 0;
        p[4] = 64;
    }

    // init OpenGL context
    putenv("__GL_SYNC_TO_VBLANK=0");
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_ALPHA);
    glutInitWindowSize(misc.img_w, misc.img_h);

    int window_split = 16;

    misc.win_orig = glutCreateWindow("Original");
    glutPositionWindow(0, 0);
    glutDisplayFunc(draw_win_orig);
    glutReshapeFunc(reshape);

    misc.win_best = glutCreateWindow("Best");
    glutPositionWindow(misc.img_w + window_split, 0);
    glutDisplayFunc(draw_win_best);
    glutReshapeFunc(reshape);

    misc.win_curr = glutCreateWindow("Current");
    glutPositionWindow((misc.img_w + window_split) * 2, 0);
    glutDisplayFunc(draw_win_curr);
    glutReshapeFunc(reshape);

    glutMainLoop();

EXIT:
    return 0;
}

