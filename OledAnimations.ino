#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Define physical pins for I2C on the integrated OLED board
// SDA is GPIO 14 (D5 pin), SCL is GPIO 12 (D6 pin)
#define OLED_SDA 14
#define OLED_SCL 12

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Animation switching variables
unsigned long lastSwitchTime = 0;
const unsigned long ANIMATION_DURATION = 6000; // Switch animation every 6 seconds
int currentAnimation = 0;
const int TOTAL_ANIMATIONS = 5;

// ==========================================
// 1. Starfield Animation Variables & Structs
// ==========================================
#define NUM_STARS 45
struct Star {
  float x;
  float y;
  float z;
};
Star stars[NUM_STARS];

void initStarfield() {
  for (int i = 0; i < NUM_STARS; i++) {
    stars[i].x = random(-64, 64);
    stars[i].y = random(-32, 32);
    stars[i].z = random(1, 128); // Depth
  }
}

void drawStarfield() {
  float centerX = SCREEN_WIDTH / 2;
  float centerY = SCREEN_HEIGHT / 2;
  
  for (int i = 0; i < NUM_STARS; i++) {
    // Move star closer to the screen
    stars[i].z -= 2.0;
    
    // If it reaches the screen or goes past, reset to the back
    if (stars[i].z <= 0) {
      stars[i].x = random(-64, 64);
      stars[i].y = random(-32, 32);
      stars[i].z = 128;
    }
    
    // 3D projection
    int screenX = (int)(centerX + (stars[i].x / stars[i].z) * 100);
    int screenY = (int)(centerY + (stars[i].y / stars[i].z) * 100);
    
    // Draw star if within bounds
    if (screenX >= 0 && screenX < SCREEN_WIDTH && screenY >= 0 && screenY < SCREEN_HEIGHT) {
      // Determine brightness/size based on depth
      if (stars[i].z < 40) {
        // Draw larger star (4-pixel cross) for close stars
        display.drawPixel(screenX, screenY, SSD1306_WHITE);
        display.drawPixel(screenX + 1, screenY, SSD1306_WHITE);
        display.drawPixel(screenX, screenY + 1, SSD1306_WHITE);
        display.drawPixel(screenX - 1, screenY, SSD1306_WHITE);
        display.drawPixel(screenX, screenY - 1, SSD1306_WHITE);
      } else {
        display.drawPixel(screenX, screenY, SSD1306_WHITE);
      }
    }
  }
}

// ==========================================
// 2. 3D Rotating Cube Variables & Structs
// ==========================================
float cubeVertices[8][3] = {
  {-16, -16, -16},
  { 16, -16, -16},
  { 16,  16, -16},
  {-16,  16, -16},
  {-16, -16,  16},
  { 16, -16,  16},
  { 16,  16,  16},
  {-16,  16,  16}
};

float rotX = 0;
float rotY = 0;
float rotZ = 0;

void drawCube() {
  int projected[8][2];
  float centerX = SCREEN_WIDTH / 2;
  float centerY = SCREEN_HEIGHT / 2;
  
  // Increment angles
  rotX += 0.03;
  rotY += 0.04;
  rotZ += 0.02;
  
  // Rotate and project vertices
  for (int i = 0; i < 8; i++) {
    float x = cubeVertices[i][0];
    float y = cubeVertices[i][1];
    float z = cubeVertices[i][2];
    
    // Rotate around X-axis
    float y1 = y * cos(rotX) - z * sin(rotX);
    float z1 = y * sin(rotX) + z * cos(rotX);
    
    // Rotate around Y-axis
    float x2 = x * cos(rotY) + z1 * sin(rotY);
    float z2 = -x * sin(rotY) + z1 * cos(rotY);
    
    // Rotate around Z-axis
    float x3 = x2 * cos(rotZ) - y1 * sin(rotZ);
    float y3 = x2 * sin(rotZ) + y1 * cos(rotZ);
    
    // Simple orthographic projection
    projected[i][0] = (int)(centerX + x3);
    projected[i][1] = (int)(centerY + y3);
  }
  
  // Draw edges
  // Front face
  display.drawLine(projected[0][0], projected[0][1], projected[1][0], projected[1][1], SSD1306_WHITE);
  display.drawLine(projected[1][0], projected[1][1], projected[2][0], projected[2][1], SSD1306_WHITE);
  display.drawLine(projected[2][0], projected[2][1], projected[3][0], projected[3][1], SSD1306_WHITE);
  display.drawLine(projected[3][0], projected[3][1], projected[0][0], projected[0][1], SSD1306_WHITE);
  
  // Back face
  display.drawLine(projected[4][0], projected[4][1], projected[5][0], projected[5][1], SSD1306_WHITE);
  display.drawLine(projected[5][0], projected[5][1], projected[6][0], projected[6][1], SSD1306_WHITE);
  display.drawLine(projected[6][0], projected[6][1], projected[7][0], projected[7][1], SSD1306_WHITE);
  display.drawLine(projected[7][0], projected[7][1], projected[4][0], projected[4][1], SSD1306_WHITE);
  
  // Connection edges between front and back
  for (int i = 0; i < 4; i++) {
    display.drawLine(projected[i][0], projected[i][1], projected[i+4][0], projected[i+4][1], SSD1306_WHITE);
  }
}

// ==========================================
// 3. Plexus Particle Network Variables
// ==========================================
#define NUM_PARTICLES 14
struct Particle {
  float x;
  float y;
  float vx;
  float vy;
};
Particle particles[NUM_PARTICLES];

void initPlexus() {
  for (int i = 0; i < NUM_PARTICLES; i++) {
    particles[i].x = random(2, SCREEN_WIDTH - 2);
    particles[i].y = random(2, SCREEN_HEIGHT - 2);
    particles[i].vx = random(-15, 15) / 10.0;
    particles[i].vy = random(-15, 15) / 10.0;
    // Prevent stationary particles
    if (particles[i].vx == 0) particles[i].vx = 0.8;
    if (particles[i].vy == 0) particles[i].vy = -0.8;
  }
}

void drawPlexus() {
  // Update particles
  for (int i = 0; i < NUM_PARTICLES; i++) {
    particles[i].x += particles[i].vx;
    particles[i].y += particles[i].vy;
    
    // Boundary collision
    if (particles[i].x <= 1 || particles[i].x >= SCREEN_WIDTH - 1) {
      particles[i].vx *= -1;
    }
    if (particles[i].y <= 1 || particles[i].y >= SCREEN_HEIGHT - 1) {
      particles[i].vy *= -1;
    }
    
    // Draw particle
    display.drawPixel((int)particles[i].x, (int)particles[i].y, SSD1306_WHITE);
  }
  
  // Draw connection lines
  for (int i = 0; i < NUM_PARTICLES; i++) {
    for (int j = i + 1; j < NUM_PARTICLES; j++) {
      float dx = particles[i].x - particles[j].x;
      float dy = particles[i].y - particles[j].y;
      float dist = sqrt(dx*dx + dy*dy);
      
      // Draw line if they are close
      if (dist < 26.0) {
        display.drawLine((int)particles[i].x, (int)particles[i].y, (int)particles[j].x, (int)particles[j].y, SSD1306_WHITE);
      }
    }
  }
}

// ==========================================
// 4. Lissajous Curve Variables
// ==========================================
float lissajousPhase = 0;

void drawLissajous() {
  lissajousPhase += 0.05;
  
  float centerX = SCREEN_WIDTH / 2;
  float centerY = SCREEN_HEIGHT / 2;
  float rx = 55.0;
  float ry = 27.0;
  
  // Constants for Lissajous shape: a/b determines the knot style
  float a = 3.0;
  float b = 2.0;
  
  int prevX = -1;
  int prevY = -1;
  
  // Draw curve using connected segments
  for (float t = 0; t <= 2 * M_PI + 0.1; t += 0.08) {
    int x = (int)(centerX + rx * sin(a * t + lissajousPhase));
    int y = (int)(centerY + ry * sin(b * t));
    
    if (prevX != -1) {
      display.drawLine(prevX, prevY, x, y, SSD1306_WHITE);
    }
    prevX = x;
    prevY = y;
  }
}

// ==========================================
// 5. Radar Scanner Variables
// ==========================================
float radarAngle = 0;
#define NUM_BLIPS 3
struct Blip {
  float x;
  float y;
  int brightness; // 0 to 100 for fading effect
};
Blip blips[NUM_BLIPS];

void initRadar() {
  for (int i = 0; i < NUM_BLIPS; i++) {
    float angle = random(0, 360) * M_PI / 180.0;
    float dist = random(10, 28);
    blips[i].x = SCREEN_WIDTH / 2 + cos(angle) * dist;
    blips[i].y = SCREEN_HEIGHT / 2 + sin(angle) * dist;
    blips[i].brightness = 0;
  }
}

void drawRadar() {
  float centerX = SCREEN_WIDTH / 2;
  float centerY = SCREEN_HEIGHT / 2;
  float radius = 29.0;
  
  // Draw background radar circles
  display.drawCircle((int)centerX, (int)centerY, (int)radius, SSD1306_WHITE);
  display.drawCircle((int)centerX, (int)centerY, (int)(radius / 2), SSD1306_WHITE);
  
  // Update sweep line
  radarAngle += 0.04;
  if (radarAngle >= 2 * M_PI) {
    radarAngle -= 2 * M_PI;
  }
  
  int endX = (int)(centerX + cos(radarAngle) * radius);
  int endY = (int)(centerY + sin(radarAngle) * radius);
  
  // Draw sweep line
  display.drawLine((int)centerX, (int)centerY, endX, endY, SSD1306_WHITE);
  
  // Update and draw blips
  for (int i = 0; i < NUM_BLIPS; i++) {
    // Calculate angle to the blip
    float blipAngle = atan2(blips[i].y - centerY, blips[i].x - centerX);
    if (blipAngle < 0) blipAngle += 2 * M_PI;
    
    // Check if the radar sweeps past the blip
    float angleDiff = abs(radarAngle - blipAngle);
    if (angleDiff < 0.08) {
      blips[i].brightness = 100; // Trigger glow
    }
    
    // Fade out blip
    if (blips[i].brightness > 0) {
      blips[i].brightness -= 2;
      if (blips[i].brightness < 0) blips[i].brightness = 0;
      
      // Draw blip (vary size or shape based on brightness)
      if (blips[i].brightness > 50) {
        display.fillCircle((int)blips[i].x, (int)blips[i].y, 2, SSD1306_WHITE);
      } else {
        display.drawPixel((int)blips[i].x, (int)blips[i].y, SSD1306_WHITE);
      }
    }
    
    // Periodically relocate blips when they are fully faded
    if (blips[i].brightness == 0 && random(0, 100) < 2) {
      float newAngle = random(0, 360) * M_PI / 180.0;
      float newDist = random(8, radius - 2);
      blips[i].x = centerX + cos(newAngle) * newDist;
      blips[i].y = centerY + sin(newAngle) * newDist;
    }
  }
}

// ==========================================
// Arduino Setup and Main Loop
// ==========================================
void setup() {
  // Initialize I2C with the customized pins for the integrated OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  
  // Initialize SSD1306 display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    // If initialization fails, halt
    for(;;);
  }
  
  display.clearDisplay();
  display.display();
  
  // Initialize datasets for specific animations
  initStarfield();
  initPlexus();
  initRadar();
  
  lastSwitchTime = millis();
}

void loop() {
  // Clear display buffer
  display.clearDisplay();
  
  // Switch animation based on timer
  unsigned long currentTime = millis();
  if (currentTime - lastSwitchTime >= ANIMATION_DURATION) {
    currentAnimation = (currentAnimation + 1) % TOTAL_ANIMATIONS;
    lastSwitchTime = currentTime;
  }
  
  // Render active animation
  switch (currentAnimation) {
    case 0:
      drawStarfield();
      break;
    case 1:
      drawCube();
      break;
    case 2:
      drawPlexus();
      break;
    case 3:
      drawLissajous();
      break;
    case 4:
      drawRadar();
      break;
  }
  
  // Draw brief text at the top-left to indicate animation mode
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(2, 2);
  switch (currentAnimation) {
    case 0: display.print("1/5: Starfield"); break;
    case 1: display.print("2/5: 3D Cube"); break;
    case 2: display.print("3/5: Plexus"); break;
    case 3: display.print("4/5: Lissajous"); break;
    case 4: display.print("5/5: Radar"); break;
  }
  
  // Push buffer to the hardware display
  display.display();
  
  // Control frame rate (approx. 30 fps)
  delay(30);
}
