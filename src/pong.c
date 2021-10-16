/**
 * Some random pong code jon wrote in early may 2021
 * pong.h declaration was added today (10/16)
 */
#include <stdio.h>
#include <stdlib.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "pong.h"

struct vec3 {
    float x;
    float y;
    float z;
};

struct quad {
    struct vec3 p1;
    struct vec3 p2;
    struct vec3 p3;
    struct vec3 p4;
};

struct twotris {
    int p1;
    int p2;
    int p3;
    int p4;
    int p5;
    int p6;
};

int size; // total size of the buffer
int amount; // amount of the buffer to render

struct vec3 ballVelocity = (struct vec3) {4,4,0};
int respawnTime = 60;
int suspend = 0;
int p1points = 0;
int p2points = 0;

// 7-segment displays
int *seg1;
int *seg2;
int *seg3;

int *textEndPtr;

struct quad *points;
struct twotris *indices;

// pointers to start rendering text, using quads! yay...
// really messy
struct vec3 *rawdataP;
int *rawdataI;

int maxSize;

int program;
unsigned int vao;
unsigned int vbo;
unsigned int ibo;

void init() {
    glfwInit();
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

char* readFile(char* file) {
    FILE *ptr;
    long size;
    char *buffer;

    ptr = fopen(file, "rb");

    if(ptr == NULL) {
        printf("ERROR COULD NOT PRINT FROM ");
        printf(file);
        printf("\n");
        return "";
    }

    fseek(ptr, 0L, SEEK_END);
    size = ftell(ptr);
    rewind(ptr);

    buffer = calloc(1, size+1);
    if(!buffer) {
        fclose(ptr);
        printf("failed to create buffer.");
        return "";
    }

    if(1!=fread(buffer, size, 1, ptr)) {
        fclose(ptr);
        free(buffer);
        printf("failed to read");
        return "";
    }

    fclose(ptr);
    return buffer;
}

void initShaders(unsigned int *program) {
    unsigned int vertexShader;
    unsigned int fragmentShader;
    int success;
    char infoLog[512];

    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* vData = readFile("shaders/vertexShader.vs");
    glShaderSource(vertexShader, 1, &vData, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        printf(infoLog);
    }

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fData = readFile("shaders/fragmentShader.fs");
    glShaderSource(fragmentShader, 1, &fData, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        printf(infoLog);
    }

    *program = glCreateProgram();
    glAttachShader(*program, vertexShader);
    glAttachShader(*program, fragmentShader);
    glLinkProgram(*program);
    glGetShaderiv(*program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(*program, 512, NULL, infoLog);
        printf(infoLog);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    free(vData);
    free(fData);
}

void processInput(GLFWwindow* window) {
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        // lol idk
    }
}

int touching(struct vec3 p1, struct vec3 p2, struct vec3 r1, struct vec3 r2) {
    if(p1.x > r2.x || p2.x < r1.x) {
        return 0;
    }
    if(p1.y > r2.y || p2.y < r1.y) {
        return 0;
    }
    return 1;
}

// is touching paddle? 0: no, 1: left, 2: right
int touchingPaddle() {
    if(touching(points[0].p1, points[0].p3, points[2].p1, points[2].p3)) {
        return 1;
    } else {
        if(touching(points[1].p1, points[1].p3, points[2].p1, points[2].p3)) {
            return 2;
        } else {
            return 0;
        }
    }
}

// inits 24 points for a 7 seg display
void create7Seg(struct quad *pointLoc, struct twotris *indexLoc, struct vec3 topLeft, int offset) {
    int size = 5;
    int largeOffset = size * 8;
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 2; j++) {
            int offsett = i * 2 + j;
            (pointLoc + offsett)->p1 = (struct vec3) {topLeft.x + (largeOffset * j) - size, topLeft.y + (largeOffset * i) - size, topLeft.z};
            (pointLoc + offsett)->p2 = (struct vec3) {topLeft.x + (largeOffset * j) + size, topLeft.y + (largeOffset * i) - size, topLeft.z};
            (pointLoc + offsett)->p3 = (struct vec3) {topLeft.x + (largeOffset * j) + size, topLeft.y + (largeOffset * i) + size, topLeft.z};
            (pointLoc + offsett)->p4 = (struct vec3) {topLeft.x + (largeOffset * j) - size, topLeft.y + (largeOffset * i) + size, topLeft.z};
        }
    }
    // "-" lines
    *(indexLoc+0) = (struct twotris) {offset + 0, offset + 5, offset + 3, offset + 5, offset + 6, offset + 3};
    *(indexLoc+1) = (struct twotris) {offset + 8, offset + 13, offset + 11, offset + 13, offset + 14, offset + 11};
    *(indexLoc+2) = (struct twotris) {offset + 16, offset + 21, offset + 19, offset + 21, offset + 22, offset + 19};
    // "|" left lines
    *(indexLoc+3) = (struct twotris) {offset + 0, offset + 11, offset + 10, offset + 10, offset + 1, offset + 0};
    *(indexLoc+4) = (struct twotris) {offset + 8, offset + 19, offset + 18, offset + 18, offset + 9, offset + 8};
    // "|" right lines
    *(indexLoc+5) = (struct twotris) {offset + 4, offset + 5, offset + 14, offset + 14, offset + 15, offset + 4};
    *(indexLoc+6) = (struct twotris) {offset + 12, offset + 13, offset + 22, offset + 22, offset + 23, offset + 12};
}

void enableSeg(struct twotris *seg, int offset) {
    if((seg+offset)->p1 < 0) {
        (seg+offset)->p1 *= -1;
        (seg+offset)->p2 *= -1;
        (seg+offset)->p3 *= -1;
        (seg+offset)->p4 *= -1;
        (seg+offset)->p5 *= -1;
        (seg+offset)->p6 *= -1;
    } else {
    }
}

void disableSeg(struct twotris *seg, int offset) {
    if((seg+offset)->p1 > 0) {
        (seg+offset)->p1 *= -1;
        (seg+offset)->p2 *= -1;
        (seg+offset)->p3 *= -1;
        (seg+offset)->p4 *= -1;
        (seg+offset)->p5 *= -1;
        (seg+offset)->p6 *= -1;
    } else {
    }
}

// pushes number to 7 seg, use create7Seg first
void pushToSeg(struct twotris *indexLoc, char number) {
    switch(number) {
        case 0:
            enableSeg(indexLoc, 0);
            disableSeg(indexLoc, 1);
            enableSeg(indexLoc, 2);
            enableSeg(indexLoc, 3);
            enableSeg(indexLoc, 4);
            enableSeg(indexLoc, 5);
            enableSeg(indexLoc, 6);
            break;
        case 1:
            disableSeg(indexLoc, 0);
            disableSeg(indexLoc, 1);
            disableSeg(indexLoc, 2);
            disableSeg(indexLoc, 3);
            disableSeg(indexLoc, 4);
            enableSeg(indexLoc, 5);
            enableSeg(indexLoc, 6);
            break;
        case 2:
            enableSeg(indexLoc, 0);
            enableSeg(indexLoc, 1);
            enableSeg(indexLoc, 2);
            disableSeg(indexLoc, 3);
            enableSeg(indexLoc, 4);
            enableSeg(indexLoc, 5);
            disableSeg(indexLoc, 6);
            break;
        case 3:
            enableSeg(indexLoc, 0);
            enableSeg(indexLoc, 1);
            enableSeg(indexLoc, 2);
            disableSeg(indexLoc, 3);
            disableSeg(indexLoc, 4);
            enableSeg(indexLoc, 5);
            enableSeg(indexLoc, 6);
            break;
        case 4:
            disableSeg(indexLoc, 0);
            enableSeg(indexLoc, 1);
            disableSeg(indexLoc, 2);
            enableSeg(indexLoc, 3);
            disableSeg(indexLoc, 4);
            enableSeg(indexLoc, 5);
            enableSeg(indexLoc, 6);
            break;
        case 5:
            enableSeg(indexLoc, 0);
            enableSeg(indexLoc, 1);
            enableSeg(indexLoc, 2);
            enableSeg(indexLoc, 3);
            disableSeg(indexLoc, 4);
            disableSeg(indexLoc, 5);
            enableSeg(indexLoc, 6);
            break;
        case 6:
            enableSeg(indexLoc, 0);
            enableSeg(indexLoc, 1);
            enableSeg(indexLoc, 2);
            enableSeg(indexLoc, 3);
            enableSeg(indexLoc, 4);
            disableSeg(indexLoc, 5);
            enableSeg(indexLoc, 6);
            break;
        case 7:
            enableSeg(indexLoc, 0);
            disableSeg(indexLoc, 1);
            disableSeg(indexLoc, 2);
            disableSeg(indexLoc, 3);
            disableSeg(indexLoc, 4);
            enableSeg(indexLoc, 5);
            enableSeg(indexLoc, 6);
            break;
        case 8:
            enableSeg(indexLoc, 0);
            enableSeg(indexLoc, 1);
            enableSeg(indexLoc, 2);
            enableSeg(indexLoc, 3);
            enableSeg(indexLoc, 4);
            enableSeg(indexLoc, 5);
            enableSeg(indexLoc, 6);
            break;
        case 9:
            enableSeg(indexLoc, 0);
            enableSeg(indexLoc, 1);
            disableSeg(indexLoc, 2);
            enableSeg(indexLoc, 3);
            disableSeg(indexLoc, 4);
            enableSeg(indexLoc, 5);
            enableSeg(indexLoc, 6);
            break;
        case 'p':
            enableSeg(indexLoc, 0);
            enableSeg(indexLoc, 1);
            disableSeg(indexLoc, 2);
            enableSeg(indexLoc, 3);
            enableSeg(indexLoc, 4);
            enableSeg(indexLoc, 5);
            disableSeg(indexLoc, 6);
            break;
        case 'w':
            disableSeg(indexLoc, 0);
            disableSeg(indexLoc, 1);
            enableSeg(indexLoc, 2);
            enableSeg(indexLoc, 3);
            disableSeg(indexLoc, 4);
            enableSeg(indexLoc, 5);
            disableSeg(indexLoc, 6);
            break;
        case 'i':
            disableSeg(indexLoc, 0);
            disableSeg(indexLoc, 1);
            disableSeg(indexLoc, 2);
            disableSeg(indexLoc, 3);
            disableSeg(indexLoc, 4);
            enableSeg(indexLoc, 5);
            enableSeg(indexLoc, 6);
            break;
        case 'n':
            enableSeg(indexLoc, 0);
            disableSeg(indexLoc, 1);
            disableSeg(indexLoc, 2);
            enableSeg(indexLoc, 3);
            enableSeg(indexLoc, 4);
            enableSeg(indexLoc, 5);
            enableSeg(indexLoc, 6);
            break;
    }
}

// side 1 is left, side 2 is right
void win(int side) {
    pushToSeg(seg3, side);
    amount = (textEndPtr - (int*) indices) / 6;
    suspend = 1;
}

// side 1 is left, side 2 is right
void score(int side) {
    switch(side) {
        case 1:
            p2points++;
            pushToSeg(seg1, p2points%10);
            if(p2points == 10) {
                win(1);
            }
            points[2] = (struct quad) {(struct vec3) {590,300,0}, (struct vec3) {590,306,0}, (struct vec3) {584,306,0}, (struct vec3) {584,300,0}};
            ballVelocity = (struct vec3) {-4,4,0};
            break;
        case 2:
            p1points++;
            pushToSeg(seg2, p1points%10);
            if(p1points == 10) {
                win(2);
            }
            points[2] = (struct quad) {(struct vec3) {40,300,0}, (struct vec3) {40,306,0}, (struct vec3) {46,306,0}, (struct vec3) {46,300,0}};
            ballVelocity = (struct vec3) {4,4,0};
            break;
    }
    respawnTime = 60;
}

void render(GLFWwindow* window) {
    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(struct quad) * size, (float*) points, GL_STREAM_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(struct twotris) * size, (int*) indices, GL_STREAM_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glUseProgram(program);

    glDrawElements(GL_TRIANGLES, 6 * amount, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glfwSwapBuffers(window);
}

void update(GLFWwindow* window) {
    glfwPollEvents();
    processInput(window);
    if(suspend) return;

    int moveDist = 6;

    // left paddle
    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        if(points[0].p4.y + moveDist <= 480) {
            points[0].p1.y += moveDist;
            points[0].p2.y += moveDist;
            points[0].p3.y += moveDist;
            points[0].p4.y += moveDist;
        }
    }
    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        if(points[0].p1.y - moveDist >= 0) {
            points[0].p1.y -= moveDist;
            points[0].p2.y -= moveDist;
            points[0].p3.y -= moveDist;
            points[0].p4.y -= moveDist;
        }
    }

    // right paddle
    if(glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
        if(points[1].p4.y + moveDist <= 480) {
            points[1].p1.y += moveDist;
            points[1].p2.y += moveDist;
            points[1].p3.y += moveDist;
            points[1].p4.y += moveDist;
        }
    }
    if(glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
        if(points[1].p1.y - moveDist >= 0) {
            points[1].p1.y -= moveDist;
            points[1].p2.y -= moveDist;
            points[1].p3.y -= moveDist;
            points[1].p4.y -= moveDist;
        }
    }

    // ball
    if(respawnTime) { // can't do this in java :)
        respawnTime--;
    } else {
        if(ballVelocity.y > 0) {
            if(points[2].p4.y + ballVelocity.y <= 480) {
                points[2].p1.y += ballVelocity.y;
                points[2].p2.y += ballVelocity.y;
                points[2].p3.y += ballVelocity.y;
                points[2].p4.y += ballVelocity.y;
            } else {
                ballVelocity.y *= -1;
            }
        } else if(ballVelocity.y < 0) {
            if(points[2].p1.y + ballVelocity.y >= 0) {
                points[2].p1.y += ballVelocity.y;
                points[2].p2.y += ballVelocity.y;
                points[2].p3.y += ballVelocity.y;
                points[2].p4.y += ballVelocity.y;
            } else {
                ballVelocity.y *= -1;
            }
        }
        if(ballVelocity.x > 0) {
            if(points[2].p2.x + ballVelocity.x <= 640) {
                points[2].p1.x += ballVelocity.x;
                points[2].p2.x += ballVelocity.x;
                points[2].p3.x += ballVelocity.x;
                points[2].p4.x += ballVelocity.x;
            } else {
                score(1);
            }
        } else if(ballVelocity.x < 0) {
            if(points[2].p1.x + ballVelocity.x >= 0) {
                points[2].p1.x += ballVelocity.x;
                points[2].p2.x += ballVelocity.x;
                points[2].p3.x += ballVelocity.x;
                points[2].p4.x += ballVelocity.x;
            } else {
                score(2);
            }
        }
        int touch = touchingPaddle();
        if(touch) ballVelocity.x *= -1;
        // CALCULATE WHICH ANGLE BASED ON Y will do
    }
}

int run() {
    init(); // window hints and init

    // WINDOWING
    GLFWwindow* window = glfwCreateWindow(640, 480, "Hello, World!", NULL, NULL);
    if(window == NULL) {
        printf("uhoh the window failed\n");
        glfwTerminate();
        exit(-1);
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if(!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        printf("uhoh glad failed to init\n");
        glfwTerminate();
        exit(-1);
    }

    initShaders(&program);

    // OPENGL INIT
    glClearColor(0,0,0,0);

    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ibo);
    glGenVertexArrays(1, &vao);

    // DATA INIT
    size = 100;
    amount = 100;

    points = calloc(180, sizeof(struct vec3)); // don't wanna read garbage lol
    points[0] = (struct quad) {(struct vec3) {30,120,0}, (struct vec3) {40,120,0}, (struct vec3) {40,160,0}, (struct vec3) {30,160,0}};
    points[1] = (struct quad) {(struct vec3) {590,120,0}, (struct vec3) {600,120,0}, (struct vec3) {600,160,0}, (struct vec3) {590,160,0}};
    points[2] = (struct quad) {(struct vec3) {40,300,0}, (struct vec3) {40,306,0}, (struct vec3) {46,306,0}, (struct vec3) {46,300,0}};
    rawdataP = &(points[3]);

    indices = calloc(312, sizeof(int)); // still, reading garbage is bad
    indices[0] = (struct twotris) {0, 1, 3, 1, 2, 3};
    indices[1] = (struct twotris) {4, 5, 7, 5, 6, 7};
    indices[2] = (struct twotris) {8, 9, 11, 9, 10, 11};
    rawdataI = &(indices[3]);

    // text
    create7Seg(rawdataP, rawdataI, (struct vec3) {100, 50, 0}, 12);
    pushToSeg(rawdataI, 0);
    seg1 = rawdataI;

    struct vec3 *ptr = rawdataP + 24;
    seg2 = seg1 + 42; // 6 values * 7 segments
    create7Seg(ptr, seg2, (struct vec3) {490, 50, 0}, 12+24);
    pushToSeg(seg2, 0);

    // hidden until gameover
    struct vec3 *ptr2 = ptr + 24;
    seg3 = seg2 + 42;
    create7Seg(ptr2, seg3, (struct vec3) {320, 240, 0}, 12+24+24);
    pushToSeg(seg3, 6);
    // hide seg3
    amount = (seg3 - ((int*) indices)) / 6;

    // gameover text
    struct vec3 *textP = (struct quad*) (ptr2 + 24);
    int *textI = (struct twotris*) (seg3 + 42);
    create7Seg(textP, textI, (struct vec3) {240, 240, 0}, 12+24+24+24);
    pushToSeg(textI, 'p');

    create7Seg(textP+24, textI+42, (struct vec3) {200, 360, 0}, 12+24+24+24+24);
    pushToSeg(textI+42, 'w');

    create7Seg(textP+24+24, textI+42+42, (struct vec3) {240, 360, 0}, 12+24+24+24+24+24);
    pushToSeg(textI+42+42, 'i');

    create7Seg(textP+24+24+24, textI+42+42+42, (struct vec3) {320, 360, 0}, 12+24+24+24+24+24+24);
    pushToSeg(textI+42+42+42, 'n');
    textEndPtr = textI+42+42+42+42;
    // LOOP
    // i literally copied this from stackoverflow lol
    double limitFPS = 1.0 / 60.0;

    double lastTime = glfwGetTime(), timer = lastTime;
    double deltaTime = 0, nowTime = 0;
    int frames = 0 , updates = 0;

    while(!glfwWindowShouldClose(window)) {
        // - Measure time
        nowTime = glfwGetTime();
        deltaTime += (nowTime - lastTime) / limitFPS;
        lastTime = nowTime;

        // - Only update at 60 frames / s
        while (deltaTime >= 1.0){
            update(window);   // - Update function
            updates++;
            deltaTime--;
        }
        // - Render at maximum possible frames
        render(window); // - Render function
        frames++;

        // - Reset after one second
        if (glfwGetTime() - timer > 1.0) {
            timer++;
            char text[20]; // this shouldn't fail...
            sprintf(text, "FPS: %d UPS: %d", frames, updates);
            glfwSetWindowTitle(window, text);
            updates = 0, frames = 0;
        }
    }

    glfwTerminate();
    free(points);
    free(indices);

    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ibo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);

    return 0;
}
