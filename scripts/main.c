#include <pigpio.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <wiringPi.h>
#include <string.h>

// Add TFTGL library
#include <tftgl.h>

// Add NANOVG library
#include <nanovg.h>
#define NANOVG_GLES2_IMPLEMENTATION	// Use GLES 2 implementation.
#include <nanovg_gl.h>
#include <bcm2835.h>

static volatile uint32_t* gpioData = NULL;

#define GPIO_GPFSET0 (BCM2835_GPSET0/4)
#define GPIO_GPFCLR0 (BCM2835_GPCLR0/4)
#define GPIO_WRITE_PIN(pinnum, pinstate) \
	*(gpioData + (pinstate ? GPIO_GPFSET0 : GPIO_GPFCLR0)) = (1 << pinnum);

#define MAX_CHILDREN 5
#define MAX_OBJECTS 100


int handle;
int getValueX();
int getValueY();
int isPressed = 0; // 1 if the touch screen is pressed
int touch_down = 0;

struct NVGcontext* vg;
GLint width, height;
int current_scene = 0;

struct Text {
	char * s;
	int len, max_len;

	int font, font_size;
	int x, y, dx, dy;
	int r, g, b;
	int hide;
};

struct Box {
	int x1, y1, x2, y2;
	int r, g, b, a, w;
	int hide;
};

struct Rect {
	int x1, y1, x2, y2;
	int r, g, b, a;
	int hide;
};

struct TouchArea {
	int x1, y1, x2, y2;
};

struct Object {
	int scene, id, priority;
	int hide;

	int cnt_rects, cnt_boxes, cnt_texts;
	struct Rect rects[MAX_CHILDREN];
	struct Box boxes[MAX_CHILDREN];
	struct Text texts[MAX_CHILDREN];

	int can_touch, has_focus;
	struct TouchArea tarea;
};

int isTouched(struct Object * object, int x, int y) {
	if (object->can_touch) {
		int tx = object->tarea.x1 <= x && object->tarea.x2 >= x;
		int ty = object->tarea.y1 <= y && object->tarea.y2 >= y;
		return tx && ty;
	}
	return 0;
}

void setTouchArea(struct Object * object, int x1, int y1, int x2, int y2) {
	object->can_touch = 1;
	object->tarea.x1 = x1;
	object->tarea.y1 = y1;
	object->tarea.x2 = x2;
	object->tarea.y2 = y2;
}

void defaultObject(struct Object * object) {
	object->cnt_rects = object->cnt_boxes = object->cnt_texts = 0;
	object->can_touch = object->has_focus = 0;
	object->scene = object->id = object->priority = object->hide = 0;
}

void addColorRect(struct Object * object, int x1, int y1, int x2, int y2, int r, int g, int b, int a) {
	int cnt_rects = object->cnt_rects;
	object->rects[cnt_rects].x1 = x1;
	object->rects[cnt_rects].y1 = y1;
	object->rects[cnt_rects].x2 = x2;
	object->rects[cnt_rects].y2 = y2;
	object->rects[cnt_rects].r = r;
	object->rects[cnt_rects].g = g;
	object->rects[cnt_rects].b = b;
	object->rects[cnt_rects].a = a;
	object->rects[cnt_rects].hide = 0;
	object->cnt_rects++;
}

void addRect(struct Object * object, int x1, int y1, int x2, int y2) {
	addColorRect(object, x1, y1, x2, y2, 0, 0, 0, 255);
}

void addColorBox(struct Object * object, int x1, int y1, int x2, int y2, int r, int g, int b, int a, int w) {
	int cnt_boxes = object->cnt_boxes;
	object->boxes[cnt_boxes].x1 = x1;
	object->boxes[cnt_boxes].y1 = y1;
	object->boxes[cnt_boxes].x2 = x2;
	object->boxes[cnt_boxes].y2 = y2;
	object->boxes[cnt_boxes].r = r;
	object->boxes[cnt_boxes].g = g;
	object->boxes[cnt_boxes].b = b;
	object->boxes[cnt_boxes].a = a;
	object->boxes[cnt_boxes].w = w;
	object->boxes[cnt_boxes].hide = 0;
	object->cnt_boxes++;
}

void addBox(struct Object * object, int x1, int y1, int x2, int y2, int w) {
	addColorBox(object, x1, y1, x2, y2, 0, 0, 0, 255, w);
}

void addColorText(struct Object * object, int x, int y, int dx, int dy, int font, int font_size, int r, int g, int b) {
	int cnt_texts = object->cnt_texts;
	object->texts[cnt_texts].x = x;
	object->texts[cnt_texts].y = y;
	object->texts[cnt_texts].dx = dx;
	object->texts[cnt_texts].dy = dy;
	object->texts[cnt_texts].font = font;
	object->texts[cnt_texts].font_size = font_size;
	object->texts[cnt_texts].r = r;
	object->texts[cnt_texts].g = g;
	object->texts[cnt_texts].b = b;
	object->texts[cnt_texts].hide = 0;
	object->cnt_texts++;
}

void addText(struct Object * object, int x, int y, int dx, int dy, int font, int font_size) {
	addColorText(object, x, y, dx, dy, font, font_size, 0, 0, 0);
}

struct Object * objects[MAX_OBJECTS];
int cnt_objects = 0;

void addObject(struct Object * object, int scene) {
	object->scene = scene;
	objects[cnt_objects] = object;
	cnt_objects++;
	int j = cnt_objects - 2;
	while (j >= 0 && objects[j]->priority > objects[j+1]->priority) {
		struct Object * temp = objects[j];
		objects[j] = objects[j+1];
		objects[j+1] = temp;
		j--;
	}
	object->id = j + 1;
}

void drawRect(struct Rect * rect) {
	if (rect->hide) return;
	nvgBeginPath(vg);
	nvgMoveTo(vg, rect->x1, rect->y1);
	nvgLineTo(vg, rect->x2, rect->y1);
	nvgLineTo(vg, rect->x2, rect->y2);
	nvgLineTo(vg, rect->x1, rect->y2);
	nvgLineTo(vg, rect->x1, rect->y1);
	nvgFillColor(vg, nvgRGBA(rect->r, rect->g, rect->b, rect->a));
	nvgFill(vg);
}

void drawBox(struct Box * box) {
	if (box->hide) return;
}

void drawText(struct Text * text) {
	if (text->hide) return;
}


// Priority:
// 1. Rectangles
// 2. Boxes
// 3. Texts
void drawObject(struct Object * object) {
	if (object->hide) return;
	for (int i = 0; i < object->cnt_rects; i++) {
		drawRect(&object->rects[i]);
	}
	for (int i = 0; i < object->cnt_boxes; i++) {
		drawBox(&object->boxes[i]);
	}
	for (int i = 0; i < object->cnt_texts; i++) {
		drawText(&object->texts[i]);
	}
}

void drawScene() {
	for (int i = 0; i < cnt_objects; i++) {
		if (objects[i]->scene == current_scene) {
			drawObject(objects[i]);
		}
	}
}

void registerTouch(int x, int y) {
	for (int i = 0; i < cnt_objects; i++) {
		if (isTouched(objects[i], x, y) && objects[i]->scene == current_scene) {
			printf("touched object at index %i,  x = %i, y = %i\n", i, x, y);
			fflush(stdout);
		}
	}
}

int main()
{
    int speed = 1000000;

    // GPIO initialization
    if (gpioInitialise() < 0) {
        printf("initialize error");
        exit(1);
    }

    // SPI initialization

    handle = spiOpen(0, speed, 0);
    if (handle < 0) {
        printf("SPI open error");
    }

	double pxRatio;
	unsigned int i;

	// Initialize tftgl!
	if(tftglInit(TFTGL_LANDSCAPE) != TFTGL_OK){
		fprintf(stderr, "Failed to initialize TFTGL library! Error: %s\n",
			tftglGetErrorStr());
		return EXIT_FAILURE;
	}

	// Set brightness to full 100%
	tftgSetBrightness(255);

	// Get viewport size
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	printf("TFT display initialized with EGL! Screen size: %dx%d\n",
		viewport[2], viewport[3]);

	width = viewport[2];
	height = viewport[3];

	glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
	vg = nvgCreateGLES2(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
	nvgBeginFrame(vg, width, height, 1.0);

	// Touch and draw variables
	int px = 2*width, py = 2*height;
	int mdraw = 1;
	int draw = mdraw - 1;
	int cycles_to_next_touch = 4;

	int font = nvgCreateFont(vg, "sans", "FreeSans.ttf");

	// priority = 0, scene = 0
	struct Object a, b;
	{
		defaultObject(&a);
		a.priority = 1;
		addObject(&a, 0);
		addColorRect(&a, 0, 0, width, height, 255, 255, 255, 255);
		addRect(&a, 100, 100, width - 100, height - 100);
		addColorRect(&a, 150, 150, width - 150, height - 150, 255, 0, 0, 255);
		setTouchArea(&a, 0, 0, width, height);
	}

	{
		defaultObject(&b);
		b.priority = 0;
		addObject(&b, 1);
		addColorRect(&b, 50, 50, width - 50, height - 50, 0, 255, 0, 255);
	}

	current_scene = 0;
	while (1)
    {
		// REGISTER TOUCH INPUT
		{
	        int x = 0, y = 0;
			int tcnt = 0;
			for (int i = 0; i < 20; i++) {
		        int tx = getValueX(), ty = getValueY();
				if (tx < width && ty < height) {
					if(px >= width || ((px - tx) * (px - tx) + (py - ty) * (py - ty) < 20000)) {
						tcnt++;
						x += tx; y += ty;
					}
				}
				delay(1);
			}
			if (tcnt > 0) {
				x /= tcnt;
				y /= tcnt;
				px = x;
				py = y;
				if (touch_down == 0) {
					registerTouch(x, y);
				}
				touch_down = cycles_to_next_touch - 1;
			} else {
				px = 2*width;
				py = 2*height;
				if (touch_down > 0) {
					touch_down--;
				}
			}
		}
		// REGISTER TOUCH INPUT




		drawScene();
		// Draw frame every 'mdraw' cycles of this loop. The neccesay delay is provided by reading the touch input data
		if (draw == 0) {
			nvgEndFrame(vg);
			tftglUploadFbo();
			nvgBeginFrame(vg, width, height, 1.0);
			draw = mdraw;
		}
		draw -= 1;
    }

    // Terminate SPI and GPIO

    spiClose(handle);

    gpioTerminate();

    return 0;
}

int getValueY()
{
    int value;
    unsigned char buf[3] = { 0x94, 0x00, 0x00 };
    spiXfer(handle, buf, buf, 3);
	float mult = 0.015869140625;
	float mult2 = 1.1733333333333333;
    value = round(((((buf[1] & 0x7f) << 8) | buf[2]) * mult - 45.0) * mult2);
    return value;
}

int getValueX()
{
    int value;
    unsigned char buf[3] = { 0xD4, 0x00, 0x00 };
    spiXfer(handle, buf, buf, 3);
	float mult = 0.0244140625;
	float mult2 = 1.1111111111111112;
    value = round(800.0 - ((((buf[1] & 0x7f) << 8) | buf[2]) * mult - 40.0) * mult2);
    return value;
}
