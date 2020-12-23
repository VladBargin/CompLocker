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


#define ROWS 2
#define COLS 3

char pressedKey = '\0';
int rowPins[ROWS] = {8, 11};
int colPins[COLS] = {10, 13, 12};

char keys[ROWS][COLS] = {
   {'1', '2', '3', 'A'},
   {'4', '5', '6', 'B'},
   {'7', '8', '9', 'C'},
   {'*', '0', '#', 'D'}
};

void init_keypad()
{
   for (int c = 0; c < COLS; c++) {
      pinMode(colPins[c], OUTPUT);
      digitalWrite(colPins[c], HIGH);
   }

   for (int r = 0; r < ROWS; r++) {
      pinMode(rowPins[0], INPUT);
      pullUpDnControl(rowPins[r], PUD_UP);
   }
}

int findLowRow() {
   for (int r = 0; r < ROWS; r++) {
      if (digitalRead(rowPins[r]) == LOW)
         return r;
   }

   return -1;
}

char get_key() {
   int rowIndex;

   for (int c = 0; c < COLS; c++) {
      digitalWrite(colPins[c], LOW);
      rowIndex = findLowRow();
      if (rowIndex > -1) {
         if (!pressedKey)
            pressedKey = keys[rowIndex][c];
         return pressedKey;
      }
      digitalWrite(colPins[c], HIGH);
   }
   pressedKey = '\0';
   return pressedKey;
}

struct TextBox {
	char * s;
	int len, max_len, x1, y1, x2, y2, font, r, g, b, fill;
	float width;
};

void init(struct TextBox *tbox, int max_len, int x1, int y1, int x2, int y2, float w, int font, int r, int g, int b, int fill) {
	tbox->s = (char *)malloc(max_len + 2);
	tbox->s[0] = 0;
	tbox->len = 0;
	tbox->max_len = max_len;
	tbox->x1 = x1;
	tbox->y1 = y1;
	tbox->x2 = x2;
	tbox->y2 = y2;
	tbox->width = w;
	tbox->font = font;
	tbox->r = r;
	tbox->g = g;
	tbox->b = b;
	tbox->fill = fill;
}

void destroy(struct TextBox *tbox) {
	free(tbox->s);
}

void addLetter(struct TextBox *tbox, char c) {
	if (tbox->len < tbox->max_len) {
		tbox->s[tbox->len] = c;
		tbox->len++;
		tbox->s[tbox->len] = 0;
	}
}

void removeLetter(struct TextBox *tbox) {
	if (tbox->len > 0) {
		tbox->s[tbox->len] = 0;
		tbox->len--;
	}
}


void drawBox(struct NVGcontext *vg, int x1, int y1, int x2, int y2, float w) {
	nvgBeginPath(vg);
	nvgMoveTo(vg, x1, y1);
	nvgLineTo(vg, x2, y1);
	nvgLineTo(vg, x2, y2);
	nvgLineTo(vg, x1, y2);
	nvgLineTo(vg, x1, y1);
	nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 255));
	nvgStrokeWidth(vg, w);
	nvgStroke(vg);
}

void drawRect(struct NVGcontext *vg, int x1, int y1, int x2, int y2, int r, int g, int b, int a) {
	nvgBeginPath(vg);
	nvgMoveTo(vg, x1, y1);
	nvgLineTo(vg, x2, y1);
	nvgLineTo(vg, x2, y2);
	nvgLineTo(vg, x1, y2);
	nvgLineTo(vg, x1, y1);
	nvgFillColor(vg, nvgRGBA(r, g, b, a));
	nvgFill(vg);
}

void draw(struct TextBox *tbox, struct NVGcontext *vg, int offset) {
	if (tbox->fill) {
		drawRect(vg, tbox->x1, tbox->y1, tbox->x2, tbox->y2, tbox->r, tbox->g, tbox->b, 255);
	}
	drawBox(vg, tbox->x1, tbox->y1, tbox->x2, tbox->y2, tbox->width);
	nvgFontFaceId(vg, tbox->font);
	nvgFontSize(vg, 50);
	nvgFillColor(vg, nvgRGBA(0,0,0,255));
	nvgText(vg, tbox->x1 + 10, tbox->y2 - offset, tbox->s, tbox->s + tbox->len);
}


//==============================================================================
int main(int argv, char** argc){
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

	GLint width = viewport[2];
	GLint height = viewport[3];

	glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

	struct NVGcontext* vg = nvgCreateGLES2(NVG_ANTIALIAS | NVG_STENCIL_STROKES);

	wiringPiSetup();

  	init_keypad();

//	float bounds[4] = {300, 100, 500, 300};
	int font = nvgCreateFont(vg, "sans", "FreeSans.ttf");
	struct TextBox tb1, tb2, tb3, tb4, tb5, tb6;
	init(&tb1, 15, 310, 100, width - 105, 150, 8, font, 0, 0, 0, 0);
	init(&tb2, 10, 95, 100, 210, 150, 0, font, 0, 0, 0, 0);
	init(&tb3, 15, 310, 180, width - 105, 230, 4, font, 0, 0, 0, 0);
	init(&tb6, 15, 310, 180, width - 105, 230, 4, font, 0, 0, 0, 0);
	init(&tb4, 10, 95, 180, 210, 230, 0, font, 0, 0, 0, 0);
	init(&tb5, 10, 362, 340, 438, 390, 4, font, 0, 190, 0, 1);
	addLetter(&tb2, 'L');
	addLetter(&tb2, 'o');
	addLetter(&tb2, 'g');
	addLetter(&tb2, 'i');
	addLetter(&tb2, 'n');

	addLetter(&tb4, 'P');
	addLetter(&tb4, 'a');
	addLetter(&tb4, 's');
	addLetter(&tb4, 's');
	addLetter(&tb4, 'w');
	addLetter(&tb4, 'o');
	addLetter(&tb4, 'r');
	addLetter(&tb4, 'd');

	addLetter(&tb5, 'O');
	addLetter(&tb5, 'K');

	int ci = 3;
	char pc = 0;
	char param[200];
	int ticks = 1, lock = 14, match = 0;
	int cnt = 0;
	while(1) {
		int dl = 50;

		nvgBeginFrame(vg, width, height, 1.0);

		drawRect(vg, 0, 0, width, height, 255, 255, 255, 255);

		draw(&tb1, vg, 12);
		draw(&tb2, vg, 12);
		draw(&tb6, vg, 6);
		draw(&tb4, vg, 12);
		drawBox(vg, 55, 55, width - 55, 275, 4);

		draw(&tb5, vg, 12);

	    	char x = get_key();

		if (x && pc != x) {
			printf("pressed: %c\n", x);
			if (x == '1') {
				cnt++;
				if (cnt >= 3) {
					printf("DEAD");
					break;
				}
			} else {
				cnt = 0;
			}

			if (x == '6') {
				cnt++;
				ci += 1;
				tb1.width = 4;
				tb6.width = 4;
				tb5.width = 4;
				if (ci % 3 == 0) tb1.width = 8;
				else if (ci % 3 == 1) tb6.width = 8;
				else tb5.width = 8;
			}
			else {
				if (ci % 3 == 0) addLetter(&tb1, x);
				else if (ci % 3 == 1) {
					addLetter(&tb3, x);
					addLetter(&tb6, '*');
				} else {
					sprintf(param, "python3 login.py %s %s", tb1.s, tb3.s);
					int res = system(param);
					match = 0;
					if (res == 0) {
						match = 1;
						ticks = 150;
					} else {
						ticks = 50;
					}
					while (tb1.len > 0) removeLetter(&tb1);
					while (tb3.len > 0) {
						removeLetter(&tb3);
						removeLetter(&tb6);
					}
				}
			}
		}
		pc = x;
	//	printf("%i\n", ticks);
		if (ticks > 1) {
			drawRect(vg, 0, 0, width, height, 0, 0, 0, 128);
			int r = 0, g = 0, b = 0;
			if (match) {
				g = 200;
			} else {
				r = 200;
			}
			drawRect(vg, 200, 120, 600, 360, r, g, b, 255);
			drawBox(vg, 200, 120, 600, 360, 4);
			nvgEndFrame(vg);
			tftglUploadFbo();
			delay(1000);
			nvgBeginFrame(vg, width, height, 1.0);
		}

		if (ticks > 1) {
			drawRect(vg, 0, 0, width, height, 0, 0, 0, 128);
			int r = 0, g = 0, b = 0;
			if (match) {
				g = 200;
			} else {
				r = 200;
			}
			drawRect(vg, 200, 120, 600, 360, r, g, b, 255);
			drawBox(vg, 200, 120, 600, 360, 4);
		}
		nvgEndFrame(vg);
		tftglUploadFbo();
		if (match) {
			delay(1000);
			pinMode(lock, OUTPUT);
			digitalWrite(lock, LOW);
		}
		delay(dl*ticks);

		if (match) {
			pinMode(lock, INPUT);
			pullUpDnControl(lock, PUD_UP);
			match = 0;
			delay(1000);
		}
		ticks = 1;
	}


	nvgDeleteGLES2(vg);

	// Terminates everything (GPIO, LCD, and EGL)
	tftglTerminate();

	return EXIT_SUCCESS;
}
