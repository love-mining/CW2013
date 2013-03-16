/*
 * This is 2013AI_cw2.c by Pengyu CHEN(cpy.prefers.you@gmail.com)
 * As the coursework of Artificial Intelligence, 2013.02-2013.04
 * COPYLEFT, ALL WRONGS RESERVED.
 */

#include <GL/freeglut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <FreeImage.h>
#include <stdio.h>
#include <stdlib.h>

typedef unsigned long long u64_t;
typedef unsigned long u32_t;
typedef unsigned short u16_t;
typedef unsigned char u8_t;
typedef int i32_t;
typedef short i16_t;
typedef float f32_t;

typedef struct _confit_t
{
    // int population_size; // hardcoded as 1
    // int polygon_n; // hardcoded as 3
    int polygon_size; 
}   config_t;

static config_t config = {
    .polygon_size = 256,
};

typedef struct _gene_t
{
    f32_t (*vertex)[3][2];
    u8_t (*color)[3][4];
    u8_t *pixels;
    u64_t fitness;
}   gene_t;

static gene_t gene_best;
static gene_t gene_curr;

typedef struct misc_t
{
    FREE_IMAGE_FORMAT fmt;
    FIBITMAP *img;
    u32_t img_w;
    u32_t img_h;
    int win_orig;
    int win_best;
    int win_curr;
}   misc_t;

static misc_t misc;

static void draw_win_orig()
{
    void *ptr = FreeImage_GetBits(misc.img);
//    glutSetWindow(misc.win_orig);
    glDrawPixels(misc.img_w, misc.img_h, GL_RGB, GL_UNSIGNED_BYTE, ptr);
    glutSwapBuffers();
    return;
}

static void draw_win_best()
{

}

static void draw_win_curr()
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST); 
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClear(GL_COLOR_BUFFER_BIT);
    glVertexPointer(2, GL_FLOAT, 0, gene_curr.vertex);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, gene_curr.color);
    glDrawArrays(GL_TRIANGLES, 0, config.polygon_size * 3);

    glutSwapBuffers();
    return;
}

static void reshape(GLsizei w, GLsizei h)
{
    glutReshapeWindow(misc.img_w, misc.img_h);
    return;
}

int main(int argc, char **argv)
{
    char *filename;
    if (argc != 2)
    {
        fprintf(stderr, "usage: ./a.out filename\n");
        goto EXIT;
    }
    else
        filename = argv[1];

    // init image
    misc.fmt = FreeImage_GetFileType(filename, 0);
    if (misc.fmt == FIF_UNKNOWN)
    {
        fprintf(stderr, "unknown file type: %s\n", filename);
        goto EXIT;
    }
    FIBITMAP *img = FreeImage_Load(misc.fmt, filename, 0);
    if (!img)
    {
        fprintf(stderr, "failed to load image: %s\n", filename);
    }
    misc.img = FreeImage_ConvertTo24Bits(img);
    FreeImage_Unload(img);

    misc.img_w = FreeImage_GetWidth(misc.img);
    misc.img_h = FreeImage_GetHeight(misc.img);

    // init gene
    gene_best.vertex = malloc(config.polygon_size * 3 * 2 * sizeof(f32_t));
    gene_best.color = malloc(config.polygon_size * 3* 4 * sizeof(u8_t));
    gene_best.pixels = malloc(misc.img_w * misc.img_h * 3 * sizeof(u8_t));
    gene_best.fitness = 1llu << 63;

    gene_curr.vertex = malloc(config.polygon_size * 3 * 2 * sizeof(f32_t));
    gene_curr.color = calloc(config.polygon_size * 3 * 4, sizeof(u8_t));
    gene_curr.pixels = malloc(misc.img_w * misc.img_h * 3 * sizeof(u8_t));
    gene_curr.fitness = 1llu << 63;

    int i;
    f32_t *p = (f32_t*)gene_curr.vertex;
    for (i = 0; i < config.polygon_size * 3 * 2; i++)
        p[i] = (drand48() - 0.5) * 2;
    #if 0
    for (i = 0; i < config.polygon_size; i++)
    {
        int *p = (int*)gene_curr.color[i];
        p[0] = mrand48() & ~(255 << 24);
        p[1] = p[0];
        p[2] = p[0];
    }
    #endif

    // init OpenGL context
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

