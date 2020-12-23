#include <pigpio.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <wiringPi.h>
#include <string.h>
#include <time.h>

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
	int r, g, b, a;
	int hide;
	int pswd;
};

void addString(struct Text * text, char * s) {
	int i = text->len;
	while (i < text->max_len && s[i - text->len]) {
		if (text->pswd) text->s[i] = '*';
		else text->s[i] = s[i - text->len];
		i++;
	}
	text->len = i;
	text->s[i] = 0;
}

void addChar(struct Text * text, char c) {
	if (text->len == text->max_len) return;
	if (text->pswd) text->s[text->len] = '*';
	else text->s[text->len] = c;
	text->len++;
	text->s[text->len] = 0;
}

void clearText(struct Text * text) {
	text->len = 0;
	text->s[0] = 0;
}

// Remove last 'n' letters from 'text->s'
void removeText(struct Text * text, int n) {
	while (n-- && text->len > 0) {
		text->s[text->len - 1] = 0;
		text->len--;
	}
}

void removeChar(struct Text * text) {
	removeText(text, 1);
}

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
	int touch_event, data;
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
	object->data = 0;
	object->touch_event = -1;
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

struct Text* addColorText(struct Object * object, int x, int dx, int y, int dy, int font, int font_size, int r, int g, int b, int a, int max_len) {
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
	object->texts[cnt_texts].a = a;
	object->texts[cnt_texts].hide = 0;
	object->texts[cnt_texts].s = (char *) malloc(sizeof(char) * (max_len + 1));
	object->texts[cnt_texts].s[0] = 0;
	object->texts[cnt_texts].max_len = max_len;
	object->texts[cnt_texts].len = 0;
	object->texts[cnt_texts].pswd = 0;
	object->cnt_texts++;
	return &object->texts[cnt_texts];
}

struct Text* addText(struct Object * object, int x, int dx, int y, int dy, int font, int font_size, int max_len) {
	return addColorText(object, x, dx, y, dy, font, font_size, 0, 0, 0, 255, max_len);
}

struct Object * objects[MAX_OBJECTS];
int cnt_objects = 0;

// Also sorts objects by priority in ascending order
void addObject(struct Object * object, int scene) {
	object->scene = scene;
	objects[cnt_objects] = object;
	object->id = cnt_objects;
	cnt_objects++;
	int j = cnt_objects - 2;
	while (j >= 0 && objects[j]->priority > objects[j+1]->priority) {
		struct Object * temp = objects[j];
		objects[j] = objects[j+1];
		objects[j+1] = temp;
		j--;
	}
}

void addObjectToAll(struct Object * object) {
	addObject(object, -1);
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
	nvgBeginPath(vg);
	nvgMoveTo(vg, box->x1, box->y1);
	nvgLineTo(vg, box->x2, box->y1);
	nvgLineTo(vg, box->x2, box->y2);
	nvgLineTo(vg, box->x1, box->y2);
	nvgLineTo(vg, box->x1, box->y1);
	nvgStrokeColor(vg, nvgRGBA(box->r, box->g, box->b, box->a));
	nvgStrokeWidth(vg, box->w);
	nvgStroke(vg);
}

float bounds[4];

void drawText(struct Text * text) {
	if (text->hide) return;
	nvgFontFaceId(vg, text->font);
	nvgFontSize(vg, text->font_size);
	nvgFillColor(vg, nvgRGBA(text->r, text->g, text->b, text->a));
	nvgText(vg, text->x + text->dx, text->y + text->dy, text->s, text->s + text->len);
}

/*
Priority:
1. Rectangles
2. Boxes
3. Texts
*/
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
		if (objects[i]->scene == current_scene || objects[i]->scene < 0) {
			drawObject(objects[i]);
		}
	}
}

void touchEvent(struct Object * object, int x, int y);

void registerTouch(int x, int y) {
	for (int i = cnt_objects - 1; i >= 0; i--) {
		if (isTouched(objects[i], x, y) && (objects[i]->scene == current_scene || objects[i]->scene < 0)) {
			touchEvent(objects[i], x, y);
			printf("touched object at index %i,  x = %i, y = %i\n", i, x, y);
			fflush(stdout);
			break;
		}
	}
}


int current_computer = 0;
#define NUM_SCENES 5
/*
	SCENES:

0 - Contains the option(s) to select a computer to take
1 - Authorisation
2 - Access granted
3 - Access denied
4 - Help!
*/


// TIME
struct Object timeTextObj;
struct Text* timeText;

void updateTime() {
	time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
	char * ts = (char *) malloc(sizeof(char) * 6);
	ts[5] = 0;
	ts[0] = '0' + timeinfo->tm_hour / 10 % 10;
	ts[1] = '0' + timeinfo->tm_hour % 10;
	ts[2] = ':';
	ts[3] = '0' + timeinfo->tm_min / 10 % 10;
	ts[4] = '0' + timeinfo->tm_min % 10;

	clearText(timeText);
	addString(timeText, ts);
}

void updateTimeTextColorAndPos() {
	int col = 0;
	if (current_scene == 2 || current_scene == 3) {
		col = 255;
	}
	timeText->r = col;
	timeText->g = col;
	timeText->b = col;
	timeText->a = 255;

////	if (current_scene == 0) {
	//			timeTextObj = addText(&timeTextObj, width - 100, 35, 0, 0, font, 32, 5);
	//}
}
// TIME



struct Object backgrounds[NUM_SCENES];
struct Object bigLogo, smallLogo;
struct Object passwdText;

#define LEN_PASSWD 6
char passwd[LEN_PASSWD + 1];

#define MAX_COMP 50
struct Object computerIcon[MAX_COMP];

#define MAX_BUTTONS 20
struct Object buttons[MAX_BUTTONS];

struct Object backButton[NUM_SCENES];

int computerStatus(int id);
char * idToName(int id);
int logAttempt(int id, char * pswd);


void updateColor(struct Object * object, int status) {
	int r, g, b;
	r = g = b = 0;
	if (status == 0) {
		r = g = b = 127;
	} else if (status == 1) {
		r = 240;
		g = 240;
	} else if (status == 2) {
		r = 240;
	} else if (status == 3) {
		g = 240;
	}
	object->rects[0].r = r;
	object->rects[0].g = g;
	object->rects[0].b = b;
}

void updateText(struct Object * object, int id) {
	char * text = idToName(id);
	clearText(&(object->texts[0]));
	addString(&(object->texts[0]), text);
	printf("id: %i, s: %s, s2: %s\n", id, text, object->texts[0].s);
	fflush(stdout);
	free(text);
}

void changeScene(int scene){
	current_scene = scene;
	updateTimeTextColorAndPos();

	passwd[0] = 0;
	clearText(&(passwdText.texts[0]));

	int st = computerStatus(current_computer);
	updateColor(&bigLogo, st);
	updateColor(&smallLogo, st);

	updateText(&bigLogo, current_computer);
	updateText(&smallLogo, current_computer);

	if (scene == 0) {
		timeText->dx = -124;
		timeText->dy = 89;
	} else if (scene == 1) {
		timeText->dx = -95;
		timeText->dy = 55;
	} else {
		timeText->dx = -95;
		timeText->dy = 35;
	}


	printf("scene: %d, c_c: %d\n", scene, current_computer);
	fflush(stdout);
}

void addCharP(char c) {
	addChar(&(passwdText.texts[0]), c);
	int i = 0;
	while (i < LEN_PASSWD) {
		if (passwd[i] == 0) {
			passwd[i] = c;
			passwd[i + 1] = 0;
			break;
		}
		i++;
	}
}

void remCharP() {
	removeText(&(passwdText.texts[0]), 1);
	int i = 0;
	while (i < LEN_PASSWD) {
		if (passwd[i]) passwd[i] = 0;
		i++;
	}
}

void touchEvent(struct Object * object, int x, int y) {
	int _ev = object->touch_event, data = object->data;
	if (_ev < 0) return;

	if (_ev == 0) {
		changeScene(data);
	} else if (_ev == 1) {
		current_computer = data;
		changeScene(1);
	} else if (_ev == 2) {
		if (computerStatus(current_computer)) addCharP(data);
	} else if (_ev == 3) {
		remCharP();
	} else if (_ev == 4) {
		if (computerStatus(current_computer)) {
			int res = logAttempt(current_computer, passwd);
			if (res) {
				changeScene(2);
			} else {
				changeScene(3);
			}
		}
	}
}

/*
0 - No computer							(grey)
1 - No sensor							(yellow)
2 - Computer not avaliable right now	(red)
3 - Computer is avaliable				(green)
*/
int computerStatus(int id) {
	if (id < 2) return 1;
	if (id == 9) return 2;
	if (id == 8) return 3;
	return 0;
}

int correctPassword(int id, char * pswd) {
	return (id == 0) || (id == 9);
}

// 0 - unsuccessful attempt, 1 - successful attempt
int logAttempt(int id, char * pswd) {
	int res = correctPassword(id, pswd);

	/*
		LOG SOMETHING
	*/

//	int pid = fork();
//	if (pid == 0) {
		system("bash /home/pi/Documents/CompLocker/scripts/cam.sh");
//		exit(0);
//	}
	/*
		LOG SOMETHING
	*/

	printf("id: %i, res: %i", id, res);
	return res;
}

char * idToName(int id) {
	char * ts = (char *) malloc(sizeof(char) * 3);
	ts[0] = id / 10 % 10 + '0';
	ts[1] = id % 10 + '0';
	ts[2] = 0;
	return ts;
}

int abs(int a) {
	if (a >= 0) return a;
	return -a;
}

int dist(int x1, int y1, int x2, int y2) {
	return abs(x1 - x2) + abs(y1 - y2);
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
	int mdraw = 60;
	int draw = mdraw - 1;
	int cycles_to_next_touch = 4;

	int font = nvgCreateFont(vg, "sans", "CourierNewBd.ttf");

	// priority = 0, scene = 0


	for (int i = 0; i < NUM_SCENES; i++) {
		defaultObject(&backgrounds[i]);
		backgrounds[i].priority = -1;
		addObject(&backgrounds[i], i);
	}
	addColorRect(&backgrounds[0], 0, 0, width, height, 255, 255, 255, 255);
	addColorRect(&backgrounds[1], 0, 0, width, height, 255, 255, 255, 255);
	addColorRect(&backgrounds[2], 0, 0, width, height, 0, 32, 0, 255);
	addColorRect(&backgrounds[3], 0, 0, width, height, 64, 0, 0, 255);
	addColorRect(&backgrounds[4], 0, 0, width, height, 255, 255, 255, 255);

	{
		defaultObject(&bigLogo);
		bigLogo.priority = 1;
		addBox(&bigLogo, 10, 90, 370, 470, 2);
		addRect(&bigLogo, 10, 90, 370, 470);
		addText(&bigLogo, 200, -52, 278, 24, font, 100, 3);
		addObject(&bigLogo, 1);
	}

	int d = 150;
	{
		defaultObject(&smallLogo);
		smallLogo.priority = 1;
	//	addColorBox(&smallLogo, 300, 140, 500, 340, 255, 255, 255, 255, 2);
	//	addRect(&smallLogo, 300, 140, 500, 340);
		int x1 = width/2 - d/2, y1 = height/2 - d/2, x2 = width/2 + d/2, y2 = height/2 + d/2;
		addColorBox(&smallLogo, x1, y1, x2, y2, 255, 255, 255, 255, 2);
		addRect(&smallLogo, x1, y1, x2, y2);
		addText(&smallLogo, 400, -26, 240, 13, font, 50, 3);
		addObject(&smallLogo, 2);
	}

	{
		int cw = 5, ch = 3;
		int tw = width / cw, th = height / ch;
		int dx = (tw - d) / 2;
		int dy = (th - d) / 2;
		if (dy < dx) dx = dy;
		int ti = 0;
		for (int i = 0; i < ch; i++) {
			for (int j = 0; j < cw; j++) {
				if (i == 0 && j == cw - 1) continue;
				defaultObject(&computerIcon[ti]);
				computerIcon[ti].priority = 1;
				computerIcon[ti].data = ti;
				computerIcon[ti].touch_event = 1;
				int x1, y1, x2, y2;
				x1 = j * width / cw;
				y1 = i * height / ch;
				x2 = (j + 1) * width / cw;
				y2 = (i + 1) * height / ch;
				setTouchArea(&computerIcon[ti], x1, y1, x2, y2);
				addRect(&computerIcon[ti], x1 + dx, y1 + dx, x2 - dx, y2 - dx);
				addBox(&computerIcon[ti], x1 + dx, y1 + dx, x2 - dx, y2 - dx, 2);
				addText(&computerIcon[ti], x1 + tw / 2, -26, y1 + th / 2, 13, font, 50, 5);
				updateColor(&computerIcon[ti], computerStatus(ti));
				updateText(&computerIcon[ti], ti);
				addObject(&computerIcon[ti], 0);
				ti++;
			}
		}
		{
			defaultObject(&timeTextObj);
			timeTextObj.priority = 10;
			timeText = addText(&timeTextObj, width, -115, 0, 40, font, 32, 5);
			addObjectToAll(&timeTextObj);
		}


		{
			d = 82;
			int x1 = 385, y1 = 180, x2 = width - 5, y2 = height - 5;
			int cw = 4, ch = 3;
			int ti = 0;
			int tw = (x2 - x1) / cw;
			int th = (y2 - y1) / ch;
			int dx = (tw - d) / 2;
			int dy = (th - d) / 2;
			if (dy < dx) dx = dy;
			for (int i = 0; i < ch; i++) {
				for (int j = 0; j < cw; j++) {
					defaultObject(&buttons[ti]);
					buttons[ti].priority = 1;
					buttons[ti].data = ti;
					buttons[ti].touch_event = 2;
					int tx1, tx2, ty1, ty2;
					tx1 = j * (x2 - x1) / cw + x1;
					tx2 = (j + 1) * (x2 - x1) / cw + x1;
					ty1 = i * (y2 - y1) / ch + y1;
					ty2 = (i + 1) * (y2 - y1) / ch + y1;
					setTouchArea(&buttons[ti], tx1, ty1, tx2, ty2);
					addColorRect(&buttons[ti], tx1 + dx, ty1 + dy, tx2 - dx, ty2 - dy, 0, 128, 255, 255);
					addBox(&buttons[ti], tx1 + dx, ty1 + dy, tx2 - dx, ty2 - dy, 2);
					struct Text * tp = addText(&buttons[ti], tx1 + tw / 2, -13, ty1 + th / 2, 13, font, 50, 1);
					buttons[ti].touch_event = 2;
					char bChar = ti + '0';
					if (ti == 10) {
						buttons[ti].touch_event = 3;
						bChar = '<';
					} else if (ti == 11) {
						buttons[ti].touch_event = 4;
						bChar = '>';
					} else {
						buttons[ti].data = bChar;
					}
					addChar(tp, bChar);
					addObject(&buttons[ti], 1);
					ti++;
				}
			}

			{
				defaultObject(&passwdText);
				addBox(&passwdText, 390, 90, width - 10, 175, 2);
				addObject(&passwdText, 1);
				addText(&passwdText, 390, 35 - 13, 132, 17, font, 50, LEN_PASSWD);
			}
		}
	}

	{
		for (int i = 0; i < NUM_SCENES; i++) {
			defaultObject(&backButton[i]);
			addBox(&backButton[i], 10, 10, 80, 80, 2);
			struct Text * tmp = addText(&backButton[i], 45, -13, 45, 13, font, 50, 1);
			addChar(tmp, '<');
			backButton[i].priority = 2;
			backButton[i].touch_event = 0;
			backButton[i].data = 0;
			setTouchArea(&backButton[i], 10, 10, 80, 80);
		}
		addObject(&backButton[1], 1);
		addObject(&backButton[2], 2);
		addObject(&backButton[3], 3);
		backButton[3].data = 1;
	}

	changeScene(0);
	passwdText.texts[0].pswd = 0;

#define CNT_READS 20
	int touchData[CNT_READS][2];

	for (int i = 0; i < CNT_READS; i++) touchData[i][0] = touchData[i][1] = width;

	struct Object tmp;
	defaultObject(&tmp);
	addColorBox(&tmp, 0, 0, 1, 1, 255, 0, 0, 128, 10);
	tmp.priority = 20;
	addObjectToAll(&tmp);

	while (1)
    {
		// REGISTER TOUCH INPUT
		{
			for (int i = CNT_READS - 1; i > 0; i--) {
				touchData[i][0] = touchData[i - 1][0];
				touchData[i][1] = touchData[i - 1][1];
			}
			touchData[0][0] = getValueX();
			touchData[0][1] = getValueY();
			int g = 1;

#define MANXATAN_SPREAD 200
			for (int i = 0; i < CNT_READS; i++) {
				if (touchData[i][0] >= width) g = 0;
				if (!g) break;
				for (int j = i + 1; j < CNT_READS; j++) {
					if (dist(touchData[i][0], touchData[i][1], touchData[j][0], touchData[j][1]) > MANXATAN_SPREAD) {
						g = 0;
						break;
					}
				}
			}

			if (g) {
				int x = 0, y = 0;
				for (int i = 0; i < CNT_READS; i++) {
					x += touchData[i][0];
					y += touchData[i][1];
				}
				x /= CNT_READS;
				y /= CNT_READS;
				if (touch_down == 0) {
					tmp.boxes[0].x1 = x - 50;
					tmp.boxes[0].x2 = x + 50;
					tmp.boxes[0].y1 = y - 50;
					tmp.boxes[0].y2 = y + 50;
					registerTouch(x, y);
				}
				touch_down = cycles_to_next_touch - 1;
			} else {
				if (touch_down > 0) {
					touch_down--;
				}
			}
			delay(1);
		}
		// REGISTER TOUCH INPUT


		// Draw frame every 'mdraw' cycles of this loop. The neccesay delay is provided by reading the touch input data
		if (draw == 0) {
			drawScene();
			nvgEndFrame(vg);
			tftglUploadFbo();
			nvgBeginFrame(vg, width, height, 1.0);
			draw = mdraw;
			updateTime();
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
