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

typedef struct _confit_t
{
    
}   config_t;

static config_t config;

typedef struct misc_t
{
    FREE_IMAGE_FORMAT fmt;
    FIBITMAP *img;
    unsigned int img_w;
    unsigned int img_h;
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

