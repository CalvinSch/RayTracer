//Calvin Schaul, CS657 Spring 2021

#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define nelem(a) (sizeof(a) / (sizeof(a[0])))
#define min(x,y) ((x) < (y) ? (x) : (y))
#define max(x,y) ((x) < (y) ? (y) : (x))

#define PI 3.1415926536
#define WIDTH 500
#define HEIGHT 500

typedef unsigned char rgb[3];

struct vec3 {
    float x, y, z;
};

struct Material {
    vec3 color;
    float ka, kd, ks, ns, kr;
};

//output image
rgb *fb;

vec3 viewpoint = {0.0, 1.0, 0.0};

//viewplane;
vec3 vp_u = {1.0, 0.0, 0.0};
vec3 vp_v = {0.0, 1.0, 0.0};
vec3 vp_w = {0.0, 0.0, -1.0};

//focal length
float vp_d = 1.0;

//set of spheres
vec3 mysph_pos[] = {
    { 2.0, 1.0, -7.0},
    {-1.0, 1.0, -6.0},
};

//material properties
Material myobj_mat[] = {
    {{0.6, 0.8, 0.6}, 0.2, 1.0, 0.0, 28, 0.0 },
    {{0.6, 0.6, 0.8}, 0.4, 0.8, 0.7, 28, 0.0 },
    {{1.0, 1.0, 1.0}, 0.7, 0.7, 0.7, 28, 0.5 },  //floor
};

//background color
vec3 bgcolor = { 0.0, 0.0, 0.0 };
vec3 ambient = { 0.2, 0.2, 0.2 };

//set of lights
vec3 light_pos[] = {
    {  5.0, 6.0, -6.0 },
    { -5.0, 3.0, -4.5 },
};

vec3 light_color[] = {
    { 0.5, 0.5, 0.5 },
    { 0.7, 0.0, 0.0 },
};



float
clamp(float v, float l, float m)
{
    if(v > m) return m;
    if(v < l) return l;
    return v;
}

vec3
clamp(const vec3 &v)
{
    vec3 r = { clamp(v.x, 0.0, 1.0), clamp(v.y, 0.0, 1.0), clamp(v.z, 0.0, 1.0) };
    return r;
}

vec3
operator+(const vec3 &a, const vec3 &b)
{
    vec3 r = { a.x + b.x, a.y + b.y, a.z + b.z };
    return r;
}

vec3
operator-(const vec3 &a, const vec3 &b)
{
    vec3 r = { a.x - b.x, a.y - b.y, a.z - b.z };
    return r;
}

vec3
operator*(const vec3 &a, float s)
{
    vec3 r = { a.x * s, a.y * s, a.z * s };
    return r;
}

float
dot(const vec3 &a, const vec3 &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float
length(const vec3 &v)
{
    return sqrt(dot(v, v));
}

vec3
normalize(const vec3 &v)
{
    float n = length(v);
    if(n == 0.0)
        n = FLT_MIN;
    return v * (1.0/n);
}

vec3
reflect(const vec3 &ldir, const vec3 &normal)
{
    return (normal * (2.0 * dot(ldir, normal))) - ldir;
}

float
isphere(const vec3 &o, const vec3 &d, const vec3 &c)
{
    float tm, l2, dt, t1, t2;
    vec3 p, pm, ddt;

    p = c - o;
    tm = dot(p, d);
    l2 = dot(p, p) - tm * tm;
    if(tm < 0 || (1.0 - l2) < 0)
        return -1.0;

    dt = sqrt(1.0 - l2);

    pm = d * tm;
    ddt = d * dt;

    t1 = length(pm - ddt);
    t2 = length(pm + ddt);
    return min(t1, t2);
}

float
ifloor(const vec3 &origin, const vec3 &dir)
{
    float t;

    if(dir.y == 0.0)
        return -1.0;
    t = -origin.y / dir.y;
    if(t < 0)
        return -1.0;
    return length(dir * t);
}

vec3
traceray(const vec3 &origin, const vec3 &dir, int bounce)
{
    vec3 ip, ldir, rdir, normal, diffuse, specular, eyedir, fcolor, rcolor;
    const Material *mat;
    int i, j, imat, inshadow;
    float ldist, h, t = FLT_MAX;

    for(i = 0; i < nelem(mysph_pos); i++) {
        h = isphere(origin, dir, mysph_pos[i]);
        if(h >= 0 && h < t) {
            t = h;
            imat = i;
        }
    }

    h = -1;
    if(origin.y != 0.0)
        h = ifloor(origin, dir);
    if(h > 0 && h < t) {
        t = h;
        imat = nelem(mysph_pos);
        ip = origin + dir * t;
        ip.y = 0.0;
    }
    else if(t != FLT_MAX)
        ip = origin + dir * t;
    else
        return bgcolor;

    fcolor.x = 0.0;
    fcolor.y = 0.0;
    fcolor.z = 0.0;
    mat = &myobj_mat[imat];
    eyedir = normalize(origin - ip);

    for(i = 0; i < nelem(light_pos); i++) {
        ldir = light_pos[i] - ip;
        ldist = length(ldir);
        ldir = ldir * (1.0/ldist);


        inshadow = 0;
        for(j = 0; j < nelem(mysph_pos); j++) {
            h = isphere(ip, ldir, mysph_pos[j]);
            if(h >= 0 && h < ldist) {
                inshadow = 1;
                break;
            }
        }
        if(inshadow)
            continue;

        if(imat < nelem(mysph_pos))
            normal = normalize(ip - mysph_pos[imat]);
        else {
            normal.x = 0.0;
            normal.y = 1.0;
            normal.z = 0.0;
        }

        rdir = reflect(ldir, normal);

        diffuse = light_color[i] * max(dot(ldir, normal), 0.0);
        specular = light_color[i] * pow(max((double)dot(eyedir, rdir), 0.0), (double)(mat->ns));

        fcolor = fcolor + (diffuse * mat->kd) + (specular * mat->ks);
    }
    if(bounce > 0) {
        rdir = reflect(eyedir, normal);
        rcolor = traceray(ip, rdir, bounce-1);
        fcolor = fcolor + (rcolor * mat->kr);
    }
    fcolor = fcolor + (ambient * mat->ka);
    fcolor.x = fcolor.x * mat->color.x;
    fcolor.y = fcolor.y * mat->color.y;
    fcolor.z = fcolor.z * mat->color.z;
    return clamp(fcolor);
}

void
render(void)
{
    int x,y;
    vec3 fcolor, origin, dir;

    for(y = 0; y < HEIGHT; y++) {
        for(x = 0; x < WIDTH; x++) {

            origin = viewpoint + (vp_u * ((1.0/WIDTH)*(x+.5)-.5))
                   + (vp_v * ((1.0/HEIGHT)*(y+.5)-.5))
                   + (vp_w * vp_d);
            dir = normalize(origin - viewpoint);

            fcolor = traceray(origin, dir, 2);
            fb[(HEIGHT - y) * WIDTH + x][0] = fcolor.x * 255;
            fb[(HEIGHT - y) * WIDTH + x][1] = fcolor.y * 255;
            fb[(HEIGHT - y) * WIDTH + x][2] = fcolor.z * 255;
        }
    }
}

void
WritePPM(char *fn, rgb *dataset)
{
    int i,j;
    FILE *fp;
    if((fp = fopen(fn, "wb")) == NULL) {
        perror(fn);
        return;
    }
    printf("Begin writing to file %s....", fn);
    fflush(stdout);
    fprintf(fp, "P6\n%d %d\n%d\n", WIDTH, HEIGHT, 255);
    for (j=0; j<HEIGHT; j++)
        for (i=0; i<WIDTH; i++) {
            fputc(dataset[j*WIDTH+i][0], fp);
            fputc(dataset[j*WIDTH+i][1], fp);
            fputc(dataset[j*WIDTH+i][2], fp);
        }
    printf("done\n");
    fclose(fp);
}

int
main(int argc, char *argv[])
{
    if(argc != 2) {
        fprintf(stderr, "Usage: %s <output_img>\n"
                        "       <output_img>: PPM file\n", argv[0]);
        exit(-1);
    }

    fb = (rgb*)malloc(WIDTH * HEIGHT * sizeof(rgb));
    if(fb == NULL) {
        perror("malloc");
        exit(-1);
    }
	printf("hello\n");
    render();
	printf("hello222\n");
    WritePPM(argv[1], fb);
    free(fb);
    return 0;
}
